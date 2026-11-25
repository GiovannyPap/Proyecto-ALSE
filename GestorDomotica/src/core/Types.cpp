#include "Types.h"

namespace domotica {

std::string toString(DeviceType t) {
    switch (t) {
        case DeviceType::Light: return "light";
        case DeviceType::Blind: return "blind";
        case DeviceType::Sensor: return "sensor";
        case DeviceType::Actuator: return "actuator";
    }
    return "light";
}

std::string toString(DeviceState s) {
    switch (s) {
        case DeviceState::Off: return "off";
        case DeviceState::On: return "on";
        case DeviceState::Fault: return "fault";
    }
    return "off";
}

bool parseDeviceType(const std::string& text, DeviceType& out) {
    for (DeviceType t : {DeviceType::Light, DeviceType::Blind, DeviceType::Sensor, DeviceType::Actuator}) {
        if (toString(t) == text) { out = t; return true; }
    }
    return false;
}

bool parseDeviceState(const std::string& text, DeviceState& out) {
    for (DeviceState s : {DeviceState::Off, DeviceState::On, DeviceState::Fault}) {
        if (toString(s) == text) { out = s; return true; }
    }
    return false;
}

std::string label(DeviceType t) {
    switch (t) {
        case DeviceType::Light: return "Luz";
        case DeviceType::Blind: return "Persiana";
        case DeviceType::Sensor: return "Sensor";
        case DeviceType::Actuator: return "Actuador";
    }
    return "";
}

std::string label(DeviceState s) {
    switch (s) {
        case DeviceState::Off: return "Apagado";
        case DeviceState::On: return "Encendido";
        case DeviceState::Fault: return "Fallo";
    }
    return "";
}

}  // namespace domotica
