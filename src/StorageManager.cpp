#include "StorageManager.h"

StorageManager::StorageManager(const std::string& db_path) : heapFile(db_path), current_page_id(0)
{
    // Inicializar con una página vacía o cargar la última
    page_buffer.assign(PAGE_SIZE, 0);
    initializePage(current_page_id);
}

void StorageManager::initializePage(int id)
{
    // Limpiar buffer
    std::fill(page_buffer.begin(), page_buffer.end(), 0);
    
    // Escribir Header inicial
    // [0-3] ID de página
    memcpy(&page_buffer[0], &id, 4);
    // [4-7] Cantidad de registros (inicia en 0)
    int zero = 0;
    memcpy(&page_buffer[4], &zero, 4);
    // [8-11] Offset de espacio libre (inicia al final de la página: 4096)
    int initial_free = PAGE_SIZE;
    memcpy(&page_buffer[8], &initial_free, 4);
}

RecordPointer StorageManager::insertRecord(const std::map<std::string, std::string>& flat_map)
{
    // 1. Serializar el mapa a bytes
    std::vector<char> serialized_data = serializer.serialize(flat_map);
    int data_size = serialized_data.size();

    // 2. Leer metadatos actuales de la página desde el buffer
    int slot_count;
    memcpy(&slot_count, &page_buffer[4], 4);
    int free_space_ptr;
    memcpy(&free_space_ptr, &page_buffer[8], 4);

    // 3. Calcular si cabe: (Datos + un nuevo Slot de 8 bytes)
    int space_needed = data_size + 8; 
    int current_header_end = PAGE_HEADER_SIZE + (slot_count * 8);

    if (free_space_ptr - current_header_end < space_needed)
    {
        // ¡No cabe! Guardar página actual y crear una nueva
        heapFile.writePage(current_page_id, page_buffer);
        current_page_id++;
        initializePage(current_page_id);
        
        // Recargar valores para la nueva página
        slot_count = 0;
        free_space_ptr = PAGE_SIZE;
    }

    // 4. Insertar Datos al final del espacio libre
    int new_data_offset = free_space_ptr - data_size;
    memcpy(&page_buffer[new_data_offset], serialized_data.data(), data_size);

    // 5. Crear el Slot (índice) al inicio
    // Slot = [Offset del dato (4 bytes) | Tamaño del dato (4 bytes)]
    int slot_pos = PAGE_HEADER_SIZE + (slot_count * 8);
    memcpy(&page_buffer[slot_pos], &new_data_offset, 4);
    memcpy(&page_buffer[slot_pos + 4], &data_size, 4);

    // 6. Actualizar Header
    slot_count++;
    memcpy(&page_buffer[4], &slot_count, 4);
    memcpy(&page_buffer[8], &new_data_offset, 4);

    // 7. Persistir en archivo (opcional: puedes hacerlo cada N inserciones)
    heapFile.writePage(current_page_id, page_buffer);

    return {current_page_id, slot_count - 1}; // Retornamos el ID de página y el índice del slot
}