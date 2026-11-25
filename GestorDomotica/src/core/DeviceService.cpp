#include "DeviceService.h"

#include <algorithm>
#include <cmath>

#include "Time.h"

namespace domotica {

namespace {

std::string trim(const std::string& s) {
    const char* ws = " \t\r\n";
    const auto b = s.find_first_not_of(ws);
    if (b == std::string::npos) return "";
    const auto e = s.find_last_not_of(ws);
    return s.substr(b, e - b + 1);
}

size_t utf8Length(const std::string& s) {
    size_t n = 0;
    for (unsigned char c : s) {
        if ((c & 0xC0) != 0x80) ++n;
    }
    return n;
}

Result validate(const DeviceRecord& r) {
    if (r.name.empty()) return Result::failure("El nombre del dispositivo es obligatorio.");
    if (utf8Length(r.name) > 50) return Result::failure("El nombre admite máximo 50 caracteres.");
    if (utf8Length(r.room) > 50) return Result::failure("La habitación admite máximo 50 caracteres.");
    if (!std::isfinite(r.powerW) || r.powerW < 0 || r.powerW > 20000)
        return Result::failure("La potencia debe estar entre 0 y 20000 W.");
    if (!std::isfinite(r.dailyHours) || r.dailyHours < 0 || r.dailyHours > 24)
        return Result::failure("Las horas de uso diarias deben estar entre 0 y 24.");
    return Result::success();
}

std::string describe(const Device& dev) {
    const DeviceRecord& r = dev.record();
    std::string d = r.name + ": " + label(r.state);
    if (dev.supportsLevel()) d += " (" + std::to_string(r.level) + " %)";
    return d;
}

}  // namespace

void DeviceService::log(const std::string& kind, int deviceId, const std::string& deviceName,
                        const std::string& message, const std::string& user) {
    Event e;
    e.timestamp = clock_();
    e.deviceId = deviceId;
    e.deviceName = deviceName;
    e.kind = kind;
    e.message = message;
    e.username = user;
    repo_.addEvent(e);
}

Result DeviceService::registerDevice(DeviceRecord rec, const std::string& user) {
    rec.name = trim(rec.name);
    rec.room = trim(rec.room);
    Result v = validate(rec);
    if (!v.ok) return v;
    if (repo_.deviceNameExists(rec.name, 0)) return Result::failure("Ya existe un dispositivo con ese nombre.");
    rec.id = 0;
    rec.state = DeviceState::Off;
    rec.level = 0;
    const int id = repo_.addDevice(rec);
    if (id <= 0) return Result::failure("No se pudo guardar el dispositivo en la base de datos.");
    log("REGISTRO", id, rec.name, "Dispositivo registrado (" + label(rec.type) + ", " + rec.room + ")", user);
    return Result::success("Dispositivo «" + rec.name + "» registrado.");
}

Result DeviceService::updateDevice(DeviceRecord rec, const std::string& user) {
    const auto cur = repo_.getDevice(rec.id);
    if (!cur) return Result::failure("El dispositivo no existe.");
    rec.name = trim(rec.name);
    rec.room = trim(rec.room);
    Result v = validate(rec);
    if (!v.ok) return v;
    if (repo_.deviceNameExists(rec.name, rec.id)) return Result::failure("Ya existe un dispositivo con ese nombre.");
    rec.state = cur->state;
    rec.level = cur->level;
    if (rec.type != cur->type) {
        rec.level = (rec.state == DeviceState::On && Device::create(rec)->supportsLevel()) ? 100 : 0;
    }
    if (!repo_.updateDevice(rec)) return Result::failure("No se pudo actualizar el dispositivo.");
    log("EDICION", rec.id, rec.name, "Datos del dispositivo actualizados", user);
    return Result::success("Dispositivo «" + rec.name + "» actualizado.");
}

Result DeviceService::removeDevice(int id, const std::string& user) {
    const auto cur = repo_.getDevice(id);
    if (!cur) return Result::failure("El dispositivo no existe.");
    if (!repo_.removeDevice(id)) return Result::failure("No se pudo eliminar el dispositivo.");
    log("ELIMINACION", 0, cur->name, "Dispositivo eliminado", user);
    return Result::success("Dispositivo «" + cur->name + "» eliminado.");
}

Result DeviceService::apply(int id, const std::string& user, const std::function<Result(Device&)>& op) {
    const auto before = repo_.getDevice(id);
    if (!before) return Result::failure("El dispositivo no existe.");
    auto dev = Device::create(*before);
    Result r = op(*dev);
    if (!r.ok) return r;
    const DeviceRecord& after = dev->record();
    const bool stateChanged = after.state != before->state;
    const bool levelChanged = after.level != before->level;
    if (!stateChanged && !levelChanged)
        return Result::success("Sin cambios: «" + before->name + "» ya estaba en ese estado.");
    if (!repo_.updateDevice(after)) return Result::failure("No se pudo guardar el cambio en la base de datos.");

    const int64_t now = clock_();
    if (before->state != DeviceState::On && after.state == DeviceState::On) repo_.openSession(id, now);
    else if (before->state == DeviceState::On && after.state != DeviceState::On) repo_.closeSession(id, now);

    std::string kind;
    if (stateChanged) {
        if (after.state == DeviceState::Fault) kind = "FALLO";
        else if (before->state == DeviceState::Fault) kind = "REPARACION";
        else if (after.state == DeviceState::On) kind = "ENCENDIDO";
        else kind = "APAGADO";
    } else {
        kind = "NIVEL";
    }
    const std::string msg = describe(*dev);
    log(kind, id, after.name, msg, user);
    return Result::success(msg);
}

Result DeviceService::activate(int id, const std::string& user) {
    return apply(id, user, [](Device& d) { return d.activate(); });
}
Result DeviceService::deactivate(int id, const std::string& user) {
    return apply(id, user, [](Device& d) { return d.deactivate(); });
}
Result DeviceService::setLevel(int id, int level, const std::string& user) {
    return apply(id, user, [level](Device& d) { return d.setLevel(level); });
}
Result DeviceService::reportFault(int id, const std::string& user) {
    return apply(id, user, [](Device& d) { return d.reportFault(); });
}
Result DeviceService::repair(int id, const std::string& user) {
    return apply(id, user, [](Device& d) { return d.repair(); });
}

std::vector<UsageRow> DeviceService::usage(int days) {
    const int64_t now = clock_();
    return repo_.usage(now - static_cast<int64_t>(days) * 86400, now);
}

double DeviceService::tariff() {
    try {
        return std::stod(repo_.getSetting("tariff_per_kwh", "850"));
    } catch (...) {
        return 850.0;
    }
}

void DeviceService::setTariff(double value) { repo_.setSetting("tariff_per_kwh", std::to_string(value)); }

// ---------------------------------------------------------------- RoutineService

Result RoutineService::validate(const Routine& r) const {
    if (r.name.empty() || r.name.size() > 100) return Result::failure("La rutina necesita un nombre (máx. 50 caracteres).");
    if (r.minuteOfDay < 0 || r.minuteOfDay > 1439) return Result::failure("Hora inválida.");
    if ((r.daysMask & 0x7F) == 0) return Result::failure("Seleccione al menos un día.");
    if (r.deviceIds.empty()) return Result::failure("Seleccione al menos un dispositivo.");
    return Result::success();
}

Result RoutineService::add(const Routine& in, const std::string& user) {
    Routine r = in;
    r.name = trim(r.name);
    Result v = validate(r);
    if (!v.ok) return v;
    r.id = 0;
    if (repo_.addRoutine(r) <= 0) return Result::failure("No se pudo guardar la rutina.");
    Event e;
    e.timestamp = clock_();
    e.kind = "RUTINA";
    e.message = "Rutina «" + r.name + "» creada (" + formatMinute(r.minuteOfDay) + ", " + daysToString(r.daysMask) + ")";
    e.username = user;
    repo_.addEvent(e);
    return Result::success("Rutina «" + r.name + "» creada.");
}

Result RoutineService::update(const Routine& in, const std::string& user) {
    Routine r = in;
    r.name = trim(r.name);
    Result v = validate(r);
    if (!v.ok) return v;
    if (!repo_.updateRoutine(r)) return Result::failure("No se pudo actualizar la rutina.");
    Event e;
    e.timestamp = clock_();
    e.kind = "RUTINA";
    e.message = "Rutina «" + r.name + "» actualizada";
    e.username = user;
    repo_.addEvent(e);
    return Result::success("Rutina «" + r.name + "» actualizada.");
}

Result RoutineService::remove(int id, const std::string& user) {
    std::string name;
    for (const Routine& r : repo_.listRoutines()) {
        if (r.id == id) name = r.name;
    }
    if (!repo_.removeRoutine(id)) return Result::failure("No se pudo eliminar la rutina.");
    Event e;
    e.timestamp = clock_();
    e.kind = "RUTINA";
    e.message = "Rutina «" + name + "» eliminada";
    e.username = user;
    repo_.addEvent(e);
    return Result::success("Rutina eliminada.");
}

std::vector<std::string> RoutineService::tick(const LocalTime& now, const std::string& user) {
    std::vector<std::string> summaries;
    for (const Routine& r : engine_.due(repo_.listRoutines(), now)) {
        int ok = 0, failed = 0;
        for (int id : r.deviceIds) {
            const Result res = r.action == RoutineAction::TurnOn ? devices_.activate(id, user)
                                                                 : devices_.deactivate(id, user);
            if (res.ok) ++ok;
            else ++failed;
        }
        std::string msg = "Rutina «" + r.name + "» (" + std::string(r.action == RoutineAction::TurnOn ? "encender" : "apagar") +
                          "): " + std::to_string(ok) + " dispositivo(s) actualizado(s)";
        if (failed > 0) msg += ", " + std::to_string(failed) + " con error";
        Event e;
        e.timestamp = clock_();
        e.kind = "RUTINA";
        e.message = msg;
        e.username = user;
        repo_.addEvent(e);
        summaries.push_back(msg);
    }
    return summaries;
}

}  // namespace domotica
