#ifndef STORAGEMANAGER_H
#define STORAGEMANAGER_H

#include "HeapFile.h"
#include "../Core/Types.h"
#include "Serializer.h"
#include <map>
#include <string>
#include <vector>


class StorageManager {
private:
    HeapFile dataFile;   // Para el .bin
    HeapFile indexFile;  // Para el .idx
    int current_page_id;
    std::vector<char> page_buffer;
    Serializer serializer;

    void saveMetadata();
    void loadMetadata();
    void initializePage(int id);

public:
    // El constructor ahora pide dos rutas
    StorageManager(const std::string& data_path, const std::string& index_path);

    // Métodos para Datos
    RecordPointer insertRecord(const std::map<std::string, std::string>& flat_map);
    std::map<std::string, std::string> getRecord(RecordPointer ptr);

    // Métodos para el B-Tree (Índice)
    void writeIndexPage(int page_id, const std::vector<char>& data);
    std::vector<char> readIndexPage(int page_id);
};

#endif