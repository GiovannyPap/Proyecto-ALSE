// Pruebas unitarias del núcleo (sin Qt). Compilar con: g++ -std=c++17 ... (ver README).
#include <cstdlib>
#include <iostream>

#include "MemoryRepository.h"
#include "../src/core/AuthService.h"
#include "../src/core/DeviceService.h"
#include "../src/core/Energy.h"
#include "../src/core/Report.h"
#include "../src/core/Sha256.h"

using namespace domotica;

static int g_fail = 0, g_total = 0;
#define CHECK(cond)                                                                   \
    do {                                                                              \
        ++g_total;                                                                    \
        if (!(cond)) { ++g_fail; std::cerr << "FALLO " << __LINE__ << ": " #cond "\n"; } \
    } while (0)
#define CHECK_NEAR(a, b) CHECK(std::abs((a) - (b)) < 1e-9)

static DeviceRecord rec(const std::string& name, DeviceType t, double w, double h, const std::string& room = "Sala") {
    DeviceRecord r;
    r.name = name; r.type = t; r.powerW = w; r.dailyHours = h; r.room = room;
    return r;
}
static int idOf(DeviceService& s, const std::string& name) {
    for (auto& d : s.devices()) if (d.name == name) return d.id;
    return 0;
}

struct Env {
    MemoryRepository repo;
    int64_t now = 1700000000;
    Clock clock = [this] { return now; };
    AuthService auth{repo, clock};
    DeviceService dev{repo, clock};
    RoutineService routines{repo, dev, clock};
};

static void testSha() {
    CHECK(sha256Hex("") == "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    CHECK(sha256Hex("abc") == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    CHECK(sha256Hex("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq") ==
          "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1");
}

static void testAuth() {
    Env e;
    CHECK(!e.auth.hasUsers());
    CHECK(!e.auth.registerUser("ab", "secreto1").ok);          // usuario corto
    CHECK(!e.auth.registerUser("nicolas", "123").ok);          // contraseña corta
    CHECK(!e.auth.registerUser("nico las", "secreto1").ok);    // caracteres inválidos
    CHECK(e.auth.registerUser("Nicolas", "secreto1").ok);
    CHECK(e.auth.hasUsers());
    CHECK(!e.auth.registerUser("nicolas", "otraclave").ok);    // duplicado (sin distinguir mayúsculas)
    CHECK(!e.auth.login("Nicolas", "mala").ok);
    CHECK(!e.auth.login("nadie", "secreto1").ok);
    CHECK(e.auth.currentUser().empty());
    CHECK(e.auth.login("nicolas", "secreto1").ok);
    CHECK(e.auth.currentUser() == "Nicolas");
    e.auth.logout();
    CHECK(e.auth.currentUser().empty());
    auto u = e.repo.findUser("nicolas");
    CHECK(u && u->passwordHash != "secreto1" && u->passwordHash.size() == 64 && u->salt.size() == 32);
}

static void testDeviceValidation() {
    Env e;
    CHECK(!e.dev.registerDevice(rec("  ", DeviceType::Light, 10, 1), "u").ok);
    CHECK(!e.dev.registerDevice(rec("A", DeviceType::Light, -1, 1), "u").ok);
    CHECK(!e.dev.registerDevice(rec("A", DeviceType::Light, 10, 25), "u").ok);
    CHECK(!e.dev.registerDevice(rec(std::string(51, 'x'), DeviceType::Light, 10, 1), "u").ok);
    CHECK(e.dev.registerDevice(rec(" Luz sala ", DeviceType::Light, 60, 5), "u").ok);
    CHECK(e.dev.devices().size() == 1 && e.dev.devices()[0].name == "Luz sala");
    CHECK(!e.dev.registerDevice(rec("LUZ SALA", DeviceType::Light, 60, 5), "u").ok);  // duplicado
    CHECK(e.dev.events(10, 0, "").size() == 1);
    // edición
    int id = idOf(e.dev, "Luz sala");
    DeviceRecord ed = *e.dev.find(id);
    ed.name = "Luz principal"; ed.powerW = 40;
    CHECK(e.dev.updateDevice(ed, "u").ok);
    CHECK(e.dev.find(id)->name == "Luz principal" && e.dev.find(id)->powerW == 40);
    CHECK(e.dev.registerDevice(rec("Otra", DeviceType::Sensor, 1, 24), "u").ok);
    ed.name = "otra";
    CHECK(!e.dev.updateDevice(ed, "u").ok);  // nombre usado por otro
    CHECK(!e.dev.updateDevice(rec("X", DeviceType::Light, 1, 1), "u").ok);  // id 0 inexistente
}

static void testStatesAndSessions() {
    Env e;
    e.dev.registerDevice(rec("Luz", DeviceType::Light, 100, 5), "u");
    e.dev.registerDevice(rec("Persiana", DeviceType::Blind, 50, 1), "u");
    e.dev.registerDevice(rec("Sensor", DeviceType::Sensor, 2, 24), "u");
    const int luz = idOf(e.dev, "Luz"), per = idOf(e.dev, "Persiana"), sen = idOf(e.dev, "Sensor");

    CHECK(e.dev.find(luz)->state == DeviceState::Off);
    CHECK(e.dev.activate(luz, "u").ok);
    CHECK(e.dev.find(luz)->state == DeviceState::On && e.dev.find(luz)->level == 100);
    e.now += 7200;  // 2 h encendida
    CHECK(e.dev.setLevel(luz, 50, "u").ok);
    CHECK(e.dev.find(luz)->level == 50 && e.dev.find(luz)->state == DeviceState::On);
    CHECK_NEAR(Device::create(*e.dev.find(luz))->activePowerW(), 50.0);  // polimorfismo: regulable
    CHECK(e.dev.setLevel(luz, 0, "u").ok);                               // nivel 0 apaga
    CHECK(e.dev.find(luz)->state == DeviceState::Off);
    CHECK(!e.dev.setLevel(luz, 101, "u").ok);
    CHECK(!e.dev.setLevel(sen, 50, "u").ok);                             // sensor sin nivel

    // sin cambios no genera evento
    const size_t n = e.dev.events(100, 0, "").size();
    CHECK(e.dev.deactivate(luz, "u").ok);
    CHECK(e.dev.events(100, 0, "").size() == n);

    // persiana: abrir/cerrar
    CHECK(e.dev.activate(per, "u").ok && e.dev.find(per)->level == 100);
    CHECK(e.dev.deactivate(per, "u").ok && e.dev.find(per)->level == 0 && e.dev.find(per)->state == DeviceState::Off);

    // consumo medido: luz 2 h a 100 W -> 0.2 kWh
    double kwh = 0;
    for (auto& u : e.dev.usage(30)) if (u.deviceId == luz) kwh = measuredKwh(u.seconds, u.powerW);
    CHECK_NEAR(kwh, 0.2);
    // sesión abierta cuenta hasta "ahora"
    e.dev.activate(sen, "u");
    e.now += 3600;
    for (auto& u : e.dev.usage(30)) if (u.deviceId == sen) CHECK(u.seconds == 3600);

    // fallo y reparación
    CHECK(e.dev.reportFault(sen, "u").ok);
    CHECK(e.dev.find(sen)->state == DeviceState::Fault);
    CHECK(!e.dev.reportFault(sen, "u").ok);
    CHECK(!e.dev.activate(sen, "u").ok);      // en fallo no se activa
    CHECK(!e.dev.deactivate(sen, "u").ok);
    e.now += 3600;                            // la sesión se cerró al fallar: no sigue contando
    for (auto& u : e.dev.usage(30)) if (u.deviceId == sen) CHECK(u.seconds == 3600);
    CHECK(e.dev.repair(sen, "u").ok);
    CHECK(e.dev.find(sen)->state == DeviceState::Off);
    CHECK(!e.dev.repair(sen, "u").ok);
    CHECK(e.dev.events(100, sen, "FALLO").size() == 1);
    CHECK(e.dev.events(100, sen, "REPARACION").size() == 1);
    CHECK(!e.dev.activate(9999, "u").ok);

    // eliminar conserva el historial sin referencia al dispositivo
    CHECK(e.dev.removeDevice(luz, "u").ok);
    CHECK(!e.dev.find(luz));
    bool kept = false;
    for (auto& ev : e.dev.events(1000, 0, "")) if (ev.deviceName == "Luz" && ev.deviceId == 0) kept = true;
    CHECK(kept);
    CHECK(!e.dev.removeDevice(luz, "u").ok);
}

static void testEnergy() {
    DeviceRecord r = rec("TV", DeviceType::Actuator, 100, 5);
    EnergyEstimate est = estimateEnergy(r, 800);
    CHECK_NEAR(est.kwhPerDay, 0.5);
    CHECK_NEAR(est.kwhPerMonth, 15.0);
    CHECK_NEAR(est.costPerMonth, 12000.0);
    std::vector<DeviceRecord> v = {r, rec("Luz", DeviceType::Light, 10, 10)};
    CHECK_NEAR(estimateTotal(v, 800).kwhPerDay, 0.6);
    Env e;
    CHECK_NEAR(e.dev.tariff(), 850.0);
    e.dev.setTariff(920.5);
    CHECK_NEAR(e.dev.tariff(), 920.5);
}

static void testRoutines() {
    Env e;
    e.dev.registerDevice(rec("Luz", DeviceType::Light, 10, 1), "u");
    e.dev.registerDevice(rec("Rota", DeviceType::Actuator, 10, 1), "u");
    const int a = idOf(e.dev, "Luz"), b = idOf(e.dev, "Rota");
    e.dev.activate(b, "u"); e.dev.reportFault(b, "u");

    Routine r;
    r.name = "Encender";
    r.minuteOfDay = 18 * 60 + 30;
    r.daysMask = 0b0000011;  // lunes y martes
    r.deviceIds = {a, b};
    CHECK(!e.routines.add(Routine{}, "u").ok);
    Routine bad = r; bad.deviceIds.clear();
    CHECK(!e.routines.add(bad, "u").ok);
    bad = r; bad.daysMask = 0;
    CHECK(!e.routines.add(bad, "u").ok);
    CHECK(e.routines.add(r, "u").ok);

    LocalTime t; t.minuteOfDay = 18 * 60 + 29; t.weekday = 0; t.dayKey = 20261116;
    CHECK(e.routines.tick(t, "sistema").empty());          // aún no es la hora
    t.minuteOfDay = 18 * 60 + 30; t.weekday = 2;           // miércoles: no aplica
    CHECK(e.routines.tick(t, "sistema").empty());
    t.weekday = 0;
    auto out = e.routines.tick(t, "sistema");
    CHECK(out.size() == 1);
    CHECK(out[0].find("1 dispositivo(s) actualizado(s), 1 con error") != std::string::npos);
    CHECK(e.dev.find(a)->state == DeviceState::On);
    CHECK(e.routines.tick(t, "sistema").empty());          // no se repite el mismo día
    t.dayKey = 20261117; t.weekday = 1;
    CHECK(e.routines.tick(t, "sistema").size() == 1);      // al día siguiente sí

    // apagar + deshabilitada
    Routine off = r; off.name = "Apagar"; off.action = RoutineAction::TurnOff; off.minuteOfDay = 23 * 60;
    off.deviceIds = {a}; off.enabled = false;
    CHECK(e.routines.add(off, "u").ok);
    t.minuteOfDay = 23 * 60;
    CHECK(e.routines.tick(t, "sistema").empty());
    auto list = e.routines.list();
    for (auto& x : list) if (x.name == "Apagar") { x.enabled = true; CHECK(e.routines.update(x, "u").ok); }
    t.dayKey = 20261118;
    CHECK(e.routines.tick(t, "sistema").size() == 1);
    CHECK(e.dev.find(a)->state == DeviceState::Off);

    // al eliminar un dispositivo, sale de las rutinas
    e.dev.removeDevice(a, "u");
    for (auto& x : e.routines.list()) CHECK(std::find(x.deviceIds.begin(), x.deviceIds.end(), a) == x.deviceIds.end());
    CHECK(e.routines.remove(list[0].id, "u").ok);
}

static void testTimeAndCsv() {
    setenv("TZ", "UTC", 1);
    tzset();
    CHECK(formatTimestamp(0) == "1970-01-01 00:00:00");
    LocalTime lt = localTimeFrom(1700000000);  // 2023-11-14 22:13:20 UTC (martes)
    CHECK(lt.minuteOfDay == 22 * 60 + 13 && lt.weekday == 1 && lt.dayKey == 20231114);
    CHECK(formatMinute(65) == "01:05");
    CHECK(daysToString(0x7F) == "Todos los días" && daysToString(0b101) == "Lun,Mié");
    Event ev; ev.timestamp = 0; ev.deviceName = "Luz \"sala\""; ev.kind = "ENCENDIDO"; ev.message = "a,b"; ev.username = "u";
    const std::string csv = eventsToCsv({ev});
    CHECK(csv == "Fecha y hora,Dispositivo,Tipo de evento,Detalle,Usuario\n"
                 "\"1970-01-01 00:00:00\",\"Luz \"\"sala\"\"\",\"ENCENDIDO\",\"a,b\",\"u\"\n");
}

int main() {
    testSha();
    testAuth();
    testDeviceValidation();
    testStatesAndSessions();
    testEnergy();
    testRoutines();
    testTimeAndCsv();
    std::cout << (g_total - g_fail) << "/" << g_total << " comprobaciones correctas\n";
    return g_fail == 0 ? 0 : 1;
}
