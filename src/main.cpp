#include <iostream>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include "../include/query_engine/tokenizer.h"
#include "../include/query_engine/parser.h"
#include "../include/Storage/StorageManager.h"
#include "../include/B-tree/IndexManager.h"

bool fileExists(const std::string& name) {
    std::ifstream f(name.c_str());
    return f.good();
}

struct SchemaInfo {
    int nextId;
    std::string fields;
};

SchemaInfo loadSchema(const std::string& tableName) {
    std::ifstream f("data/" + tableName + ".schema");
    SchemaInfo info = {1, ""};
    if (f.is_open()) {
        std::string idStr;
        std::getline(f, idStr);
        if (!idStr.empty()) info.nextId = std::stoi(idStr);
        std::getline(f, info.fields);
        f.close();
    }
    return info;
}

void saveSchema(const std::string& tableName, const SchemaInfo& info) {
    std::ofstream f("data/" + tableName + ".schema");
    f << info.nextId << "\n" << info.fields;
    f.close();
}

bool validateSchema(const std::string& tableName, std::map<std::string, std::string>& dataToInsert) {
    SchemaInfo info = loadSchema(tableName);
    if (info.fields.empty()) return true; 
    std::stringstream ss(info.fields);
    std::string field;
    while (std::getline(ss, field, ',')) {
        if (!field.empty() && dataToInsert.find(field) == dataToInsert.end()) {
            std::cerr << "Error de Esquema: Falta el campo '" << field << "'" << std::endl;
            return false;
        }
    }
    return true;
}

int main() {
    std::map<std::string, StorageManager*> storage_units;
    std::map<std::string, IndexManager*> index_units;
    std::cout << "--- NEXO DATABASE SYSTEM (Corrected CRUD) ---" << std::endl;

    while (true) {
        std::string input;
        std::cout << "\nNEXO > ";
        std::getline(std::cin, input);
        if (input == "exit") break;
        if (input.empty()) continue;

        try {
            Tokenizer tokenizer(input);
            Parser parser(tokenizer.tokenize());
            std::map<std::string, std::string> queryData = parser.parse();
            std::string op = queryData["operation"];
            std::string tableName = queryData["_table_target"]; 

            if (storage_units.find(tableName) == storage_units.end() && fileExists("data/" + tableName + ".bin")) {
                storage_units[tableName] = new StorageManager("data/"+tableName+".bin", "data/"+tableName+".idx");
                index_units[tableName] = new IndexManager(storage_units[tableName], 3);
            }

            if (op == "CREATE") {
                if (fileExists("data/" + tableName + ".bin")) {
                    std::cout << "Coleccion ya existe." << std::endl;
                } else {
                    storage_units[tableName] = new StorageManager("data/"+tableName+".bin", "data/"+tableName+".idx");
                    index_units[tableName] = new IndexManager(storage_units[tableName], 3);
                    saveSchema(tableName, {1, queryData["_fields"]});
                    std::cout << "Coleccion '" << tableName << "' creada." << std::endl;
                }
            } 
            else if (op == "INSERT") {
                SchemaInfo info = loadSchema(tableName);
                queryData["id"] = std::to_string(info.nextId);
                if (!validateSchema(tableName, queryData)) continue;
                
                RecordPointer ptr = storage_units[tableName]->insertRecord(queryData);
                index_units[tableName]->insert(info.nextId, ptr);
                
                info.nextId++;
                saveSchema(tableName, info);
                std::cout << "Insertado con ID: " << queryData["id"] << std::endl;
            }
            else if (op == "UPDATE") {
                int id = std::stoi(queryData["id"]);
                RecordPointer ptr = index_units[tableName]->search(id);
                if (ptr.page_id != -1) {
                    auto record = storage_units[tableName]->getRecord(ptr);
                    for (auto const& [k, v] : queryData) {
                        if (k != "operation" && k != "id" && k[0] != '_') record[k] = v;
                    }
                    // El nuevo insert sobrescribirá la entrada antigua en el B-Tree
                    RecordPointer nPtr = storage_units[tableName]->insertRecord(record);
                    index_units[tableName]->insert(id, nPtr);
                    std::cout << "Registro " << id << " actualizado." << std::endl;
                }
            }
            else if (op == "GET") {
                int id_buscar = std::stoi(queryData["id"]);
                RecordPointer ptr = index_units[tableName]->search(id_buscar);
                if (ptr.page_id != -1) {
                    auto res = storage_units[tableName]->getRecord(ptr);
                    if (res.count("id") && std::stoi(res["id"]) == id_buscar) {
                        for (auto const& [k, v] : res) {
                            if (k[0] != '_' && k != "operation") {
                                std::cout << k << ": " << v << std::endl;
                            }
                        }
                    } else { std::cout << "Registro eliminado o antiguo." << std::endl; }
                } else { std::cout << "ID no encontrado." << std::endl; }
            }
            else if (op == "DELETE") {
                index_units[tableName]->remove(std::stoi(queryData["id"]));
                std::cout << "Eliminado." << std::endl;
            }
        } catch (const std::exception& e) { std::cerr << "Error: " << e.what() << std::endl; }
    }
    return 0;
}