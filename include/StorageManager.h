#ifndef STORAGE_MANAGER_H
#define STORAGE_MANAGER_H

#include <string>
#include <vector>
#include <map>
#include <cstring>
#include <algorithm>
#include "HeapFile.h"
#include "Serializer.h"

struct RecordPointer 
{
    int page_id;
    int slot_offset;
};

class StorageManager 
{
    private:
        HeapFile heapFile;
        Serializer serializer;
        int current_page_id;
        std::vector<char> page_buffer; // Buffer en RAM de la página actual

        // Constantes de diseño
        const int PAGE_SIZE = 4096;
        const int PAGE_HEADER_SIZE = 12; // page_id (4) + slot_count (4) + free_space_offset (4)

        void initializePage(int id);

    public:
        StorageManager(const std::string& db_path);
        RecordPointer insertRecord(const std::map<std::string, std::string>& flat_map);

        std::string getRecord(RecordPointer pointer);
};

#endif // STORAGE_MANAGER_H