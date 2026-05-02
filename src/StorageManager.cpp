#include "StorageManager.h"

StorageManager::StorageManager(const std::string& db_path) : heapFile(db_path), current_page_id(1), root_page_id(-1) // Empezamos datos en Pág 1
{
    page_buffer.assign(PAGE_SIZE, 0);
    
    // Intentar cargar metadatos existentes
    std::vector<char> meta = heapFile.readPage(0);
    
    // Si la página está vacía (archivo nuevo), inicializamos metadatos
    if (meta.empty() || (meta[0] == 0 && meta[1] == 0 && meta[2] == 0 && meta[3] == 0)) 
    {
        saveMetadata(); // Crear Página 0 inicial
        initializePage(current_page_id); // Crear Página 1 para datos
    } 
    else 
    {
        loadMetadata(); // Cargar root_page_id y current_page_id existentes
    }
}

void StorageManager::saveMetadata()
{
    std::vector<char> meta_page(PAGE_SIZE, 0);
    // [0-3] Root Page ID
    memcpy(&meta_page[0], &root_page_id, 4);
    // [4-7] Last Page ID (para saber dónde seguir insertando)
    memcpy(&meta_page[4], &current_page_id, 4);
    
    heapFile.writePage(0, meta_page); // Guardar en la posición física 0
}

void StorageManager::loadMetadata()
{
    std::vector<char> meta_page = heapFile.readPage(0);
    memcpy(&root_page_id, &meta_page[0], 4);
    memcpy(&current_page_id, &meta_page[4], 4);
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

        saveMetadata(); // Notificamos a la Pág 0 que ahora hay una nueva página activa
        
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

std::map<std::string, std::string> StorageManager::getRecord(RecordPointer pointer)
{
    // 1. Leer la página completa desde el HeapFile
    std::vector<char> page = heapFile.readPage(pointer.page_id);

    // 2. VALIDACIÓN DE SEGURIDAD (Paso 2): Verificar que sea una página de datos
    int page_type;
    memcpy(&page_type, &page[4], 4); // El tipo está en los bytes 4-7[cite: 9]
    
    if (page_type != 0) { // 0 es el código para Páginas de Datos[cite: 9]
        std::cerr << "Error: Se intentó leer un registro de una página que no es de datos (Tipo: " << page_type << ")" << std::endl;
        return {}; // Retorna un mapa vacío por seguridad
    }

    // 3. Ubicar el Slot (Ajustado al header: ID(4) + Tipo(4) + Slots(4) + FreeSpace(4) = 16 bytes iniciales)
    // Nota: Usamos 16 como desplazamiento base si tus páginas de datos ahora incluyen el campo 'Tipo'[cite: 9]
    int PAGE_DATA_HEADER_SIZE = 16; 
    int slot_pos = PAGE_DATA_HEADER_SIZE + (pointer.slot_offset * 8);
    
    int data_offset;
    int data_size;
    
    memcpy(&data_offset, &page[slot_pos], 4);
    memcpy(&data_size, &page[slot_pos + 4], 4);

    // 4. Extraer bytes y deserializar
    if (data_offset + data_size > PAGE_SIZE) return {}; // Validación extra de límites
    
    std::vector<char> record_bytes(page.begin() + data_offset, page.begin() + data_offset + data_size);
    return serializer.deserialize(record_bytes);
}

// Escribir un nodo del B-Tree directamente a una página
void StorageManager::writeRawPage(int page_id, const std::vector<char>& data)
{
    if (data.size() != PAGE_SIZE) return; // Validación de seguridad
    heapFile.writePage(page_id, data);
}

// Leer un nodo para que el B-Tree lo procese en RAM
std::vector<char> StorageManager::readRawPage(int page_id)
{
    return heapFile.readPage(page_id);
}

int StorageManager::allocateNewPage(int type)
{
    current_page_id++; // Incrementamos el contador global
    
    std::vector<char> new_page(PAGE_SIZE, 0);
    
    // Escribir el header básico
    memcpy(&new_page[0], &current_page_id, 4); // ID
    memcpy(&new_page[4], &type, 4);            // Tipo (0: Datos, 1: Índice)
    
    // Si es de datos, inicializamos punteros de slots
    if (type == 0) {
        int zero = 0;
        int initial_free = PAGE_SIZE;
        memcpy(&new_page[8], &zero, 4);         // Slot count
        memcpy(&new_page[12], &initial_free, 4); // Free space offset
    }

    heapFile.writePage(current_page_id, new_page);
    saveMetadata(); // Actualizamos la Página 0 con el nuevo current_page_id
    
    return current_page_id;
}