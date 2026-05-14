#include <iostream>
#include <vector>
#include <map>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <set>
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

// Función para limpiar físicamente el archivo y reconstruir índices
void autoCompact(std::string tableName, std::map<std::string, StorageManager*>& storage_units, std::map<std::string, std::map<std::string, IndexManager*>>& index_map) {
    std::cout << "\n[SISTEMA] Iniciando Deep-Compactado para '" << tableName << "'..." << std::endl;
    
    // 1. Respaldar qué índices existían antes de borrar
    std::vector<std::string> active_fields;
    for (auto const& [field, mgr] : index_map[tableName]) {
        active_fields.push_back(field);
    }

    // 2. Obtener registros vivos (Streaming)
    auto live_records = storage_units[tableName]->scanAll(); 
    
    // 3. Limpiar memoria: Cerrar Storage e Índices
    delete storage_units[tableName];
    for (auto const& field : active_fields) {
        delete index_map[tableName][field];
    }
    index_map[tableName].clear();

    // 4. Borrar archivos físicos viejos (.bin e .idx)
    fs::remove("data/" + tableName + ".bin");
    for (const auto& field : active_fields) {
        fs::remove("data/" + tableName + "_" + field + ".idx");
    }

    // 5. Reiniciar Storage y recrear TODOS los índices en blanco
    storage_units[tableName] = new StorageManager("data/" + tableName + ".bin");
    for (const auto& field : active_fields) {
        index_map[tableName][field] = new IndexManager("data/" + tableName + "_" + field + ".idx", 3);
    }
    
    // 6. REINSERCIÓN MASIVA: Aquí ocurre la magia
    for (auto& [id, data] : live_records) {
        // Nuevo puntero en el archivo limpio
        RecordPointer new_ptr = storage_units[tableName]->insertRecord(data);
        
        // Actualizar cada índice activo con la nueva dirección
        for (const auto& field : active_fields) {
            if (data.count(field)) {
                index_map[tableName][field]->insert(data[field], new_ptr);
            }
        }
    }
    
    std::cout << "[SISTEMA] " << active_fields.size() << " Indices reconstruidos exitosamente." << std::endl;
    std::cout << "[SISTEMA] Compactado completado.\n" << std::endl;
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

    std::map<std::string, int> registros_sucios; // Cada tabla tiene su contador de basura
    const double UMBRAL_FRAGMENTACION = 30.0; // Compactar cuando la basura supere el 30%

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
                std::cout << "Coleccion '" << tableName << "' creada exitosamente." << std::endl;   
            }
            else if (op == "CREATE_INDEX") {
                if (!storage_units.count(tableName)) throw std::runtime_error("Tabla no existe.");
                
                std::string field = queryData["field"];
                if (field.empty()) throw std::runtime_error("Debe especificar un campo.");
                if (index_map[tableName].count(field)) throw std::runtime_error("El indice ya existe.");

                // --- SOLUCIÓN AL BUG: VALIDACIÓN DE SCHEMA ---
                SchemaInfo info = loadSchema(tableName);
                bool campoValido = false;

                // El ID siempre es indexable por defecto
                if (field == "id") campoValido = true;
                else {
                    std::stringstream ss(info.fields);
                    std::string f;
                    while (std::getline(ss, f, ',')) {
                        // Limpieza básica de espacios
                        f.erase(0, f.find_first_not_of(" "));
                        f.erase(f.find_last_not_of(" ") + 1);
                        if (f == field) { campoValido = true; break; }
                    }
                }

                if (!campoValido) {
                    throw std::runtime_error("El campo '" + field + "' no existe en el esquema de '" + tableName + "'.");
                }

                // Si pasa la validación, procedemos a crear el archivo físico
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
                std::cout << "Indice '" << field << "' creado exitosamente para '" << tableName << "'." << std::endl;
            }
            else if (op == "INSERT") {
                if (!storage_units.count(tableName)) throw std::runtime_error("Tabla no existe.");
                
                queryData.erase("operation"); 
                queryData.erase("_table_target");
                
                SchemaInfo info = loadSchema(tableName);
                
                // --- 1. VALIDACIÓN DE CAMPOS EXTRA ---
                // Creamos un set con los campos permitidos para búsqueda rápida
                std::set<std::string> camposPermitidos = {"id"};
                std::stringstream ss(info.fields);
                std::string f;
                while (std::getline(ss, f, ',')) {
                    f.erase(0, f.find_first_not_of(" "));
                    f.erase(f.find_last_not_of(" ") + 1);
                    if (!f.empty()) camposPermitidos.insert(f);
                }

                // Revisamos si el usuario envió algo que no está en el esquema
                for (auto const& [key, val] : queryData) {
                    if (camposPermitidos.find(key) == camposPermitidos.end()) {
                        throw std::runtime_error("El campo '" + key + "' no pertenece al esquema de la coleccion.");
                    }
                }
                // ------------------------------------------------

                queryData["id"] = std::to_string(info.nextId);
                
                // Validar que no falten campos obligatorios (tu función actual)
                validateSchema(tableName, queryData); 
                
                RecordPointer ptr = storage_units[tableName]->insertRecord(queryData);
                
                for (auto const& [idx_field, mgr] : index_map[tableName]) {
                    if (queryData.count(idx_field)) {
                        mgr->insert(queryData[idx_field], ptr);
                    }
                }
                
                info.nextId++; 
                saveSchema(tableName, info);
                std::cout << "Insertado exitosamente con ID: " << queryData["id"] << std::endl;
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
                    std::cout << "Error: No se encontro un indice secundario para esta busqueda." << std::endl;
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
                    registros_sucios[tableName] += results.size(); // <--- Sumamos el total real
                    std::cout << "Borrados " << results.size() << " registros." << std::endl;
                } else {
                    std::cout << "Error: No se encontro un indice secundario para esta busqueda." << std::endl;
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
                        
                        // Aplicar cambios
                        for (auto const& [k, v] : queryData) {
                            if (k.find("_set_") == 0) old_rec[k.substr(5)] = v;
                        }

                        // El proceso de UPDATE es: borrar lógicamente el viejo e insertar uno nuevo
                        storage_units[tableName]->logicalDelete(ptr); 
                        RecordPointer new_ptr = storage_units[tableName]->insertRecord(old_rec); 
                        
                        // Actualizar índices
                        for (auto const& [idx_field, mgr] : index_map[tableName]) {
                            if (old_rec.count(idx_field)) mgr->insert(old_rec[idx_field], new_ptr);
                        }
                        actualizados++;
                    }

                    // Sumamos la cantidad REAL de registros obsoletos creados
                    registros_sucios[tableName] += actualizados; 

                    std::cout << "Actualizados " << actualizados << " registros." << std::endl;
                } else {
                    std::cout << "Error: No se encontro un indice secundario para esta busqueda." << std::endl;
                }
            } 
            else if (op == "SCAN") {
                if (!storage_units.count(tableName)) {
                    throw std::runtime_error("La coleccion '" + tableName + "' no existe.");
                }

                auto all_records = storage_units[tableName]->scanAll();
                
                if (all_records.empty()) {
                    std::cout << "La coleccion '" << tableName << "' esta vacia." << std::endl;
                } else {
                    std::cout << "\n=== Registros en '" << tableName << "' ===" << std::endl;
                    int count = 0;
                    for (auto const& item : all_records) {
                        // item.second es el map con los datos del registro
                        std::cout << "[ Registro " << ++count << " ]: ";
                        bool first = true;
                        for (auto const& [k, v] : item.second) {
                            if (!first) std::cout << " | ";
                            std::cout << k << ": " << v;
                            first = false;
                        }
                        std::cout << std::endl;
                    }
                    std::cout << "======================================" << std::endl;
                    std::cout << "Total: " << count << " registro(s)." << std::endl;
                }
            }   else if (op == "DROP") {
                std::cout << "¡ADVERTENCIA! Esta a punto de eliminar la coleccion '" << tableName << "'." << std::endl;
                std::cout << "¿Seguro de continuar? (Y/N): ";
                std::string confirm;
                std::getline(std::cin, confirm);

                if (confirm == "Y" || confirm == "y") {
                    // 1. Limpiar memoria RAM
                    if (storage_units.count(tableName)) {
                        delete storage_units[tableName];
                        storage_units.erase(tableName);
                    }
                    if (index_map.count(tableName)) {
                        // Borrar cada IndexManager individualmente antes de borrar el mapa de la tabla
                        for (auto& [field, mgr] : index_map[tableName]) {
                            delete mgr;
                        }
                        index_map.erase(tableName);
                    }

                    // 2. Borrado de archivos físicos
                    fs::remove("data/" + tableName + ".bin");
                    fs::remove("data/" + tableName + ".schema");

                    // Borrar dinámicamente cualquier índice (.idx) que empiece con el nombre de la tabla
                    for (const auto& entry : fs::directory_iterator("data")) {
                        std::string fName = entry.path().filename().string();
                        // Si el archivo empieza por "tabla_" y termina en ".idx"
                        if (fName.find(tableName + "_") == 0 && fName.find(".idx") != std::string::npos) {
                            fs::remove(entry.path());
                        }
                    }
                    std::cout << "Coleccion '" << tableName << "' y todos sus indices eliminados con exito." << std::endl;
                } else {
                    std::cout << "Operacion cancelada por el usuario." << std::endl;
                }
            }   else if (op == "DEFINE") {
                SchemaInfo info = loadSchema(tableName);
                // Verificamos si el archivo de esquema existe físicamente
                if (!fs::exists("data/" + tableName + ".schema")) {
                    std::cout << "Error: La coleccion '" << tableName << "' no existe." << std::endl;
                } else {
                    std::cout << "\n--- ESQUEMA DE COLECCION: " << tableName << " ---" << std::endl;
                    std::cout << "Campos: " << (info.fields.empty() ? "Sin campos definidos" : info.fields) << std::endl;
                    std::cout << "Ultimo ID asignado: " << (info.nextId - 1) << std::endl;
                    std::cout << "Proximo ID disponible: " << info.nextId << std::endl;
                    
                    // Mostrar índices activos
                    std::cout << "Indices activos: ";
                    bool hasIndex = false;
                    for (const auto& entry : fs::directory_iterator("data")) {
                        std::string fName = entry.path().filename().string();
                        if (fName.find(tableName + "_") == 0 && fName.find(".idx") != std::string::npos) {
                            size_t start = tableName.length() + 1;
                            size_t end = fName.find(".idx");
                            std::cout << "[" << fName.substr(start, end - start) << "] ";
                            hasIndex = true;
                        }
                    }
                    if (!hasIndex) std::cout << "Solo ID (Default)";
                    std::cout << "\n--------------------------------------------" << std::endl;
                }
            }  else if (op == "STATS") {
                if (!storage_units.count(tableName)) throw std::runtime_error("Tabla no existe.");

                std::string path = "data/" + tableName + ".bin";
                long long file_size = fs::exists(path) ? fs::file_size(path) : 0;

                // 1. Obtener métricas actuales de la tabla
                auto live_records = storage_units[tableName]->scanAll();
                size_t vivos = live_records.size();
                size_t sucios = registros_sucios[tableName];

                // 2. Calcular ratio
                double ratio = (vivos + sucios > 0) ? (static_cast<double>(sucios) / (vivos + sucios)) * 100.0 : 0;

                std::cout << "\n--- ESTADISTICAS DE: " << tableName << " ---" << std::endl;
                std::cout << "Registros activos: " << vivos << std::endl;
                std::cout << "Registros sucios:  " << sucios << std::endl;
                std::cout << "Fragmentacion:     " << ratio << "% (Limite: " << UMBRAL_FRAGMENTACION << "%)" << std::endl;
                std::cout << "Tamaño en disco:   " << file_size << " bytes" << std::endl;
                
                if (vivos + sucios > 0) {
                    std::cout << "Promedio estimado por registro: " << (file_size / (vivos + sucios)) << " bytes" << std::endl;
                }
                
                std::cout << "--------------------------------------" << std::endl;
            }
            
              // --- LÓGICA DE AUTO-COMPACTADO ---
            if (op == "DELETE" || op == "UPDATE") {
                
                // 1. Obtener métricas reales
                auto live_records = storage_units[tableName]->scanAll();
                size_t vivos = live_records.size();
                size_t sucios = registros_sucios[tableName];
                size_t totales = vivos + sucios;

                // 2. Calcular porcentaje de fragmentación
                double ratio = (totales > 0) ? (static_cast<double>(sucios) / totales) * 100.0 : 0;

                // 3. Disparar si supera el 30% y hay una base mínima (ej. 2 registros)
                if (vivos >= 100 && ratio >= UMBRAL_FRAGMENTACION) { 
                    autoCompact(tableName, storage_units, index_map);
                    registros_sucios[tableName] = 0; // Reiniciar contador de esta tabla
                }
            }
            
        } catch (const std::exception& e) {
            std::cout << "Error: " << e.what() << std::endl;
        }
    }
    return 0;
}