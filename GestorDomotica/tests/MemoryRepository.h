/**
 * @file MemoryRepository.h
 * @brief Repositorio en memoria usado solo por las pruebas unitarias.
 */
#pragma once

#include <algorithm>
#include <map>

#include "../src/core/IRepository.h"

namespace domotica {

class MemoryRepository : public IRepository {
public:
    bool addUser(const User& u) override {
        if (findUser(u.username)) return false;
        User n = u;
        n.id = ++nextUser_;
        users_.push_back(n);
        return true;
    }
    std::optional<User> findUser(const std::string& name) override {
        for (const auto& u : users_) if (lower(u.username) == lower(name)) return u;
        return std::nullopt;
    }
    int userCount() override { return static_cast<int>(users_.size()); }

    int addDevice(const DeviceRecord& r) override {
        if (deviceNameExists(r.name, 0)) return 0;
        DeviceRecord n = r;
        n.id = ++nextDevice_;
        devices_[n.id] = n;
        return n.id;
    }
    bool updateDevice(const DeviceRecord& r) override {
        if (!devices_.count(r.id)) return false;
        devices_[r.id] = r;
        return true;
    }
    bool removeDevice(int id) override {
        if (!devices_.erase(id)) return false;
        for (auto& e : events_) if (e.deviceId == id) e.deviceId = 0;  // ON DELETE SET NULL
        sessions_.erase(std::remove_if(sessions_.begin(), sessions_.end(),
                                       [id](const Session& s) { return s.deviceId == id; }), sessions_.end());
        for (auto& kv : routines_) {
            auto& v = kv.second.deviceIds;
            v.erase(std::remove(v.begin(), v.end(), id), v.end());
        }
        return true;
    }
    std::optional<DeviceRecord> getDevice(int id) override {
        auto it = devices_.find(id);
        if (it == devices_.end()) return std::nullopt;
        return it->second;
    }
    std::vector<DeviceRecord> listDevices() override {
        std::vector<DeviceRecord> v;
        for (auto& kv : devices_) v.push_back(kv.second);
        std::sort(v.begin(), v.end(), [](const DeviceRecord& a, const DeviceRecord& b) {
            return lower(a.name) < lower(b.name);
        });
        return v;
    }
    bool deviceNameExists(const std::string& name, int excludeId) override {
        for (auto& kv : devices_)
            if (kv.first != excludeId && lower(kv.second.name) == lower(name)) return true;
        return false;
    }

    void addEvent(const Event& e) override {
        Event n = e;
        n.id = ++nextEvent_;
        events_.push_back(n);
    }
    std::vector<Event> listEvents(int limit, int deviceId, const std::string& kind) override {
        std::vector<Event> out;
        for (auto it = events_.rbegin(); it != events_.rend(); ++it) {
            if (deviceId != 0 && it->deviceId != deviceId) continue;
            if (!kind.empty() && it->kind != kind) continue;
            out.push_back(*it);
            if (static_cast<int>(out.size()) >= limit) break;
        }
        return out;
    }

    void openSession(int deviceId, int64_t ts) override { sessions_.push_back({deviceId, ts, -1}); }
    void closeSession(int deviceId, int64_t ts) override {
        for (auto& s : sessions_) if (s.deviceId == deviceId && s.end < 0) s.end = ts;
    }
    std::vector<UsageRow> usage(int64_t from, int64_t now) override {
        std::vector<UsageRow> out;
        for (const auto& d : listDevices()) {
            UsageRow row;
            row.deviceId = d.id;
            row.deviceName = d.name;
            row.powerW = d.powerW;
            for (const auto& s : sessions_) {
                if (s.deviceId != d.id) continue;
                const int64_t end = s.end < 0 ? now : s.end;
                if (end <= from) continue;
                row.seconds += std::max<int64_t>(0, std::min(end, now) - std::max(s.start, from));
            }
            out.push_back(row);
        }
        return out;
    }

    int addRoutine(const Routine& r) override {
        Routine n = r;
        n.id = ++nextRoutine_;
        routines_[n.id] = n;
        return n.id;
    }
    bool updateRoutine(const Routine& r) override {
        if (!routines_.count(r.id)) return false;
        routines_[r.id] = r;
        return true;
    }
    bool removeRoutine(int id) override { return routines_.erase(id) > 0; }
    std::vector<Routine> listRoutines() override {
        std::vector<Routine> v;
        for (auto& kv : routines_) v.push_back(kv.second);
        return v;
    }

    std::string getSetting(const std::string& k, const std::string& def) override {
        auto it = settings_.find(k);
        return it == settings_.end() ? def : it->second;
    }
    void setSetting(const std::string& k, const std::string& v) override { settings_[k] = v; }

private:
    struct Session { int deviceId; int64_t start; int64_t end; };
    static std::string lower(std::string s) {
        for (auto& c : s) if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
        return s;
    }
    std::vector<User> users_;
    std::map<int, DeviceRecord> devices_;
    std::vector<Event> events_;
    std::vector<Session> sessions_;
    std::map<int, Routine> routines_;
    std::map<std::string, std::string> settings_;
    int nextUser_ = 0, nextDevice_ = 0, nextRoutine_ = 0;
    int64_t nextEvent_ = 0;
};

}  // namespace domotica
