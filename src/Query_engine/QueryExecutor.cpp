#include "../../include/query_engine/QueryExecutor.h"
#include "../../include/Storage/SchemaManager.h"
#include "Core/Messages.h"
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
    using nexo::msg;
    using nexo::Msg;
    std::stringstream ss;
    std::string filepath = "data/" + target + ".bin";
    
    auto live_records = storage_units[target]->scanAll();
    size_t vivos = live_records.size();
    size_t sucios = registros_sucios[target];
    size_t totales = vivos + sucios;
    
    double ratio = (totales > 0) ? (static_cast<double>(sucios) / totales) * 100.0 : 0.0;
    size_t file_size = fs::exists(filepath) ? fs::file_size(filepath) : 0;

    char ratio_buf[32];
    snprintf(ratio_buf, sizeof(ratio_buf), "%.6g", ratio);

    ss << msg(Msg::MetricsTitle, target)
       << msg(Msg::MetricsLive, std::to_string(vivos))
       << msg(Msg::MetricsDead, std::to_string(sucios))
       << msg(Msg::MetricsFrag, ratio_buf)
       << msg(Msg::MetricsSize, std::to_string(file_size));

    if (vivos >= 100 && ratio >= 30.0) {
        ss << msg(Msg::MetricsAlert);
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
    using nexo::msg;
    using nexo::Msg;
    if (query_ast.empty() || query_ast.find("operation") == query_ast.end()) {
        return msg(Msg::EmptyQuery);
    }

    std::string op = query_ast["operation"];
    std::string target = query_ast["_table_target"];

    // 1. CREATE
    if (op == "CREATE") {
        if (storage_units.count(target)) return msg(Msg::CollectionExists, target);
        if (!validateCollectionName(target)) return msg(Msg::InvalidName);

        std::string user_fields = query_ast.count("_fields") ? query_ast["_fields"] : "";
        std::string full_fields = "id";
        if (!user_fields.empty()) full_fields += "," + user_fields;

        saveSchema(target, 1, full_fields);

        storage_units[target] = new StorageManager("data/" + target + ".bin");
        index_map[target]["id"] = new IndexManager("data/" + target + "_id.idx", 3);
        registros_sucios[target] = 0;

        return msg(Msg::CollectionCreated, target);
    }

    if (storage_units.find(target) == storage_units.end()) {
        return msg(Msg::CollectionNotExists, target);
    }

    // 2. DEFINE
    if (op == "DEFINE") {
        const SchemaInfo schema = loadSchema(target);
        if (schema.fields.empty()) return msg(Msg::SchemaNotFound, target);

        std::string index_list = "";
        for(auto const& [idx_name, idx_ptr] : index_map[target]) index_list += idx_name + ", ";
        if (!index_list.empty()) index_list.erase(index_list.length() - 2);

        return msg(Msg::DefineReport, target, schema.fields,
                   std::to_string(schema.nextId), index_list);
    }

    // 3. INDEX
    if (op == "INDEX") {
        std::string field = query_ast["field"];
        if (index_map[target].count(field)) return msg(Msg::IndexExists, field);
        
        index_map[target][field] = new IndexManager("data/" + target + "_" + field + ".idx", 3);
        auto records = storage_units[target]->scanAll();
        for (auto& row : records) {
            if (row.second.count(field)) {
                index_map[target][field]->insert(row.second[field], row.first);
            }
        }
        return msg(Msg::IndexCreated, field);
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

        return msg(Msg::CollectionDropped, target);
    }

    // 5. STATS
    if (op == "STATS") {
        return obtenerReporteMetricas(target);
    }

    // 6. SCAN
    if (op == "SCAN") {
        auto records = storage_units[target]->scanAll();
        if (records.empty()) return msg(Msg::ScanEmpty, target);
        std::stringstream ss;
        ss << msg(Msg::ScanHeader, std::to_string(records.size()));
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
        
        return msg(Msg::RecordInserted, payload["id"]);
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
            return msg(Msg::GetNeedsIndex, target);
        }

        // Realizar la búsqueda usando el índice dinámico encontrado
        RecordPointer ptr;
        if (findLiveRecord(target, search_field, search_value, ptr)) {
            std::map<std::string, std::string> record = storage_units[target]->getRecord(ptr);
            return msg(Msg::FoundViaIndex, search_field) + formatRecord(record);
        }
        
        return msg(Msg::GetNotFound, search_field, search_value);
    }

    // 9. DELETE
    else if (op == "DELETE") {
        if (!query_ast.count("id")) return msg(Msg::DeleteNeedsId);
        
        RecordPointer ptr;
        if (findLiveRecord(target, "id", query_ast["id"], ptr)) {
            storage_units[target]->logicalDelete(ptr);
            registros_sucios[target]++; 
            return msg(Msg::DeleteOk, query_ast["id"]);
        }
        return msg(Msg::DeleteNotFound);
    }

    // 10. UPDATE
    else if (op == "UPDATE") {
        if (!query_ast.count("id")) return msg(Msg::UpdateNeedsId);
        
        RecordPointer ptr;
        if (!findLiveRecord(target, "id", query_ast["id"], ptr)) {
            return msg(Msg::UpdateNotFound);
        }

        std::map<std::string, std::string> data = storage_units[target]->getRecord(ptr);
        if (data.empty()) return msg(Msg::AlreadyDeleted);

        for (auto const& [key, val] : query_ast) {
            if (key.substr(0, 5) == "_set_") data[key.substr(5)] = val;
        }

        storage_units[target]->logicalDelete(ptr);
        registros_sucios[target]++; 

        RecordPointer new_ptr = storage_units[target]->insertRecord(data);
        for (auto const& [field, idx] : index_map[target]) {
            if (data.count(field)) idx->insert(data[field], new_ptr); 
        }

        return msg(Msg::UpdateOk);
    }

    return msg(Msg::UnsupportedOp, op);
}