#include <iostream>
#include <vector>
#include <map>
#include <fstream>
#include <filesystem>
#include <sstream>
#include "../include/Query_engine/tokenizer.h"
#include "../include/Query_engine/parser.h"
#include "../include/Storage/StorageManager.h"
#include "../include/B-tree/IndexManager.h"

namespace fs = std::filesystem;

struct SchemaInfo { int nextId; std::string fields; };

SchemaInfo loadSchema(const std::string& tableName) {
    std::ifstream f("data/" + tableName + ".schema");
    SchemaInfo info = {1, ""};
    if (f.is_open()) {
        std::string idStr;
        std::getline(f, idStr);
        if (!idStr.empty()) info.nextId = std::stoi(idStr);
        std::getline(f, info.fields);
    }
    return info;
}

void saveSchema(const std::string& tableName, const SchemaInfo& info) {
    std::ofstream f("data/" + tableName + ".schema");
    f << info.nextId << "\n" << info.fields;
}

bool validateSchema(const std::string& tableName, std::map<std::string, std::string>& data) {
    SchemaInfo info = loadSchema(tableName);
    if (info.fields.empty()) return true;

    std::stringstream ss(info.fields);
    std::string field;
    while (std::getline(ss, field, ',')) {
        size_t start = field.find_first_not_of(" \t\r\n\"");
        if (start != std::string::npos) field = field.substr(start);
        size_t end = field.find_last_not_of(" \t\r\n\"");
        if (end != std::string::npos) field = field.substr(0, end + 1);

        if (!field.empty() && data.find(field) == data.end()) {
            throw std::runtime_error("Falta campo obligatorio en la inserción: " + field);
        }
    }
    return true;
}

int main() {
    std::map<std::string, StorageManager*> storage_units;
    std::map<std::string, std::map<std::string, IndexManager*>> index_map;
    fs::create_directories("data");

    for (const auto& entry : fs::directory_iterator("data")) {
        std::string fname = entry.path().filename().string();
        if (fname.find(".bin") != std::string::npos) {
            std::string tName = fname.substr(0, fname.find(".bin"));
            storage_units[tName] = new StorageManager("data/" + fname);
        } else if (fname.find(".idx") != std::string::npos) {
            size_t underscore_pos = fname.find('_');
            size_t dot_pos = fname.find(".idx");
            if (underscore_pos != std::string::npos && dot_pos != std::string::npos) {
                std::string tName = fname.substr(0, underscore_pos);
                std::string field = fname.substr(underscore_pos + 1, dot_pos - underscore_pos - 1);
                index_map[tName][field] = new IndexManager("data/" + fname, 3);
            }
        }
    }

    std::cout << "----------NEXO DB ENGINE----------" << std::endl;

    while (true) {
        std::string input;
        std::cout << "Nexo> "; 
        std::getline(std::cin, input);
        
        if (input.empty()) continue;
        if (input == "exit" || input == "quit") break;

        try {
            Tokenizer tk(input);
            Parser parser(tk.tokenize());
            auto queryData = parser.parse();
            std::string op = queryData["operation"];
            std::string tableName = queryData["_table_target"];

            if (op == "CREATE") {
                if (storage_units.count(tableName) || fs::exists("data/" + tableName + ".bin")) {
                    throw std::runtime_error("Coleccion ya existe.");
                }
                storage_units[tableName] = new StorageManager("data/" + tableName + ".bin");
                index_map[tableName]["id"] = new IndexManager("data/" + tableName + "_id.idx", 3);
                SchemaInfo info = {1, queryData["_fields"]};
                saveSchema(tableName, info);
                std::cout << "Colección '" << tableName << "' creada exitosamente." << std::endl;
            }
            else if (op == "CREATE_INDEX") {
                if (!storage_units.count(tableName)) throw std::runtime_error("Tabla no existe.");
                std::string field = queryData["field"]; 
                if (field.empty() || index_map[tableName].count(field)) throw std::runtime_error("Campo vacio o indice ya existe.");
                
                std::string indexPath = "data/" + tableName + "_" + field + ".idx";
                index_map[tableName][field] = new IndexManager(indexPath, 3);
                
                auto all_records = storage_units[tableName]->scanAll();
                int count = 0;
                for (auto& row : all_records) {
                    if (row.second.count(field)) {
                        index_map[tableName][field]->insert(row.second[field], row.first);
                        count++;
                    }
                }
                std::cout << "Indice '" << field << "' creado. Registros previos indexados: " << count << std::endl;
            }
            else if (op == "INSERT") {
                if (!storage_units.count(tableName)) throw std::runtime_error("Tabla no existe.");
                
                queryData.erase("operation"); queryData.erase("_table_target");
                SchemaInfo info = loadSchema(tableName);
                queryData["id"] = std::to_string(info.nextId);
                
                validateSchema(tableName, queryData); 
                
                RecordPointer ptr = storage_units[tableName]->insertRecord(queryData);
                
                for (auto const& [idx_field, mgr] : index_map[tableName]) {
                    if (queryData.count(idx_field)) {
                        mgr->insert(queryData[idx_field], ptr);
                    }
                }
                
                info.nextId++; saveSchema(tableName, info);
                std::cout << "Insertado con ID: " << queryData["id"] << std::endl;
            }
            else if (op == "GET") {
                if (!storage_units.count(tableName)) throw std::runtime_error("Tabla no existe.");
                std::string searchField = "";
                for (auto const& [f, mgr] : index_map[tableName]) {
                    if (queryData.count(f)) { searchField = f; break; }
                }

                if (!searchField.empty()) {
                    auto results = index_map[tableName][searchField]->search(queryData[searchField]);
                    int validos = 0;
                    for (auto ptr : results) {
                        auto rec = storage_units[tableName]->getRecord(ptr);
                        if (!rec.empty()) { 
                            for (auto const& [k, v] : rec) std::cout << k << ":" << v << " | ";
                            std::cout << std::endl;
                            validos++;
                        }
                    }
                    if (validos == 0) std::cout << "No encontrado." << std::endl;
                } else {
                    std::cout << "Error: No se encontró un índice secundario para esta búsqueda." << std::endl;
                }
            }
            else if (op == "DELETE") {
                if (!storage_units.count(tableName)) throw std::runtime_error("Tabla no existe.");
                std::string searchField = "";
                for (auto const& [f, mgr] : index_map[tableName]) {
                    if (queryData.count(f)) { searchField = f; break; }
                }
                if (!searchField.empty()) {
                    auto results = index_map[tableName][searchField]->search(queryData[searchField]);
                    for (auto ptr : results) storage_units[tableName]->logicalDelete(ptr);
                    std::cout << "Borrados " << results.size() << " registros." << std::endl;
                } else {
                    std::cout << "Error: No se encontró un índice secundario para esta búsqueda." << std::endl;
                }
            }
            else if (op == "UPDATE") {
                if (!storage_units.count(tableName)) throw std::runtime_error("Tabla no existe.");
                std::string searchField = "";
                for (auto const& [f, mgr] : index_map[tableName]) {
                    if (queryData.count(f) && f.find("_set_") == std::string::npos) { searchField = f; break; }
                }
                
                if (!searchField.empty()) {
                    auto results = index_map[tableName][searchField]->search(queryData[searchField]);
                    int actualizados = 0;
                    for (auto ptr : results) {
                        auto old_rec = storage_units[tableName]->getRecord(ptr);
                        if (old_rec.empty()) continue; 
                        
                        for (auto const& [k, v] : queryData) {
                            if (k.find("_set_") == 0) old_rec[k.substr(5)] = v;
                        }
                        storage_units[tableName]->logicalDelete(ptr); 
                        RecordPointer new_ptr = storage_units[tableName]->insertRecord(old_rec); 
                        
                        for (auto const& [idx_field, mgr] : index_map[tableName]) {
                            if (old_rec.count(idx_field)) mgr->insert(old_rec[idx_field], new_ptr);
                        }
                        actualizados++;
                    }
                    std::cout << "Actualizados " << actualizados << " registros." << std::endl;
                } else {
                    std::cout << "Error: No se encontró un índice secundario para esta búsqueda." << std::endl;
                }
            }
        } catch (const std::exception& e) {
            std::cout << "Error: " << e.what() << std::endl;
        }
    }
    return 0;
}