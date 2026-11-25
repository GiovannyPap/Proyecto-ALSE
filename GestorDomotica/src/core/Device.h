/**
 * @file Device.h
 * @brief Jerarquía polimórfica de dispositivos domóticos.
 */
#pragma once

#include <memory>

#include "Types.h"

namespace domotica {

/**
 * @brief Clase base abstracta de un dispositivo.
 *
 * Usa el patrón "método plantilla": las operaciones públicas validan reglas comunes
 * (p. ej. un dispositivo en fallo no se puede activar) y delegan el comportamiento
 * específico en los métodos virtuales protegidos `doXxx`.
 */
class Device {
public:
    explicit Device(const DeviceRecord& rec) : rec_(rec) {}
    virtual ~Device() = default;

    virtual DeviceType type() const = 0;
    /// Indica si el dispositivo admite un nivel 0-100 (atenuación o apertura).
    virtual bool supportsLevel() const { return false; }
    /// Potencia que consume mientras está encendido (W).
    virtual double activePowerW() const { return rec_.powerW; }

    Result activate();
    Result deactivate();
    Result setLevel(int level);
    Result reportFault();
    Result repair();

    const DeviceRecord& record() const { return rec_; }
    DeviceState state() const { return rec_.state; }

    /// Fábrica: crea la subclase adecuada según `rec.type`.
    static std::unique_ptr<Device> create(const DeviceRecord& rec);

protected:
    virtual void doActivate() { rec_.state = DeviceState::On; }
    virtual void doDeactivate() { rec_.state = DeviceState::Off; }
    virtual void doSetLevel(int) {}

    DeviceRecord rec_;
};

/// Luz regulable: el consumo es proporcional al nivel.
class Light : public Device {
public:
    using Device::Device;
    DeviceType type() const override { return DeviceType::Light; }
    bool supportsLevel() const override { return true; }
    double activePowerW() const override { return rec_.powerW * rec_.level / 100.0; }

protected:
    void doActivate() override;
    void doSetLevel(int level) override;
};

/// Persiana motorizada: activar = abrir (100 %), desactivar = cerrar (0 %).
class Blind : public Device {
public:
    using Device::Device;
    DeviceType type() const override { return DeviceType::Blind; }
    bool supportsLevel() const override { return true; }

protected:
    void doActivate() override;
    void doDeactivate() override;
    void doSetLevel(int level) override;
};

/// Sensor (movimiento, temperatura, etc.): se enciende/apaga, sin nivel.
class Sensor : public Device {
public:
    using Device::Device;
    DeviceType type() const override { return DeviceType::Sensor; }
};

/// Actuador genérico (relé, cerradura, bomba...).
class Actuator : public Device {
public:
    using Device::Device;
    DeviceType type() const override { return DeviceType::Actuator; }
};

}  // namespace domotica
