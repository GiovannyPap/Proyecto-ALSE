/**
 * @file Types.h
 * @brief Tipos de dominio compartidos (sin dependencias de Qt).
 */
#pragma once

#include <cstdint>
#include <functional>
#include <string>

namespace domotica {

/// Fuente de tiempo inyectable (segundos desde epoch). Permite pruebas deterministas.
using Clock = std::function<int64_t()>;

/// Categorías de dispositivo domótico soportadas.
enum class DeviceType { Light, Blind, Sensor, Actuator };

/// Estados válidos de un dispositivo.
enum class DeviceState { Off, On, Fault };

std::string toString(DeviceType t);   ///< Valor persistido en BD ("light", ...).
std::string toString(DeviceState s);  ///< Valor persistido en BD ("off", "on", "fault").
bool parseDeviceType(const std::string& text, DeviceType& out);
bool parseDeviceState(const std::string& text, DeviceState& out);
std::string label(DeviceType t);   ///< Etiqueta en español para la GUI.
std::string label(DeviceState s);  ///< Etiqueta en español para la GUI.

/// Fila de la tabla `devices`.
struct DeviceRecord {
    int id = 0;
    std::string name;
    std::string room;
    DeviceType type = DeviceType::Light;
    DeviceState state = DeviceState::Off;
    double powerW = 0.0;      ///< Potencia nominal en vatios.
    double dailyHours = 0.0;  ///< Promedio de horas de uso por día (para la estimación).
    int level = 0;            ///< Atenuación (luz) o apertura (persiana), 0-100.
};

/// Evento del historial (tabla `events`).
struct Event {
    int64_t id = 0;
    int64_t timestamp = 0;
    int deviceId = 0;  ///< 0 si no aplica o si el dispositivo ya fue eliminado.
    std::string deviceName;
    std::string kind;  ///< REGISTRO, EDICION, ELIMINACION, ENCENDIDO, APAGADO, FALLO, REPARACION, NIVEL, RUTINA.
    std::string message;
    std::string username;
};

/// Tiempo medido de encendido de un dispositivo dentro de una ventana.
struct UsageRow {
    int deviceId = 0;
    std::string deviceName;
    double powerW = 0.0;
    int64_t seconds = 0;
};

/// Usuario de la aplicación.
struct User {
    int id = 0;
    std::string username;
    std::string salt;
    std::string passwordHash;
    int64_t createdAt = 0;
};

/// Resultado de una operación con mensaje para el usuario.
struct Result {
    bool ok = true;
    std::string message;
    static Result success(const std::string& msg = "") { return Result{true, msg}; }
    static Result failure(const std::string& msg) { return Result{false, msg}; }
};

}  // namespace domotica
