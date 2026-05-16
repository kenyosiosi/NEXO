#include "../../include/Query_engine/QueryExecutor.h"
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

QueryExecutor::QueryExecutor(std::map<std::string, StorageManager*>& storage, 
                             std::map<std::string, std::map<std::string, IndexManager*>>& indices,
                             std::map<std::string, size_t>& sucios)
    : storage_units(storage), index_map(indices), registros_sucios(sucios) {}

std::string QueryExecutor::formatRecord(const std::map<std::string, std::string>& record) {
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

    ss << "\n--- MÉTRICAS DE ESTRUCTURA (" << target << ") ---\n";
    ss << " Registros Vivos:    " << vivos << "\n";
    ss << " Registros Sucios:   " << sucios << "\n";
    ss << " Fragmentación:      " << ratio << " %\n";
    ss << " Tamaño en disco:    " << file_size << " bytes\n";
    
    if (totales > 0) {
        ss << " Promedio/Registro:  " << (file_size / totales) << " bytes\n";
    }
    ss << "--------------------------------------";

    // Si supera el umbral del 30%, avisamos a la UI que requiere compactación
    if (vivos >= 100 && ratio >= 30.0) {
        ss << "\n[ALERTA] ¡Índice de fragmentación crítico! Se sugiere autoCompact().";
    }

    return ss.str();
}

std::string QueryExecutor::execute(std::map<std::string, std::string> query_ast) {
    if (query_ast.empty() || query_ast.find("operation") == query_ast.end()) {
        return "[Error] Consulta vacía o mal formada.";
    }

    std::string op = query_ast["operation"];
    std::string target = query_ast["_table_target"];

    if (op == "CREATE") {
        return "Comando CREATE detectado para '" + target + "'. Configure desde el panel visual.";
    }

    if (storage_units.find(target) == storage_units.end()) {
        return "[Error] La colección '" + target + "' no existe.";
    }

    if (op == "SCAN") {
        auto records = storage_units[target]->scanAll();
        if (records.empty()) return "La colección '" + target + "' está vacía.";
        
        std::stringstream ss;
        ss << records.size() << " documentos encontrados en '" << target << "':\n";
        for (auto& row : records) {
            ss << "  " << formatRecord(row.second) << "\n";
        }
        return ss.str();
    } 
    
    else if (op == "INSERT") {
        std::map<std::string, std::string> payload = query_ast;
        payload.erase("operation");
        payload.erase("_table_target");

        RecordPointer ptr = storage_units[target]->insertRecord(payload);

        // Indexar dinámicamente si los índices existen
        for (auto const& [field, idx] : index_map[target]) {
            if (payload.count(field)) idx->insert(payload[field], ptr);
        }
        
        return "Ok. Registro insertado exitosamente.\n" + obtenerReporteMetricas(target);
    }

    else if (op == "GET") {
        if (!query_ast.count("id") || !index_map[target].count("id")) {
            return "[Error] GET requiere filtrar por un campo 'id' indexado.";
        }

        RecordPointer ptr = index_map[target]["id"]->findOne(query_ast["id"]);
        if (ptr.page_id != -1) {
            return "Documento encontrado:\n" + formatRecord(storage_units[target]->getRecord(ptr));
        }
        return "Documento no encontrado (ID: " + query_ast["id"] + ").";
    }

    else if (op == "DELETE") {
        if (!query_ast.count("id") || !index_map[target].count("id")) {
            return "[Error] DELETE requiere especificar el 'id'.";
        }

        RecordPointer ptr = index_map[target]["id"]->findOne(query_ast["id"]);
        if (ptr.page_id != -1) {
            storage_units[target]->logicalDelete(ptr);
            registros_sucios[target]++; // Incrementar contador de fragmentación
            
            return "Documento eliminado lógicamente.\n" + obtenerReporteMetricas(target);
        }
        return "No se encontró el documento para eliminar.";
    }

    else if (op == "UPDATE") {
        if (!query_ast.count("id") || !index_map[target].count("id")) {
            return "[Error] UPDATE requiere un 'id' válido.";
        }

        RecordPointer ptr = index_map[target]["id"]->findOne(query_ast["id"]);
        if (ptr.page_id == -1) return "No se encontró el documento a modificar.";

        // Obtener datos actuales y mezclar modificaciones de los campos modificados (_set_)
        std::map<std::string, std::string> data = storage_units[target]->getRecord(ptr);
        for (auto const& [key, val] : query_ast) {
            if (key.substr(0, 5) == "_set_") {
                data[key.substr(5)] = val;
            }
        }

        // Out-of-place update (Estrategia NoSQL para registros variables)
        storage_units[target]->logicalDelete(ptr);
        registros_sucios[target]++; 

        RecordPointer new_ptr = storage_units[target]->insertRecord(data);
        index_map[target]["id"]->insert(data["id"], new_ptr);

        return "Documento actualizado correctamente.\n" + obtenerReporteMetricas(target);
    }

    return "[Error] Operación no soportada: " + op;
}