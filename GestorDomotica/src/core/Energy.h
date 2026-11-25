/**
 * @file Energy.h
 * @brief Estimación del consumo energético por potencia y uso promedio.
 */
#pragma once

#include <vector>

#include "Types.h"

namespace domotica {

/// Consumo estimado de un dispositivo (o del total).
struct EnergyEstimate {
    double kwhPerDay = 0.0;
    double kwhPerMonth = 0.0;
    double costPerMonth = 0.0;
};

/// kWh/día = W * h/día / 1000; mes = 30 días; costo = kWh/mes * tarifa.
EnergyEstimate estimateEnergy(const DeviceRecord& rec, double tariffPerKwh, int daysPerMonth = 30);
/// Suma las estimaciones de todos los dispositivos.
EnergyEstimate estimateTotal(const std::vector<DeviceRecord>& devices, double tariffPerKwh);
/// Energía realmente medida: segundos encendido * potencia.
double measuredKwh(int64_t seconds, double powerW);

}  // namespace domotica
