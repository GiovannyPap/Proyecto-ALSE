/**
 * @file DeviceService.h
 * @brief Controlador de dispositivos: validación, cambios de estado, historial y consumo.
 */
#pragma once

#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "Device.h"
#include "IRepository.h"

namespace domotica {

class DeviceService {
public:
    DeviceService(IRepository& repo, Clock clock) : repo_(repo), clock_(std::move(clock)) {}

    Result registerDevice(DeviceRecord rec, const std::string& user);
    Result updateDevice(DeviceRecord rec, const std::string& user);
    Result removeDevice(int id, const std::string& user);

    Result activate(int id, const std::string& user);
    Result deactivate(int id, const std::string& user);
    Result setLevel(int id, int level, const std::string& user);
    Result reportFault(int id, const std::string& user);
    Result repair(int id, const std::string& user);

    std::vector<DeviceRecord> devices() { return repo_.listDevices(); }
    std::optional<DeviceRecord> find(int id) { return repo_.getDevice(id); }
    std::vector<Event> events(int limit, int deviceId, const std::string& kind) {
        return repo_.listEvents(limit, deviceId, kind);
    }
    /// Tiempo medido de encendido por dispositivo en los últimos `days` días.
    std::vector<UsageRow> usage(int days);

    double tariff();                 ///< Tarifa en $/kWh (persistida).
    void setTariff(double value);

private:
    Result apply(int id, const std::string& user, const std::function<Result(Device&)>& op);
    void log(const std::string& kind, int deviceId, const std::string& deviceName, const std::string& message,
             const std::string& user);

    IRepository& repo_;
    Clock clock_;
};

/// Rutinas programadas: administración y ejecución periódica.
class RoutineService {
public:
    RoutineService(IRepository& repo, DeviceService& devices, Clock clock)
        : repo_(repo), devices_(devices), clock_(std::move(clock)) {}

    Result add(const Routine& r, const std::string& user);
    Result update(const Routine& r, const std::string& user);
    Result remove(int id, const std::string& user);
    std::vector<Routine> list() { return repo_.listRoutines(); }

    /// Ejecuta las rutinas que correspondan a `now`; devuelve un resumen por rutina ejecutada.
    std::vector<std::string> tick(const LocalTime& now, const std::string& user);

private:
    Result validate(const Routine& r) const;

    IRepository& repo_;
    DeviceService& devices_;
    Clock clock_;
    RoutineEngine engine_;
};

}  // namespace domotica
