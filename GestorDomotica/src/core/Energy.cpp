#include "Energy.h"

namespace domotica {

EnergyEstimate estimateEnergy(const DeviceRecord& rec, double tariffPerKwh, int daysPerMonth) {
    EnergyEstimate e;
    e.kwhPerDay = rec.powerW * rec.dailyHours / 1000.0;
    e.kwhPerMonth = e.kwhPerDay * daysPerMonth;
    e.costPerMonth = e.kwhPerMonth * tariffPerKwh;
    return e;
}

EnergyEstimate estimateTotal(const std::vector<DeviceRecord>& devices, double tariffPerKwh) {
    EnergyEstimate total;
    for (const auto& d : devices) {
        const EnergyEstimate e = estimateEnergy(d, tariffPerKwh);
        total.kwhPerDay += e.kwhPerDay;
        total.kwhPerMonth += e.kwhPerMonth;
        total.costPerMonth += e.costPerMonth;
    }
    return total;
}

double measuredKwh(int64_t seconds, double powerW) {
    return powerW * (static_cast<double>(seconds) / 3600.0) / 1000.0;
}

}  // namespace domotica
