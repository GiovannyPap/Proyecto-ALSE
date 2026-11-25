/**
 * @file AuthService.h
 * @brief Registro e inicio de sesión de usuarios (contraseñas con sal + SHA-256 iterado).
 */
#pragma once

#include <string>

#include "IRepository.h"

namespace domotica {

class AuthService {
public:
    AuthService(IRepository& repo, Clock clock) : repo_(repo), clock_(std::move(clock)) {}

    Result registerUser(const std::string& username, const std::string& password);
    Result login(const std::string& username, const std::string& password);
    void logout() { current_.clear(); }
    bool hasUsers() { return repo_.userCount() > 0; }
    const std::string& currentUser() const { return current_; }

    /// Hash de contraseña: SHA-256 iterado 5000 veces sobre sal+contraseña.
    static std::string hashPassword(const std::string& salt, const std::string& password);

private:
    IRepository& repo_;
    Clock clock_;
    std::string current_;
};

}  // namespace domotica
