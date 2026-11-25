#include "Routine.h"

namespace domotica {

std::vector<Routine> RoutineEngine::due(const std::vector<Routine>& routines, const LocalTime& now) {
    std::vector<Routine> out;
    for (const Routine& r : routines) {
        if (!r.enabled) continue;
        if (((r.daysMask >> now.weekday) & 1) == 0) continue;
        if (r.minuteOfDay != now.minuteOfDay) continue;
        auto it = lastRunDay_.find(r.id);
        if (it != lastRunDay_.end() && it->second == now.dayKey) continue;
        lastRunDay_[r.id] = now.dayKey;
        out.push_back(r);
    }
    return out;
}

}  // namespace domotica
