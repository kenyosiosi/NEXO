#include "../../include/Storage/HeapFile.h"

const int PAGE_SIZE = 4096;

HeapFile::HeapFile(const std::string& db_path) : filename(db_path)
{
    // Extraer la ruta de la carpeta
    std::filesystem::path p(db_path);
    if (p.has_parent_path()) {
        // Crear el directorio si no existe
        std::filesystem::create_directories(p.parent_path());
    }

    // Abrimos en modo lectura/escritura binaria. 
    // Si no existe, lo creamos.
    file.open(filename, std::ios::binary | std::ios::in | std::ios::out);

    if (!file.is_open()) 
    {
        // Caso donde el archivo no existe: lo creamos con trunc
        file.open(filename, std::ios::binary | std::ios::out | std::ios::trunc);
        file.close();
        // Lo reabrimos correctamente para in/out
        file.open(filename, std::ios::binary | std::ios::in | std::ios::out);
    }
}

void HeapFile::writePage(int page_id, const std::vector<char>& data)
{
    if (data.size() != PAGE_SIZE) return; // Validación de tamaño

    // 1. VALIDACIÓN DE SEGURIDAD (Paso 3): Evitar archivos "huecos"
    // Buscamos el final del archivo para saber cuánto mide actualmente
    file.seekg(0, std::ios::end);
    std::streampos file_size = file.tellg();
    int max_allowed_page = (file_size / PAGE_SIZE); 

    // Solo permitimos escribir en páginas existentes o en la inmediatamente siguiente (append)
    if (page_id > max_allowed_page) 
    {
        std::cerr << "Error: Intento de escritura fuera de límites. ID: " << page_id 
                << " Max permitido: " << max_allowed_page << std::endl;
        return;
    }

    // 2. Ejecutar la escritura
    std::streampos pos = static_cast<std::streampos>(page_id) * PAGE_SIZE;
    file.seekp(pos); 
    
    if (file.fail()) {
        file.clear();
        return;
    }
    
    file.write(data.data(), PAGE_SIZE);
    file.flush(); // Persistencia física
}

// Cerramos el archivo al destruir el objeto
HeapFile::~HeapFile()
{
    if (file.is_open()) file.close();
}

// Lee 4KB exactos de una posición
std::vector<char> HeapFile::readPage(int pageId)
{
    std::vector<char> buffer(PAGE_SIZE);
    file.seekg(pageId * PAGE_SIZE);
    file.read(buffer.data(), PAGE_SIZE);
    return buffer;
}