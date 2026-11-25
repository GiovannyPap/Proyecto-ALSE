#include "AuthService.h"

#include <random>

#include "Sha256.h"

namespace domotica {

namespace {
std::string randomSalt() {
    static const char* hex = "0123456789abcdef";
    std::random_device rd;
    std::string s;
    for (int i = 0; i < 32; ++i) s.push_back(hex[rd() % 16]);
    return s;
}

bool validUsername(const std::string& u) {
    if (u.size() < 3 || u.size() > 32) return false;
    for (char c : u) {
        const bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
                        c == '_' || c == '.';
        if (!ok) return false;
    }
    return true;
}
}  // namespace

std::string AuthService::hashPassword(const std::string& salt, const std::string& password) {
    std::string h = sha256Hex(salt + password);
    for (int i = 0; i < 5000; ++i) h = sha256Hex(h + salt + password);
    return h;
}

Result AuthService::registerUser(const std::string& username, const std::string& password) {
    if (!validUsername(username))
        return Result::failure("El usuario debe tener 3-32 caracteres (letras, números, _ o .).");
    if (password.size() < 6) return Result::failure("La contraseña debe tener al menos 6 caracteres.");
    if (repo_.findUser(username)) return Result::failure("Ese nombre de usuario ya existe.");
    User u;
    u.username = username;
    u.salt = randomSalt();
    u.passwordHash = hashPassword(u.salt, password);
    u.createdAt = clock_();
    if (!repo_.addUser(u)) return Result::failure("No se pudo guardar el usuario.");
    return Result::success("Usuario creado.");
}

Result AuthService::login(const std::string& username, const std::string& password) {
    const auto user = repo_.findUser(username);
    if (!user || hashPassword(user->salt, password) != user->passwordHash)
        return Result::failure("Usuario o contraseña incorrectos.");
    current_ = user->username;
    return Result::success("Sesión iniciada.");
}

}  // namespace domotica
