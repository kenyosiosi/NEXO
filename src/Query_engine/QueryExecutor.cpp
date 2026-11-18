#include "../../include/query_engine/QueryExecutor.h"
#include "../../include/Storage/SchemaManager.h"
#include <filesystem>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;

QueryExecutor::QueryExecutor(std::map<std::string, StorageManager*>& storage, 
                             std::map<std::string, std::map<std::string, IndexManager*>>& indices,
                             std::map<std::string, size_t>& sucios)
    : storage_units(storage), index_map(indices), registros_sucios(sucios) {}

std::string QueryExecutor::formatRecord(const std::map<std::string, std::string>& record) {
    if (record.empty()) return "{ }";
    std::string out = "{ ";
    for (auto const& [k, v] : record) out += "\"" + k + "\": \"" + v + "\", ";
    if (out.length() > 2) out.erase(out.length() - 2);
    return out + " }";
}

std::string QueryExecutor::obtenerReporteMetricas(const std::string& target) {
    std::stringstream ss;
    std::string filepath = "data/" + target + ".bin";
    
    auto live_records = storage_units[target]->scanAll();
    size_t vivos = live_records.size();
    size_t sucios = registros_sucios[target];
    size_t totales = vivos + sucios;
    
    double ratio = (totales > 0) ? (static_cast<double>(sucios) / totales) * 100.0 : 0.0;
    size_t file_size = fs::exists(filepath) ? fs::file_size(filepath) : 0;

    ss << "\n--- MÉTRICAS DE " << target << " ---\n";
    ss << " Registros Vivos:    " << vivos << "\n";
    ss << " Registros Sucios:   " << sucios << "\n";
    ss << " Fragmentación:      " << ratio << " %\n";
    ss << " Tamaño en disco:    " << file_size << " bytes";

    if (vivos >= 100 && ratio >= 30.0) {
        ss << "\n[ALERTA] Índice de fragmentación crítico. Sugerencia: COMPACT.";
    }
    return ss.str();
}

bool QueryExecutor::findLiveRecord(const std::string& target, const std::string& field,
                                   const std::string& value, RecordPointer& out) {
    // UPDATE y DELETE dejan atrás entradas de índice que apuntan a registros
    // con tombstone, así que recorremos todos los hits del índice y devolvemos
    // el primero que todavía esté vivo. La auto-compactación purga lo demás.
    for (const RecordPointer& ptr : index_map[target][field]->search(value)) {
        if (!storage_units[target]->getRecord(ptr).empty()) {
            out = ptr;
            return true;
        }
    }
    return false;
}

std::string QueryExecutor::execute(std::map<std::string, std::string> query_ast) {
    if (query_ast.empty() || query_ast.find("operation") == query_ast.end()) {
        return "[Error] Consulta vacía o mal formada.";
    }

    std::string op = query_ast["operation"];
    std::string target = query_ast["_table_target"];

    // 1. CREATE
    if (op == "CREATE") {
        if (storage_units.count(target)) return "[Error] La colección '" + target + "' ya existe.";
        if (!validateCollectionName(target)) return "[Error] Nombre de colección inválido: usa solo letras, números, '_' o '-'.";

        std::string user_fields = query_ast.count("_fields") ? query_ast["_fields"] : "";
        std::string full_fields = "id";
        if (!user_fields.empty()) full_fields += "," + user_fields;

        saveSchema(target, 1, full_fields);

        storage_units[target] = new StorageManager("data/" + target + ".bin");
        index_map[target]["id"] = new IndexManager("data/" + target + "_id.idx", 3);
        registros_sucios[target] = 0;

        return "Colección '" + target + "' creada con éxito. Índice 'id' autogenerado.";
    }

    if (storage_units.find(target) == storage_units.end()) {
        return "[Error] La colección '" + target + "' no existe.";
    }

    // 2. DEFINE
    if (op == "DEFINE") {
        const SchemaInfo schema = loadSchema(target);
        if (schema.fields.empty()) return "[Error] No se encontró el esquema de '" + target + "'.";

        std::string index_list = "";
        for(auto const& [idx_name, idx_ptr] : index_map[target]) index_list += idx_name + ", ";
        if (!index_list.empty()) index_list.erase(index_list.length() - 2);

        return "Estructura de '" + target + "':\nCampos: " + schema.fields +
               "\nSiguiente ID: " + std::to_string(schema.nextId) +
               "\nÍndices Activos: [" + index_list + "]";
    }

    // 3. INDEX
    if (op == "INDEX") {
        std::string field = query_ast["field"];
        if (index_map[target].count(field)) return "[Error] El índice para '" + field + "' ya existe.";
        
        index_map[target][field] = new IndexManager("data/" + target + "_" + field + ".idx", 3);
        auto records = storage_units[target]->scanAll();
        for (auto& row : records) {
            if (row.second.count(field)) {
                index_map[target][field]->insert(row.second[field], row.first);
            }
        }
        return "Índice secundario sobre '" + field + "' creado exitosamente.";
    }

    // 4. DROP
    if (op == "DROP") {
        delete storage_units[target];
        storage_units.erase(target);
        
        for (auto& [field, mgr] : index_map[target]) {
            delete mgr;
            fs::remove("data/" + target + "_" + field + ".idx");
        }
        index_map.erase(target);
        registros_sucios.erase(target);

        fs::remove("data/" + target + ".bin");
        fs::remove("data/" + target + ".schema");

        return "Colección '" + target + "' eliminada por completo.";
    }

    // 5. STATS
    if (op == "STATS") {
        return obtenerReporteMetricas(target);
    }

    // 6. SCAN
    if (op == "SCAN") {
        auto records = storage_units[target]->scanAll();
        if (records.empty()) return "La colección '" + target + "' está vacía.";
        std::stringstream ss;
        ss << records.size() << " documentos encontrados:\n";
        for (auto& row : records) ss << "  " << formatRecord(row.second) << "\n";
        return ss.str();
    } 
    
    // 7. INSERT
    else if (op == "INSERT") {
        std::map<std::string, std::string> payload = query_ast;
        payload.erase("operation");
        payload.erase("_table_target");

        const SchemaInfo schema = loadSchema(target);
        int nextId = schema.nextId;

        // LÓGICA INTELIGENTE: Si el usuario forzó un ID, lo respetamos y actualizamos la secuencia
        if (payload.count("id")) {
            try {
                const int provided = std::stoi(payload["id"]);
                if (provided >= nextId) nextId = provided + 1;
            } catch(...) {
                payload["id"] = std::to_string(nextId);
                nextId++;
            }
        } else {
            payload["id"] = std::to_string(nextId);
            nextId++;
        }

        RecordPointer ptr = storage_units[target]->insertRecord(payload);
        for (auto const& [field, idx] : index_map[target]) {
            if (payload.count(field)) idx->insert(payload[field], ptr);
        }

        saveSchema(target, nextId, schema.fields);
        
        return "Ok. Registro insertado exitosamente con ID: " + payload["id"];
    }

    // 8. GET
    else if (op == "GET") {
        std::string search_field = "";
        std::string search_value = "";
        bool index_found = false;

        // Buscamos entre los campos de la consulta uno que tenga un índice
        for (auto const& [field, value] : query_ast) {
            // Ignorar metadatos internos del motor
            if (field == "operation" || field == "_table_target") continue;

            if (index_map[target].count(field)) {
                search_field = field;
                search_value = value;
                index_found = true;
                break; // Usamos el primer índice que coincida
            }
        }

        if (!index_found) {
            return "[Error] No se encontró un índice para los campos proporcionados. "
                   "Crea uno con: INDEX \"campo\" ON \"" + target + "\"";
        }

        // Realizar la búsqueda usando el índice dinámico encontrado
        RecordPointer ptr;
        if (findLiveRecord(target, search_field, search_value, ptr)) {
            std::map<std::string, std::string> record = storage_units[target]->getRecord(ptr);
            return "Documento encontrado (vía índice '" + search_field + "'):\n" + formatRecord(record);
        }
        
        return "Documento no encontrado con " + search_field + ": '" + search_value + "'.";
    }

    // 9. DELETE
    else if (op == "DELETE") {
        if (!query_ast.count("id")) return "[Error] DELETE requiere especificar el 'id'.";
        
        RecordPointer ptr;
        if (findLiveRecord(target, "id", query_ast["id"], ptr)) {
            storage_units[target]->logicalDelete(ptr);
            registros_sucios[target]++; 
            return "Documento (ID: " + query_ast["id"] + ") eliminado lógicamente.";
        }
        return "No se encontró el documento para eliminar.";
    }

    // 10. UPDATE
    else if (op == "UPDATE") {
        if (!query_ast.count("id")) return "[Error] UPDATE requiere un 'id' válido.";
        
        RecordPointer ptr;
        if (!findLiveRecord(target, "id", query_ast["id"], ptr)) {
            return "No se encontró el documento a modificar.";
        }

        std::map<std::string, std::string> data = storage_units[target]->getRecord(ptr);
        if (data.empty()) return "El documento ya fue eliminado previamente.";

        for (auto const& [key, val] : query_ast) {
            if (key.substr(0, 5) == "_set_") data[key.substr(5)] = val;
        }

        storage_units[target]->logicalDelete(ptr);
        registros_sucios[target]++; 

        RecordPointer new_ptr = storage_units[target]->insertRecord(data);
        for (auto const& [field, idx] : index_map[target]) {
            if (data.count(field)) idx->insert(data[field], new_ptr); 
        }

        return "Documento actualizado correctamente.";
    }

    return "[Error] Operación no soportada: " + op;
}