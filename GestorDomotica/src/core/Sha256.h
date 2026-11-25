/**
 * @file Sha256.h
 * @brief SHA-256 propio, para no depender de librerías externas en el núcleo.
 */
#pragma once

#include <string>

namespace domotica {
/// Devuelve el resumen SHA-256 de `data` en hexadecimal minúscula (64 caracteres).
std::string sha256Hex(const std::string& data);
}  // namespace domotica
