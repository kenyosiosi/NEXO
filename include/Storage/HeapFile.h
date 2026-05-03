#ifndef HEAP_FILE_H
#define HEAP_FILE_H

#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <filesystem>

// Definir aquí para que sea global en el proyecto
const int PAGE_SIZE = 4096; 

class HeapFile
{
    private:
        std::fstream file;
        std::string filename;
    public:
        HeapFile(const std::string& path);
        void writePage(int page_id, const std::vector<char>& data);
        std::vector<char> readPage(int pageId);
        ~HeapFile();
};

#endif // HEAP_FILE_H