#include "../../include/Storage/StorageManager.h"
#include "Core/Messages.h"
#include <cstring>
#include <algorithm>
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

RecordPointer StorageManager::insertRecord(const std::map<std::string, std::string>& record) {
    std::vector<char> serialized_data = Serializer::serialize(record);
    const int data_size = static_cast<int>(serialized_data.size());
    // La zona útil de una página: 16 bytes de cabecera + slots de 8 bytes.
    constexpr int PAGE_USABLE = PAGE_SIZE - 16;
    if (8 + data_size > PAGE_USABLE) {
        // Un registro que no cabe ni en una página vacía provocaría una
        // recursión infinita buscando espacio: se rechaza de forma explícita.
        throw std::runtime_error(nexo::msg(nexo::Msg::RecordTooBig,
                                           std::to_string(data_size),
                                           std::to_string(PAGE_USABLE - 8)));
    }

    int slot_count, free_space_offset;
    memcpy(&slot_count, &page_buffer[4], 4);
    memcpy(&free_space_offset, &page_buffer[8], 4);

    const int space_needed = 8 + data_size;
    const int current_header_end = 16 + (slot_count * 8);

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
    // Validar el puntero antes de tocar la página: un índice con entradas
    // inválidas no debe poder leer fuera de la página.
    if (pointer.page_id < 1 || pointer.page_id > current_page_id) return {};
    if (pointer.slot_offset < 0) return {};

    const int slot_pos = 16 + (pointer.slot_offset * 8);
    if (slot_pos + 8 > PAGE_SIZE) return {};

    std::vector<char> page = dataFile.readPage(pointer.page_id);
    if (page.empty()) return {};

    int data_offset, data_size;
    memcpy(&data_offset, &page[slot_pos], 4);
    memcpy(&data_size, &page[slot_pos + 4], 4);

    // Si el size es -1, significa que fue eliminado 
    if (data_size <= 0 || data_offset < 0 ||
        data_offset > PAGE_SIZE || data_size > PAGE_SIZE ||
        data_offset + data_size > PAGE_SIZE) return {};
    
    std::vector<char> record_data(page.begin() + data_offset, page.begin() + data_offset + data_size);
    return Serializer::deserialize(record_data);
}

// Borrado lógico para UPDATE y DELETE
void StorageManager::logicalDelete(RecordPointer pointer) {
    if (pointer.page_id < 1 || pointer.page_id > current_page_id || pointer.slot_offset < 0) return;
    const int slot_pos = 16 + (pointer.slot_offset * 8);
    if (slot_pos + 8 > PAGE_SIZE) return;

    // para determinar si la página está en el buffer de RAM o en el disco
    bool is_in_buffer = (pointer.page_id == current_page_id);
    std::vector<char> page;

    if (is_in_buffer) {
        page = page_buffer; // Usar lo que está en RAM
    } else {
        page = dataFile.readPage(pointer.page_id); // Leer de disco
    }

    if (page.empty()) return;

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
std::vector<std::pair<RecordPointer, std::map<std::string, std::string>>> StorageManager::scanAll() {    std::vector<std::pair<RecordPointer, std::map<std::string, std::string>>> all_records;
    for (int p = 1; p <= current_page_id; p++) {
        std::vector<char> page = dataFile.readPage(p);
        if (page.empty()) continue;
        int slot_count;
        memcpy(&slot_count, &page[4], 4);
        // Un slot_count corrupto no debe lanzar el escaneo a bucles infinitos:
        // cada slot son 8 bytes dentro del área útil de la página.
        slot_count = std::max(0, std::min(slot_count, (PAGE_SIZE - 16) / 8));
        
        for (int s = 0; s < slot_count; s++) {
            RecordPointer ptr = {p, s};
            auto record = getRecord(ptr);
            if (!record.empty()) all_records.push_back({ptr, record});
        }
    }
    return all_records;
}

// Estadísticas de ocupación: recorre cabeceras y slots sin deserializar cada
// registro, así el panel de la UI puede refrescarse en cada frame.
StorageStats StorageManager::getStats() {
    StorageStats stats;
    for (int p = 1; p <= current_page_id; p++) {
        std::vector<char> page = dataFile.readPage(p);
        if (page.empty()) continue;

        int slot_count;
        memcpy(&slot_count, &page[4], 4);
        slot_count = std::max(0, std::min(slot_count, (PAGE_SIZE - 16) / 8));

        stats.pages++;
        stats.totalSlots += static_cast<size_t>(slot_count);

        for (int s = 0; s < slot_count; s++) {
            const int slot_pos = 16 + (s * 8);
            int data_offset, data_size;
            memcpy(&data_offset, &page[slot_pos], 4);
            memcpy(&data_size, &page[slot_pos + 4], 4);

            // Misma validación que getRecord: un slot corrupto cuenta como muerto.
            const bool alive = data_size > 0 && data_offset >= 0 &&
                               data_offset <= PAGE_SIZE && data_size <= PAGE_SIZE &&
                               data_offset + data_size <= PAGE_SIZE;
            if (alive) stats.live++;
            else stats.dead++;
        }
    }
    return stats;
}