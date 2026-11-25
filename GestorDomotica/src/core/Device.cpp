#include "Device.h"

namespace domotica {

Result Device::activate() {
    if (rec_.state == DeviceState::Fault)
        return Result::failure("«" + rec_.name + "» está en fallo; repárelo antes de activarlo.");
    doActivate();
    return Result::success();
}

Result Device::deactivate() {
    if (rec_.state == DeviceState::Fault)
        return Result::failure("«" + rec_.name + "» está en fallo; repárelo primero.");
    doDeactivate();
    return Result::success();
}

Result Device::setLevel(int level) {
    if (!supportsLevel()) return Result::failure("Este tipo de dispositivo no admite nivel.");
    if (level < 0 || level > 100) return Result::failure("El nivel debe estar entre 0 y 100.");
    if (rec_.state == DeviceState::Fault)
        return Result::failure("«" + rec_.name + "» está en fallo; repárelo primero.");
    doSetLevel(level);
    return Result::success();
}

Result Device::reportFault() {
    if (rec_.state == DeviceState::Fault)
        return Result::failure("«" + rec_.name + "» ya está en fallo.");
    rec_.state = DeviceState::Fault;
    return Result::success();
}

Result Device::repair() {
    if (rec_.state != DeviceState::Fault)
        return Result::failure("«" + rec_.name + "» no está en fallo.");
    doDeactivate();  // vuelve apagado y en posición coherente
    return Result::success();
}

std::unique_ptr<Device> Device::create(const DeviceRecord& rec) {
    switch (rec.type) {
        case DeviceType::Light: return std::make_unique<Light>(rec);
        case DeviceType::Blind: return std::make_unique<Blind>(rec);
        case DeviceType::Sensor: return std::make_unique<Sensor>(rec);
        case DeviceType::Actuator: return std::make_unique<Actuator>(rec);
    }
    return std::make_unique<Actuator>(rec);
}

void Light::doActivate() {
    rec_.state = DeviceState::On;
    if (rec_.level == 0) rec_.level = 100;
}

void Light::doSetLevel(int level) {
    rec_.level = level;
    rec_.state = level > 0 ? DeviceState::On : DeviceState::Off;
}

void Blind::doActivate() {
    rec_.state = DeviceState::On;
    rec_.level = 100;
}

void Blind::doDeactivate() {
    rec_.state = DeviceState::Off;
    rec_.level = 0;
}

void Blind::doSetLevel(int level) {
    rec_.level = level;
    rec_.state = level > 0 ? DeviceState::On : DeviceState::Off;
}

}  // namespace domotica
