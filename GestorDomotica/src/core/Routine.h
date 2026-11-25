/**
 * @file Routine.h
 * @brief Rutinas programadas de dispositivos y su motor de evaluación.
 */
#pragma once

#include <map>
#include <string>
#include <vector>

#include "Time.h"
#include "Types.h"

namespace domotica {

enum class RoutineAction { TurnOn, TurnOff };

/// Rutina: a cierta hora y días, enciende o apaga un conjunto de dispositivos.
struct Routine {
    int id = 0;
    std::string name;
    int minuteOfDay = 0;                       ///< 0..1439
    RoutineAction action = RoutineAction::TurnOn;
    int daysMask = 0x7F;                       ///< bit0 = lunes ... bit6 = domingo
    bool enabled = true;
    std::vector<int> deviceIds;
};

/// Decide qué rutinas deben ejecutarse en un instante dado (una vez por día calendario).
class RoutineEngine {
public:
    std::vector<Routine> due(const std::vector<Routine>& routines, const LocalTime& now);

private:
    std::map<int, int> lastRunDay_;  ///< id de rutina -> dayKey de su última ejecución
};

}  // namespace domotica
