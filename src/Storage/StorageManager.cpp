#include "../../include/Storage/StorageManager.h"
#include <cstring>
#include <iostream>

StorageManager::StorageManager(const std::string& data_path) : dataFile(data_path), current_page_id(1) {
    page_buffer.assign(PAGE_SIZE, 0); 
    std::vector<char> meta = dataFile.readPage(0);
    
    if (meta.empty() || (meta[0] == 0 && meta[4] == 0)) {
        saveMetadata(); 
        initializePage(current_page_id);
        page_buffer = dataFile.readPage(current_page_id); 
    } else {
        loadMetadata();
        page_buffer = dataFile.readPage(current_page_id);
    }
}

void StorageManager::saveMetadata() {
    std::vector<char> meta_page(PAGE_SIZE, 0);
    memcpy(&meta_page[4], &current_page_id, 4);
    dataFile.writePage(0, meta_page); 
}

void StorageManager::loadMetadata() {
    std::vector<char> meta = dataFile.readPage(0);
    if (!meta.empty()) memcpy(&current_page_id, &meta[4], 4);
}

void StorageManager::initializePage(int page_id) {
    std::vector<char> new_page(PAGE_SIZE, 0);
    int slot_count = 0;
    int free_space_offset = PAGE_SIZE;
    memcpy(&new_page[0], &page_id, 4);
    memcpy(&new_page[4], &slot_count, 4);
    memcpy(&new_page[8], &free_space_offset, 4);
    dataFile.writePage(page_id, new_page);
}

RecordPointer StorageManager::insertRecord(std::map<std::string, std::string>& record) {
    std::vector<char> serialized_data = Serializer::serialize(record);
    int data_size = serialized_data.size();
    int slot_count, free_space_offset;
    memcpy(&slot_count, &page_buffer[4], 4);
    memcpy(&free_space_offset, &page_buffer[8], 4);

    int space_needed = 8 + data_size; 
    int current_header_end = 16 + (slot_count * 8);

    if (free_space_offset - current_header_end < space_needed) {
        current_page_id++;
        initializePage(current_page_id);
        page_buffer = dataFile.readPage(current_page_id);
        saveMetadata();
        return insertRecord(record); 
    }

    int new_data_offset = free_space_offset - data_size;
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

std::map<std::string, std::string> StorageManager::getRecord(RecordPointer pointer) {
    std::vector<char> page = dataFile.readPage(pointer.page_id);
    if (page.empty()) return {};

    int slot_pos = 16 + (pointer.slot_offset * 8);
    int data_offset, data_size;
    memcpy(&data_offset, &page[slot_pos], 4);
    memcpy(&data_size, &page[slot_pos + 4], 4);

    // Si el size es -1, significa que fue eliminado 
    if (data_size <= 0 || data_offset + data_size > PAGE_SIZE) return {};
    
    std::vector<char> record_data(page.begin() + data_offset, page.begin() + data_offset + data_size);
    return Serializer::deserialize(record_data);
}

// Borrado lógico para UPDATE y DELETE
void StorageManager::logicalDelete(RecordPointer pointer) {
    // para determinar si la página está en el buffer de RAM o en el disco
    bool is_in_buffer = (pointer.page_id == current_page_id);
    std::vector<char> page;

    if (is_in_buffer) {
        page = page_buffer; // Usar lo que está en RAM
    } else {
        page = dataFile.readPage(pointer.page_id); // Leer de disco
    }

    if (page.empty()) return;

    int slot_pos = 16 + (pointer.slot_offset * 8);
    int dead_size = -1; 
    memcpy(&page[slot_pos + 4], &dead_size, 4); 

    // Sincronizar y Persistir
    if (is_in_buffer) {
        page_buffer = page; //  Actualizar la RAM
    }
    
    // Escribir a disco para que sea permanente
    dataFile.writePage(pointer.page_id, page);
}

// Escaneo completo para índices retroactivos
std::vector<std::pair<RecordPointer, std::map<std::string, std::string>>> StorageManager::scanAll() {
    std::vector<std::pair<RecordPointer, std::map<std::string, std::string>>> all_records;
    for (int p = 1; p <= current_page_id; p++) {
        std::vector<char> page = dataFile.readPage(p);
        if (page.empty()) continue;
        int slot_count;
        memcpy(&slot_count, &page[4], 4);
        
        for (int s = 0; s < slot_count; s++) {
            RecordPointer ptr = {p, s};
            auto record = getRecord(ptr);
            if (!record.empty()) all_records.push_back({ptr, record});
        }
    }
    return all_records;
}