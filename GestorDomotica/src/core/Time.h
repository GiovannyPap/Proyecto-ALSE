/**
 * @file Time.h
 * @brief Utilidades de fecha/hora local (sin Qt).
 */
#pragma once

#include <cstdint>
#include <string>

namespace domotica {

/// Descomposición de una marca de tiempo en hora local.
struct LocalTime {
    int minuteOfDay = 0;  ///< 0..1439
    int weekday = 0;      ///< 0 = lunes ... 6 = domingo
    int dayKey = 0;       ///< aaaammdd, identifica el día calendario
};

int64_t systemClock();                          ///< Segundos desde epoch (reloj del sistema).
LocalTime localTimeFrom(int64_t epoch);         ///< Convierte epoch a hora local.
std::string formatTimestamp(int64_t epoch);     ///< "aaaa-mm-dd HH:MM:SS" en hora local.
std::string formatMinute(int minuteOfDay);      ///< "HH:MM".
std::string daysToString(int daysMask);         ///< Ej.: "Lun,Mié" o "Todos los días".

}  // namespace domotica
