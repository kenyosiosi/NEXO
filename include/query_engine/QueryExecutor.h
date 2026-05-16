#ifndef QUERY_EXECUTOR_H
#define QUERY_EXECUTOR_H

#include <string>
#include <map>
#include <vector>
#include <sstream>
#include "../Storage/StorageManager.h"
#include "../B-tree/IndexManager.h"

class QueryExecutor {
private:
    std::map<std::string, StorageManager*>& storage_units;
    std::map<std::string, std::map<std::string, IndexManager*>>& index_map;
    std::map<std::string, size_t>& registros_sucios; // Copiado de la versión de consola

    std::string formatRecord(const std::map<std::string, std::string>& record);
    std::string obtenerReporteMetricas(const std::string& target);

public:
    QueryExecutor(std::map<std::string, StorageManager*>& storage, 
                  std::map<std::string, std::map<std::string, IndexManager*>>& indices,
                  std::map<std::string, size_t>& sucios);

    std::string execute(std::map<std::string, std::string> query_ast);
};

#endif