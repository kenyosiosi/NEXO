#ifndef INDEX_MANAGER_H
#define INDEX_MANAGER_H

#include <vector>
#include <string>
#include <cstring>
#include "HeapFile.h"
#include "Types.h"
// Para usar tu struct RecordPointer

// El grado t define la capacidad del nodo en disco.
// Un t=100 permite unas 199 llaves por nodo, ideal para páginas de 4KB.
const int MAX_T = 100; 

// Esta es la estructura plana que escribiremos físicamente en el disco.
// IMPORTANTE: Ya no usamos punteros de memoria (*), usamos IDs de página (int)
struct DiskNode {
    bool is_leaf;
    int n; // Número actual de llaves
    int keys[2 * MAX_T - 1];
    RecordPointer pointers[2 * MAX_T - 1];
    int children[2 * MAX_T]; // IDs de página de los hijos en vez de BTreeNode*
};

class IndexManager {
private:
    HeapFile indexFile;
    int rootPageId;
    int next_page_id;
    int t;

    // Métodos internos para leer/escribir páginas de 4KB
    void readNode(int page_id, DiskNode& node);
    void writeNode(int page_id, const DiskNode& node);
    
    // Métodos para la Página 0 (Cabecera)
    void saveHeader();
    void loadHeader();

    // Lógica de B-Tree adaptada a disco
    void splitChild(int parent_id, int i, int child_id);
    void insertNonFull(int page_id, int k, RecordPointer p);
    RecordPointer searchRecursive(int page_id, int k);

public:
    IndexManager(const std::string& path, int degree = MAX_T);
    
    void insert(int k, RecordPointer p);
    RecordPointer search(int k);
    
    int getRootId() const { return rootPageId; }
};

#endif