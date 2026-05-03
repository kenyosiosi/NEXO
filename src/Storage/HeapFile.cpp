#include "../../include/Storage/HeapFile.h"
#include <filesystem>
#include <iostream>

HeapFile::HeapFile(const std::string& db_path) : filename(db_path)
{
    std::filesystem::path p(db_path);
    if (p.has_parent_path()) {
        std::filesystem::create_directories(p.parent_path());
    }

    file.open(filename, std::ios::binary | std::ios::in | std::ios::out);

    if (!file.is_open()) 
    {
        file.open(filename, std::ios::binary | std::ios::out | std::ios::trunc);
        file.close();
        file.open(filename, std::ios::binary | std::ios::in | std::ios::out);
    }
}

void HeapFile::writePage(int page_id, const std::vector<char>& data)
{
    if (data.size() != PAGE_SIZE) return;

    // 1. CRÍTICO: Limpiar cualquier bandera de error (EOF) antes de escribir
    file.clear(); 
    
    std::streampos pos = static_cast<std::streampos>(page_id) * PAGE_SIZE;
    file.seekp(pos); 
    file.write(data.data(), PAGE_SIZE);
    file.flush(); 
}

HeapFile::~HeapFile()
{
    if (file.is_open()) file.close();
}

std::vector<char> HeapFile::readPage(int pageId)
{
    std::vector<char> buffer(PAGE_SIZE, 0); // Aseguramos que inicie en ceros
    
    // 2. CRÍTICO: Limpiar banderas antes de leer por si lecturas previas fallaron
    file.clear(); 
    
    file.seekg(pageId * PAGE_SIZE);
    file.read(buffer.data(), PAGE_SIZE);
    return buffer;
}