#ifndef HEAP_FILE_H
#define HEAP_FILE_H

#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <filesystem>

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