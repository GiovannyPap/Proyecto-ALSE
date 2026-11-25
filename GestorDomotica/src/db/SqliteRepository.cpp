#include "SqliteRepository.h"

#include <QDebug>
#include <QSqlError>
#include <QVariant>
#include <map>

#include "Sql.h"

namespace domotica {

namespace {
QString qs(const std::string& s) { return QString::fromStdString(s); }
std::string ss(const QString& s) { return s.toStdString(); }

void bindDevice(QSqlQuery& q, const DeviceRecord& r) {
    q.bindValue(":name", qs(r.name));
    q.bindValue(":room", qs(r.room));
    q.bindValue(":type", qs(toString(r.type)));
    q.bindValue(":state", qs(toString(r.state)));
    q.bindValue(":power_w", r.powerW);
    q.bindValue(":daily_hours", r.dailyHours);
    q.bindValue(":level", r.level);
}

/// Columnas: id, name, room, type, state, power_w, daily_hours, level (orden de sql::kSelectDevice).
DeviceRecord deviceFromRow(const QSqlQuery& q) {
    DeviceRecord r;
    r.id = q.value(0).toInt();
    r.name = ss(q.value(1).toString());
    r.room = ss(q.value(2).toString());
    parseDeviceType(ss(q.value(3).toString()), r.type);
    parseDeviceState(ss(q.value(4).toString()), r.state);
    r.powerW = q.value(5).toDouble();
    r.dailyHours = q.value(6).toDouble();
    r.level = q.value(7).toInt();
    return r;
}
}  // namespace

SqliteRepository::SqliteRepository(const QString& path, const QString& connectionName)
    : connName_(connectionName) {
    QSqlDatabase database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connName_);
    database.setDatabaseName(path);
    if (!database.open()) {
        lastError_ = database.lastError().text();
        return;
    }
    open_ = true;
    exec("PRAGMA foreign_keys = ON");
    exec("PRAGMA journal_mode = WAL");
    for (const char* stmt : sql::kSchema) exec(stmt);
}

SqliteRepository::~SqliteRepository() {
    {
        QSqlDatabase database = QSqlDatabase::database(connName_, false);
        if (database.isOpen()) database.close();
    }
    QSqlDatabase::removeDatabase(connName_);
}

QSqlQuery SqliteRepository::prepare(const char* sql) const {
    QSqlQuery q(db());
    q.prepare(QString::fromUtf8(sql));
    return q;
}

bool SqliteRepository::run(QSqlQuery& q) {
    if (!q.exec()) {
        lastError_ = q.lastError().text();
        qWarning() << "SQL error:" << lastError_;
        return false;
    }
    return true;
}

bool SqliteRepository::exec(const char* sql) {
    QSqlQuery q(db());
    if (!q.exec(QString::fromUtf8(sql))) {
        lastError_ = q.lastError().text();
        qWarning() << "SQL error:" << lastError_;
        return false;
    }
    return true;
}

// ------------------------------------------------------------------ Usuarios
bool SqliteRepository::addUser(const User& u) {
    QSqlQuery q = prepare(sql::kInsertUser);
    q.bindValue(":username", qs(u.username));
    q.bindValue(":salt", qs(u.salt));
    q.bindValue(":password_hash", qs(u.passwordHash));
    q.bindValue(":created_at", static_cast<qlonglong>(u.createdAt));
    return run(q);
}

std::optional<User> SqliteRepository::findUser(const std::string& username) {
    QSqlQuery q = prepare(sql::kSelectUser);
    q.bindValue(":username", qs(username));
    if (!run(q) || !q.next()) return std::nullopt;
    User u;
    u.id = q.value(0).toInt();
    u.username = ss(q.value(1).toString());
    u.salt = ss(q.value(2).toString());
    u.passwordHash = ss(q.value(3).toString());
    u.createdAt = q.value(4).toLongLong();
    return u;
}

int SqliteRepository::userCount() {
    QSqlQuery q = prepare(sql::kCountUsers);
    if (!run(q) || !q.next()) return 0;
    return q.value(0).toInt();
}

// ------------------------------------------------------------------ Dispositivos
int SqliteRepository::addDevice(const DeviceRecord& r) {
    QSqlQuery q = prepare(sql::kInsertDevice);
    bindDevice(q, r);
    if (!run(q)) return 0;
    return q.lastInsertId().toInt();
}

bool SqliteRepository::updateDevice(const DeviceRecord& r) {
    QSqlQuery q = prepare(sql::kUpdateDevice);
    bindDevice(q, r);
    q.bindValue(":id", r.id);
    return run(q) && q.numRowsAffected() > 0;
}

bool SqliteRepository::removeDevice(int id) {
    QSqlQuery q = prepare(sql::kDeleteDevice);
    q.bindValue(":id", id);
    return run(q) && q.numRowsAffected() > 0;
}

std::optional<DeviceRecord> SqliteRepository::getDevice(int id) {
    QSqlQuery q = prepare(sql::kSelectDevice);
    q.bindValue(":id", id);
    if (!run(q) || !q.next()) return std::nullopt;
    return deviceFromRow(q);
}

std::vector<DeviceRecord> SqliteRepository::listDevices() {
    std::vector<DeviceRecord> out;
    QSqlQuery q = prepare(sql::kSelectDevices);
    if (!run(q)) return out;
    while (q.next()) out.push_back(deviceFromRow(q));
    return out;
}

bool SqliteRepository::deviceNameExists(const std::string& name, int excludeId) {
    QSqlQuery q = prepare(sql::kDeviceNameExists);
    q.bindValue(":name", qs(name));
    q.bindValue(":exclude_id", excludeId);
    if (!run(q) || !q.next()) return false;
    return q.value(0).toInt() > 0;
}

// ------------------------------------------------------------------ Eventos
void SqliteRepository::addEvent(const Event& e) {
    QSqlQuery q = prepare(sql::kInsertEvent);
    q.bindValue(":ts", static_cast<qlonglong>(e.timestamp));
    q.bindValue(":device_id", e.deviceId > 0 ? QVariant(e.deviceId) : QVariant());  // NULL si no aplica
    q.bindValue(":device_name", qs(e.deviceName));
    q.bindValue(":kind", qs(e.kind));
    q.bindValue(":message", qs(e.message));
    q.bindValue(":username", qs(e.username));
    run(q);
}

std::vector<Event> SqliteRepository::listEvents(int limit, int deviceId, const std::string& kind) {
    std::vector<Event> out;
    QSqlQuery q = prepare(sql::kSelectEvents);
    q.bindValue(":device_id", deviceId);
    q.bindValue(":kind", qs(kind));
    q.bindValue(":limit", limit);
    if (!run(q)) return out;
    while (q.next()) {
        Event e;
        e.id = q.value(0).toLongLong();
        e.timestamp = q.value(1).toLongLong();
        e.deviceId = q.value(2).toInt();
        e.deviceName = ss(q.value(3).toString());
        e.kind = ss(q.value(4).toString());
        e.message = ss(q.value(5).toString());
        e.username = ss(q.value(6).toString());
        out.push_back(e);
    }
    return out;
}

// ------------------------------------------------------------------ Sesiones
void SqliteRepository::openSession(int deviceId, int64_t ts) {
    QSqlQuery q = prepare(sql::kOpenSession);
    q.bindValue(":device_id", deviceId);
    q.bindValue(":ts", static_cast<qlonglong>(ts));
    run(q);
}

void SqliteRepository::closeSession(int deviceId, int64_t ts) {
    QSqlQuery q = prepare(sql::kCloseSession);
    q.bindValue(":ts", static_cast<qlonglong>(ts));
    q.bindValue(":device_id", deviceId);
    run(q);
}

std::vector<UsageRow> SqliteRepository::usage(int64_t from, int64_t now) {
    std::vector<UsageRow> out;
    QSqlQuery q = prepare(sql::kUsage);
    q.bindValue(":now", static_cast<qlonglong>(now));
    q.bindValue(":from", static_cast<qlonglong>(from));
    if (!run(q)) return out;
    while (q.next()) {
        UsageRow u;
        u.deviceId = q.value(0).toInt();
        u.deviceName = ss(q.value(1).toString());
        u.powerW = q.value(2).toDouble();
        u.seconds = q.value(3).toLongLong();
        out.push_back(u);
    }
    return out;
}

// ------------------------------------------------------------------ Rutinas
void SqliteRepository::replaceRoutineDevices(int routineId, const std::vector<int>& ids) {
    QSqlQuery del = prepare(sql::kDeleteRoutineDevices);
    del.bindValue(":routine_id", routineId);
    run(del);
    for (int deviceId : ids) {
        QSqlQuery ins = prepare(sql::kInsertRoutineDevice);
        ins.bindValue(":routine_id", routineId);
        ins.bindValue(":device_id", deviceId);
        run(ins);
    }
}

static void bindRoutine(QSqlQuery& q, const Routine& r) {
    q.bindValue(":name", qs(r.name));
    q.bindValue(":minute_of_day", r.minuteOfDay);
    q.bindValue(":action", r.action == RoutineAction::TurnOn ? QStringLiteral("on") : QStringLiteral("off"));
    q.bindValue(":days_mask", r.daysMask);
    q.bindValue(":enabled", r.enabled ? 1 : 0);
}

int SqliteRepository::addRoutine(const Routine& r) {
    db().transaction();
    QSqlQuery q = prepare(sql::kInsertRoutine);
    bindRoutine(q, r);
    if (!run(q)) {
        db().rollback();
        return 0;
    }
    const int id = q.lastInsertId().toInt();
    replaceRoutineDevices(id, r.deviceIds);
    db().commit();
    return id;
}

bool SqliteRepository::updateRoutine(const Routine& r) {
    db().transaction();
    QSqlQuery q = prepare(sql::kUpdateRoutine);
    bindRoutine(q, r);
    q.bindValue(":id", r.id);
    if (!run(q) || q.numRowsAffected() <= 0) {
        db().rollback();
        return false;
    }
    replaceRoutineDevices(r.id, r.deviceIds);
    return db().commit();
}

bool SqliteRepository::removeRoutine(int id) {
    QSqlQuery q = prepare(sql::kDeleteRoutine);
    q.bindValue(":id", id);
    return run(q) && q.numRowsAffected() > 0;
}

std::vector<Routine> SqliteRepository::listRoutines() {
    std::vector<Routine> out;
    QSqlQuery q = prepare(sql::kSelectRoutines);
    if (!run(q)) return out;
    while (q.next()) {
        Routine r;
        r.id = q.value(0).toInt();
        r.name = ss(q.value(1).toString());
        r.minuteOfDay = q.value(2).toInt();
        r.action = q.value(3).toString() == QLatin1String("off") ? RoutineAction::TurnOff : RoutineAction::TurnOn;
        r.daysMask = q.value(4).toInt();
        r.enabled = q.value(5).toInt() != 0;
        out.push_back(r);
    }
    QSqlQuery links = prepare(sql::kSelectRoutineDevices);
    if (!run(links)) return out;
    std::map<int, std::vector<int>> byRoutine;
    while (links.next()) byRoutine[links.value(0).toInt()].push_back(links.value(1).toInt());
    for (Routine& r : out) r.deviceIds = byRoutine[r.id];
    return out;
}

// ------------------------------------------------------------------ Configuración
std::string SqliteRepository::getSetting(const std::string& key, const std::string& def) {
    QSqlQuery q = prepare(sql::kGetSetting);
    q.bindValue(":key", qs(key));
    if (!run(q) || !q.next()) return def;
    return ss(q.value(0).toString());
}

void SqliteRepository::setSetting(const std::string& key, const std::string& value) {
    QSqlQuery q = prepare(sql::kSetSetting);
    q.bindValue(":key", qs(key));
    q.bindValue(":value", qs(value));
    run(q);
}

}  // namespace domotica
