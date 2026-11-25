#include "Report.h"

#include "Time.h"

namespace domotica {

namespace {
std::string field(const std::string& s) {
    std::string out = "\"";
    for (char c : s) {
        if (c == '"') out += "\"\"";
        else out += c;
    }
    return out + "\"";
}
}  // namespace

std::string eventsToCsv(const std::vector<Event>& events) {
    std::string out = "Fecha y hora,Dispositivo,Tipo de evento,Detalle,Usuario\n";
    for (const Event& e : events) {
        out += field(formatTimestamp(e.timestamp)) + "," + field(e.deviceName) + "," + field(e.kind) + "," +
               field(e.message) + "," + field(e.username) + "\n";
    }
    return out;
}

}  // namespace domotica
