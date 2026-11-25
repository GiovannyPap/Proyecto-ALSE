/**
 * @file Sql.h
 * @brief Sentencias SQL de la aplicación. Cada constante contiene UNA sola sentencia.
 *
 * Los parámetros usan marcadores con nombre (":nombre") y cada nombre aparece una sola
 * vez por sentencia. tests/test_sql.py ejecuta estas mismas sentencias contra SQLite real.
 */
#pragma once

namespace domotica {
namespace sql {

// ------------------------------------------------------------------ Esquema (3FN)
inline constexpr const char* kCreateUsers = R"SQL(
CREATE TABLE IF NOT EXISTS users (
    id            INTEGER PRIMARY KEY AUTOINCREMENT,
    username      TEXT    NOT NULL UNIQUE COLLATE NOCASE,
    salt          TEXT    NOT NULL,
    password_hash TEXT    NOT NULL,
    created_at    INTEGER NOT NULL
))SQL";

inline constexpr const char* kCreateDevices = R"SQL(
CREATE TABLE IF NOT EXISTS devices (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    name        TEXT    NOT NULL UNIQUE COLLATE NOCASE,
    room        TEXT    NOT NULL DEFAULT '',
    type        TEXT    NOT NULL CHECK (type IN ('light','blind','sensor','actuator')),
    state       TEXT    NOT NULL DEFAULT 'off' CHECK (state IN ('off','on','fault')),
    power_w     REAL    NOT NULL CHECK (power_w >= 0),
    daily_hours REAL    NOT NULL CHECK (daily_hours BETWEEN 0 AND 24),
    level       INTEGER NOT NULL DEFAULT 0 CHECK (level BETWEEN 0 AND 100)
))SQL";

inline constexpr const char* kCreateEvents = R"SQL(
CREATE TABLE IF NOT EXISTS events (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    ts          INTEGER NOT NULL,
    device_id   INTEGER REFERENCES devices(id) ON DELETE SET NULL,
    device_name TEXT    NOT NULL DEFAULT '',
    kind        TEXT    NOT NULL,
    message     TEXT    NOT NULL,
    username    TEXT    NOT NULL DEFAULT ''
))SQL";

inline constexpr const char* kCreateEventsTsIdx =
    "CREATE INDEX IF NOT EXISTS idx_events_ts ON events(ts DESC)";
inline constexpr const char* kCreateEventsDeviceIdx =
    "CREATE INDEX IF NOT EXISTS idx_events_device ON events(device_id)";

inline constexpr const char* kCreateSessions = R"SQL(
CREATE TABLE IF NOT EXISTS sessions (
    id        INTEGER PRIMARY KEY AUTOINCREMENT,
    device_id INTEGER NOT NULL REFERENCES devices(id) ON DELETE CASCADE,
    start_ts  INTEGER NOT NULL,
    end_ts    INTEGER
))SQL";

inline constexpr const char* kCreateSessionsIdx =
    "CREATE INDEX IF NOT EXISTS idx_sessions_device ON sessions(device_id)";

inline constexpr const char* kCreateRoutines = R"SQL(
CREATE TABLE IF NOT EXISTS routines (
    id            INTEGER PRIMARY KEY AUTOINCREMENT,
    name          TEXT    NOT NULL,
    minute_of_day INTEGER NOT NULL CHECK (minute_of_day BETWEEN 0 AND 1439),
    action        TEXT    NOT NULL CHECK (action IN ('on','off')),
    days_mask     INTEGER NOT NULL DEFAULT 127,
    enabled       INTEGER NOT NULL DEFAULT 1
))SQL";

inline constexpr const char* kCreateRoutineDevices = R"SQL(
CREATE TABLE IF NOT EXISTS routine_devices (
    routine_id INTEGER NOT NULL REFERENCES routines(id) ON DELETE CASCADE,
    device_id  INTEGER NOT NULL REFERENCES devices(id)  ON DELETE CASCADE,
    PRIMARY KEY (routine_id, device_id)
))SQL";

inline constexpr const char* kCreateSettings = R"SQL(
CREATE TABLE IF NOT EXISTS settings (
    key   TEXT PRIMARY KEY,
    value TEXT NOT NULL
))SQL";

/// Orden de creación (respeta las claves foráneas).
inline constexpr const char* kSchema[] = {
    kCreateUsers,        kCreateDevices,       kCreateEvents,         kCreateEventsTsIdx,
    kCreateEventsDeviceIdx, kCreateSessions,   kCreateSessionsIdx,    kCreateRoutines,
    kCreateRoutineDevices,  kCreateSettings};

// ------------------------------------------------------------------ Usuarios
inline constexpr const char* kInsertUser = R"SQL(
INSERT INTO users (username, salt, password_hash, created_at)
VALUES (:username, :salt, :password_hash, :created_at))SQL";

/// Columnas: id, username, salt, password_hash, created_at
inline constexpr const char* kSelectUser = R"SQL(
SELECT id, username, salt, password_hash, created_at FROM users WHERE username = :username)SQL";

inline constexpr const char* kCountUsers = "SELECT COUNT(*) FROM users";

// ------------------------------------------------------------------ Dispositivos
inline constexpr const char* kInsertDevice = R"SQL(
INSERT INTO devices (name, room, type, state, power_w, daily_hours, level)
VALUES (:name, :room, :type, :state, :power_w, :daily_hours, :level))SQL";

inline constexpr const char* kUpdateDevice = R"SQL(
UPDATE devices
SET name = :name, room = :room, type = :type, state = :state,
    power_w = :power_w, daily_hours = :daily_hours, level = :level
WHERE id = :id)SQL";

inline constexpr const char* kDeleteDevice = "DELETE FROM devices WHERE id = :id";

/// Columnas: id, name, room, type, state, power_w, daily_hours, level
inline constexpr const char* kSelectDevice = R"SQL(
SELECT id, name, room, type, state, power_w, daily_hours, level FROM devices WHERE id = :id)SQL";

inline constexpr const char* kSelectDevices = R"SQL(
SELECT id, name, room, type, state, power_w, daily_hours, level FROM devices
ORDER BY name COLLATE NOCASE)SQL";

inline constexpr const char* kDeviceNameExists = R"SQL(
SELECT COUNT(*) FROM devices WHERE name = :name AND id <> :exclude_id)SQL";

// ------------------------------------------------------------------ Eventos
inline constexpr const char* kInsertEvent = R"SQL(
INSERT INTO events (ts, device_id, device_name, kind, message, username)
VALUES (:ts, :device_id, :device_name, :kind, :message, :username))SQL";

/// Columnas: id, ts, device_id (0 si NULL), device_name, kind, message, username
inline constexpr const char* kSelectEvents = R"SQL(
WITH f (dev, kind) AS (SELECT :device_id, :kind)
SELECT e.id, e.ts, COALESCE(e.device_id, 0), e.device_name, e.kind, e.message, e.username
FROM events e, f
WHERE (f.dev = 0 OR e.device_id = f.dev) AND (f.kind = '' OR e.kind = f.kind)
ORDER BY e.ts DESC, e.id DESC
LIMIT :limit)SQL";

// ------------------------------------------------------------------ Sesiones de encendido
inline constexpr const char* kOpenSession =
    "INSERT INTO sessions (device_id, start_ts) VALUES (:device_id, :ts)";

inline constexpr const char* kCloseSession =
    "UPDATE sessions SET end_ts = :ts WHERE device_id = :device_id AND end_ts IS NULL";

/// Columnas: id, name, power_w, segundos encendido dentro de [from, now]
inline constexpr const char* kUsage = R"SQL(
WITH p (now_ts, from_ts) AS (SELECT :now, :from)
SELECT d.id, d.name, d.power_w,
       COALESCE(SUM(MAX(0, MIN(COALESCE(s.end_ts, p.now_ts), p.now_ts) - MAX(s.start_ts, p.from_ts))), 0) AS secs
FROM p CROSS JOIN devices d
LEFT JOIN sessions s ON s.device_id = d.id AND COALESCE(s.end_ts, p.now_ts) > p.from_ts
GROUP BY d.id, d.name, d.power_w
ORDER BY d.name COLLATE NOCASE)SQL";

// ------------------------------------------------------------------ Rutinas
inline constexpr const char* kInsertRoutine = R"SQL(
INSERT INTO routines (name, minute_of_day, action, days_mask, enabled)
VALUES (:name, :minute_of_day, :action, :days_mask, :enabled))SQL";

inline constexpr const char* kUpdateRoutine = R"SQL(
UPDATE routines
SET name = :name, minute_of_day = :minute_of_day, action = :action,
    days_mask = :days_mask, enabled = :enabled
WHERE id = :id)SQL";

inline constexpr const char* kDeleteRoutine = "DELETE FROM routines WHERE id = :id";

/// Columnas: id, name, minute_of_day, action, days_mask, enabled
inline constexpr const char* kSelectRoutines = R"SQL(
SELECT id, name, minute_of_day, action, days_mask, enabled FROM routines
ORDER BY minute_of_day, name COLLATE NOCASE)SQL";

inline constexpr const char* kInsertRoutineDevice =
    "INSERT OR IGNORE INTO routine_devices (routine_id, device_id) VALUES (:routine_id, :device_id)";

inline constexpr const char* kDeleteRoutineDevices = "DELETE FROM routine_devices WHERE routine_id = :routine_id";

/// Columnas: routine_id, device_id
inline constexpr const char* kSelectRoutineDevices =
    "SELECT routine_id, device_id FROM routine_devices ORDER BY routine_id, device_id";

// ------------------------------------------------------------------ Configuración
inline constexpr const char* kGetSetting = "SELECT value FROM settings WHERE key = :key";
inline constexpr const char* kSetSetting = "INSERT OR REPLACE INTO settings (key, value) VALUES (:key, :value)";

}  // namespace sql
}  // namespace domotica
