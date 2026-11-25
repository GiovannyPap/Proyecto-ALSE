/**
 * @file SqliteRepository.h
 * @brief Implementación de IRepository sobre SQLite usando QtSql (controlador QSQLITE).
 */
#pragma once

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QString>

#include "../core/IRepository.h"

namespace domotica {

class SqliteRepository : public IRepository {
public:
    /// Abre (o crea) la base de datos en `path` y garantiza el esquema.
    explicit SqliteRepository(const QString& path, const QString& connectionName = QStringLiteral("domotica"));
    ~SqliteRepository() override;
    SqliteRepository(const SqliteRepository&) = delete;
    SqliteRepository& operator=(const SqliteRepository&) = delete;

    bool isOpen() const { return open_; }
    QString lastError() const { return lastError_; }

    bool addUser(const User& user) override;
    std::optional<User> findUser(const std::string& username) override;
    int userCount() override;

    int addDevice(const DeviceRecord& rec) override;
    bool updateDevice(const DeviceRecord& rec) override;
    bool removeDevice(int id) override;
    std::optional<DeviceRecord> getDevice(int id) override;
    std::vector<DeviceRecord> listDevices() override;
    bool deviceNameExists(const std::string& name, int excludeId) override;

    void addEvent(const Event& ev) override;
    std::vector<Event> listEvents(int limit, int deviceId, const std::string& kind) override;

    void openSession(int deviceId, int64_t ts) override;
    void closeSession(int deviceId, int64_t ts) override;
    std::vector<UsageRow> usage(int64_t from, int64_t now) override;

    int addRoutine(const Routine& r) override;
    bool updateRoutine(const Routine& r) override;
    bool removeRoutine(int id) override;
    std::vector<Routine> listRoutines() override;

    std::string getSetting(const std::string& key, const std::string& def) override;
    void setSetting(const std::string& key, const std::string& value) override;

private:
    QSqlDatabase db() const { return QSqlDatabase::database(connName_); }
    QSqlQuery prepare(const char* sql) const;
    bool run(QSqlQuery& q);
    bool exec(const char* sql);
    void replaceRoutineDevices(int routineId, const std::vector<int>& ids);

    QString connName_;
    QString lastError_;
    bool open_ = false;
};

}  // namespace domotica
