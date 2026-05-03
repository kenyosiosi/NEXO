#include "../../include/Storage/StorageManager.h"
#include <cstring>
#include <iostream>

// El constructor ahora recibe las rutas para el .bin y el .idx
StorageManager::StorageManager(const std::string& data_path, const std::string& index_path) : dataFile(data_path), indexFile(index_path), current_page_id(1)
{
    page_buffer.assign(PAGE_SIZE, 0); 
    
    std::vector<char> meta = dataFile.readPage(0);
    
    if (meta.empty() || (meta[0] == 0 && meta[1] == 0 && meta[2] == 0 && meta[3] == 0)) 
    {
        saveMetadata(); 
        initializePage(current_page_id);
    } 
    else 
    {
        loadMetadata();
        // CRÍTICO: Cargar la página actual en memoria para evitar offsets negativos
        page_buffer = dataFile.readPage(current_page_id);
    }
}
void StorageManager::saveMetadata()
{
    std::vector<char> meta_page(PAGE_SIZE, 0);
    // Para el archivo de datos, solo guardamos la página actual para saber dónde insertar
    memcpy(&meta_page[4], &current_page_id, 4);
    dataFile.writePage(0, meta_page); 
}

void StorageManager::loadMetadata()
{
    std::vector<char> meta_page = dataFile.readPage(0);
    memcpy(&current_page_id, &meta_page[4], 4);
}

void StorageManager::initializePage(int id)
{
    std::fill(page_buffer.begin(), page_buffer.end(), 0);
    memcpy(&page_buffer[0], &id, 4);
    int zero = 0;
    memcpy(&page_buffer[4], &zero, 4); // Cantidad de registros
    int initial_free = PAGE_SIZE;
    memcpy(&page_buffer[8], &initial_free, 4); // Offset de espacio libre
}

RecordPointer StorageManager::insertRecord(const std::map<std::string, std::string>& flat_map)
{
    std::vector<char> serialized_data = serializer.serialize(flat_map);
    int data_size = serialized_data.size();

    int slot_count;
    memcpy(&slot_count, &page_buffer[4], 4);
    int free_space_ptr;
    memcpy(&free_space_ptr, &page_buffer[8], 4);

    int space_needed = data_size + 8; 
    int current_header_end = 16 + (slot_count * 8); // 16 bytes de header general

    if (free_space_ptr - current_header_end < space_needed)
    {
        dataFile.writePage(current_page_id, page_buffer);
        current_page_id++;
        initializePage(current_page_id);
        saveMetadata(); 
        
        slot_count = 0;
        free_space_ptr = PAGE_SIZE;
    }

    int new_data_offset = free_space_ptr - data_size;
    memcpy(&page_buffer[new_data_offset], serialized_data.data(), data_size);

    int slot_pos = 16 + (slot_count * 8);
    memcpy(&page_buffer[slot_pos], &new_data_offset, 4);
    memcpy(&page_buffer[slot_pos + 4], &data_size, 4);

    slot_count++;
    memcpy(&page_buffer[4], &slot_count, 4);
    memcpy(&page_buffer[8], &new_data_offset, 4);

    dataFile.writePage(current_page_id, page_buffer);

    return {current_page_id, slot_count - 1}; 
}

std::map<std::string, std::string> StorageManager::getRecord(RecordPointer pointer)
{
    // Leemos el registro desde el archivo de datos
    std::vector<char> page = dataFile.readPage(pointer.page_id);

    int PAGE_DATA_HEADER_SIZE = 16; 
    int slot_pos = PAGE_DATA_HEADER_SIZE + (pointer.slot_offset * 8);
    
    int data_offset;
    int data_size;
    
    memcpy(&data_offset, &page[slot_pos], 4);
    memcpy(&data_size, &page[slot_pos + 4], 4);

    if (data_offset + data_size > PAGE_SIZE || data_size <= 0) return {}; 
    
    std::vector<char> record_bytes(page.begin() + data_offset, page.begin() + data_offset + data_size);
    // El serializer preparará el mapa que luego imprimirás en consola
    return serializer.deserialize(record_bytes);
}

// ==========================================
// NUEVAS FUNCIONES PARA EL INDEX MANAGER
// ==========================================

void StorageManager::writeIndexPage(int page_id, const std::vector<char>& data)
{
    if (data.size() != PAGE_SIZE) return;
    indexFile.writePage(page_id, data);
}

std::vector<char> StorageManager::readIndexPage(int page_id)
{
    return indexFile.readPage(page_id);
}