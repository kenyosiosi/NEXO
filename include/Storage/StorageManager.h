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
    HeapFile dataFile;   
    int current_page_id;
    std::vector<char> page_buffer;

    void saveMetadata();
    void loadMetadata();
    void initializePage(int id);

public:
    StorageManager(const std::string& data_path);
    RecordPointer insertRecord(std::map<std::string, std::string>& record);
    std::map<std::string, std::string> getRecord(RecordPointer ptr);
    int getLastPageId() { return current_page_id; } // para reconstruir índices
    void logicalDelete(RecordPointer pointer);
    std::vector<std::pair<RecordPointer, std::map<std::string, std::string>>> scanAll();
};

#endif