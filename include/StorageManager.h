#ifndef STORAGE_MANAGER_H
#define STORAGE_MANAGER_H

#include <string>
#include <vector>
#include <map>
#include <cstring>
#include <algorithm>
#include <iostream>
#include "HeapFile.h"
#include "Serializer.h"
#include "Types.h"


class StorageManager 
{
    private:
        HeapFile heapFile;
        Serializer serializer;
        int current_page_id;
        int root_page_id; // ID de la página raíz del B-Tree
        std::vector<char> page_buffer; // Buffer en RAM de la página actual

        // Constantes de diseño
        const int PAGE_SIZE = 4096;
        const int PAGE_HEADER_SIZE = 16; // page_id (4) + slot_count (4) + free_space_offset (4) + reserved (4)

        // Métodos internos de gestión
        void initializePage(int id);
        void saveMetadata(); // Escribe en la Página 0
        void loadMetadata(); // Lee desde la Página 0

    public:
        StorageManager(const std::string& db_path);
        RecordPointer insertRecord(const std::map<std::string, std::string>& flat_map);
        std::map<std::string, std::string> getRecord(RecordPointer pointer);

        int allocateNewPage(int type); // Reserva una página nueva y devuelve su ID
        
        // Acceso directo a bytes para nodos del árbol
        void writeRawPage(int page_id, const std::vector<char>& data); 
        std::vector<char> readRawPage(int page_id);

        // Gestión de la raíz
        int getRootPageId() { return root_page_id; }
        void setRootPageId(int id) { 
            root_page_id = id; 
            saveMetadata(); 
        }
};

#endif // STORAGE_MANAGER_H