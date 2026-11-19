#ifndef STORAGEMANAGER_H
#define STORAGEMANAGER_H

#include "HeapFile.h"
#include "../Core/Types.h"
#include "Serializer.h"
#include <map>
#include <string>
#include <vector>

// Resumen de ocupación física de una colección, para el panel de estadísticas.
struct StorageStats {
    int pages = 0;          // páginas de datos en uso (sin contar la página 0 de metadatos)
    size_t totalSlots = 0;  // slots reservados en todas las páginas
    size_t live = 0;        // slots con un registro vivo
    size_t dead = 0;        // slots con tombstone (borrados lógicamente)
};

class StorageManager {
private:
    HeapFile dataFile;   
    int current_page_id;
    std::vector<char> page_buffer;

    void saveMetadata();
    void loadMetadata();
    void initializePage(int id);

public:
    StorageManager(const std::string& data_path);
    RecordPointer insertRecord(const std::map<std::string, std::string>& record);
    std::map<std::string, std::string> getRecord(RecordPointer ptr);
    int getLastPageId() { return current_page_id; } // para reconstruir índices
    void logicalDelete(RecordPointer pointer);
    std::vector<std::pair<RecordPointer, std::map<std::string, std::string>>> scanAll();
    StorageStats getStats();
};

#endif