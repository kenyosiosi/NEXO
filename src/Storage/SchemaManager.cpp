#include "../include/Storage/SchemaManager.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

bool validateSchemaFields(const std::string& fields) {
    if (fields.empty()) return false;
    return (fields.find_first_of(".;/\\") == std::string::npos);
}

bool validateCollectionName(const std::string& name) {
    if (name.empty()) return false;
    for (const char c : name) {
        const unsigned char u = static_cast<unsigned char>(c);
        if (!(std::isalnum(u) || c == '_' || c == '-')) return false;
    }
    return true;
}

SchemaInfo loadSchema(const std::string& tableName) {
    SchemaInfo info;
    std::ifstream f("data/" + tableName + ".schema");
    if (!f.is_open()) return info;

    std::string idStr;
    std::getline(f, idStr);
    if (!idStr.empty()) {
        try { info.nextId = std::stoi(idStr); } catch (...) { info.nextId = 1; }
    }
    std::getline(f, info.fields);
    f.close();

    std::stringstream ss(info.fields);
    std::string field;
    while (std::getline(ss, field, ',')) {
        const size_t first = field.find_first_not_of(' ');
        if (first == std::string::npos) continue;
        field.erase(0, first);
        while (!field.empty() && field.back() == ' ') field.pop_back();
        if (!field.empty() && field != "id") info.fieldsList.push_back(field);
    }
    return info;
}

void saveSchema(const std::string& tableName, int nextId, const std::string& fields) {
    if (!fs::exists("data")) fs::create_directory("data");
    std::ofstream f("data/" + tableName + ".schema");
    if (f.is_open()) {
        f << nextId << "\n" << fields;
        f.close();
    }
}
