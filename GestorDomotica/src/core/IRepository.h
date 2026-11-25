/**
 * @file IRepository.h
 * @brief Interfaz de persistencia. La implementa SqliteRepository (QtSql) y,
 *        en las pruebas, un repositorio en memoria.
 */
#pragma once

#include <optional>
#include <string>
#include <vector>

#include "Routine.h"
#include "Types.h"

namespace domotica {

class IRepository {
public:
    virtual ~IRepository() = default;

    // Usuarios
    virtual bool addUser(const User& user) = 0;
    virtual std::optional<User> findUser(const std::string& username) = 0;
    virtual int userCount() = 0;

    // Dispositivos
    virtual int addDevice(const DeviceRecord& rec) = 0;  ///< devuelve el id nuevo, 0 si falla
    virtual bool updateDevice(const DeviceRecord& rec) = 0;
    virtual bool removeDevice(int id) = 0;
    virtual std::optional<DeviceRecord> getDevice(int id) = 0;
    virtual std::vector<DeviceRecord> listDevices() = 0;
    virtual bool deviceNameExists(const std::string& name, int excludeId) = 0;

    // Eventos (deviceId 0 = todos; kind vacío = todos; más recientes primero)
    virtual void addEvent(const Event& ev) = 0;
    virtual std::vector<Event> listEvents(int limit, int deviceId, const std::string& kind) = 0;

    // Sesiones de encendido (base del consumo medido)
    virtual void openSession(int deviceId, int64_t ts) = 0;
    virtual void closeSession(int deviceId, int64_t ts) = 0;
    virtual std::vector<UsageRow> usage(int64_t from, int64_t now) = 0;

    // Rutinas
    virtual int addRoutine(const Routine& r) = 0;
    virtual bool updateRoutine(const Routine& r) = 0;
    virtual bool removeRoutine(int id) = 0;
    virtual std::vector<Routine> listRoutines() = 0;

    // Configuración clave/valor
    virtual std::string getSetting(const std::string& key, const std::string& def) = 0;
    virtual void setSetting(const std::string& key, const std::string& value) = 0;
};

}  // namespace domotica
