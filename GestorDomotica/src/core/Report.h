/**
 * @file Report.h
 * @brief Exportación del historial a CSV.
 */
#pragma once

#include <string>
#include <vector>

#include "Types.h"

namespace domotica {
/// Genera un CSV (separador coma, campos escapados) con cabecera en español.
std::string eventsToCsv(const std::vector<Event>& events);
}  // namespace domotica
