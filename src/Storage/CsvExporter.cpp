#include "Storage/CsvExporter.h"

namespace csv {

std::string escape(const std::string& value) {
    const bool needs_quotes = value.find_first_of(",\"\n\r") != std::string::npos;
    if (!needs_quotes) return value;

    std::string out;
    out.reserve(value.size() + 2);
    out += '"';
    for (char c : value) {
        if (c == '"') out += '"'; // duplicar la comilla
        out += c;
    }
    out += '"';
    return out;
}

std::string build(const std::vector<std::map<std::string, std::string>>& records,
                  const std::vector<std::string>& fields) {
    std::string out;

    for (size_t i = 0; i < fields.size(); ++i) {
        if (i) out += ',';
        out += escape(fields[i]);
    }
    out += '\n';

    for (const auto& record : records) {
        for (size_t i = 0; i < fields.size(); ++i) {
            if (i) out += ',';
            const auto it = record.find(fields[i]);
            if (it != record.end()) out += escape(it->second);
        }
        out += '\n';
    }
    return out;
}

} // namespace csv
