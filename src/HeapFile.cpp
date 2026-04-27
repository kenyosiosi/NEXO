#include "HeapFile.h"

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
    // Calculamos el offset: Si es la página 0, byte 0. Si es la 1, byte 4096.
    std::streampos pos = static_cast<std::streampos>(page_id) * PAGE_SIZE;
    
    file.seekp(pos); // Mover el puntero de escritura
    if (file.fail()) {
        file.clear(); // Limpiar errores si intentamos escribir muy lejos
    }
    
    file.write(data.data(), PAGE_SIZE);
    file.flush(); // Aseguramos que se escriba al disco físicamente
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