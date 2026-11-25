#include "Time.h"

#include <ctime>

namespace domotica {

namespace {
bool toLocal(int64_t epoch, std::tm& out) {
    const std::time_t t = static_cast<std::time_t>(epoch);
#ifdef _WIN32
    return localtime_s(&out, &t) == 0;
#else
    return localtime_r(&t, &out) != nullptr;
#endif
}
std::string two(int v) { return (v < 10 ? "0" : "") + std::to_string(v); }
}  // namespace

int64_t systemClock() { return static_cast<int64_t>(std::time(nullptr)); }

LocalTime localTimeFrom(int64_t epoch) {
    std::tm tm{};
    toLocal(epoch, tm);
    LocalTime lt;
    lt.minuteOfDay = tm.tm_hour * 60 + tm.tm_min;
    lt.weekday = (tm.tm_wday + 6) % 7;
    lt.dayKey = (tm.tm_year + 1900) * 10000 + (tm.tm_mon + 1) * 100 + tm.tm_mday;
    return lt;
}

std::string formatTimestamp(int64_t epoch) {
    std::tm tm{};
    if (!toLocal(epoch, tm)) return "";
    char buf[32];
    std::strftime(buf, sizeof buf, "%Y-%m-%d %H:%M:%S", &tm);
    return buf;
}

std::string formatMinute(int m) { return two(m / 60) + ":" + two(m % 60); }

std::string daysToString(int mask) {
    static const char* names[7] = {"Lun", "Mar", "Mié", "Jue", "Vie", "Sáb", "Dom"};
    if ((mask & 0x7F) == 0x7F) return "Todos los días";
    std::string out;
    for (int i = 0; i < 7; ++i) {
        if ((mask >> i) & 1) {
            if (!out.empty()) out += ",";
            out += names[i];
        }
    }
    return out;
}

}  // namespace domotica
