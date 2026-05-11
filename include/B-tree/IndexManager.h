#ifndef INDEXMANAGER_H
#define INDEXMANAGER_H

#include "../Storage/StorageManager.h"
#include "../Core/Types.h"
#include <vector>

// Estructura del nodo en disco (para PAGE_SIZE de 4KB)
struct DiskNode {
    bool is_leaf;
    int n; // número de llaves actuales
    int keys[200]; 
    RecordPointer pointers[200]; // Referencias al .bin
    int children[201]; // Referencias a otras páginas del .idx
};

class IndexManager {
private:
    StorageManager* storage; // Conexión al storage
    int rootPageId;
    int next_page_id;
    int t; // Grado mínimo

    void saveHeader();
    void loadHeader();
    void writeNode(int id, const DiskNode& node);
    void readNode(int id, DiskNode& node);
    void splitChild(int parent_id, int i, int child_id);
    void insertNonFull(int page_id, int k, RecordPointer p_data);
    RecordPointer searchRecursive(int page_id, int k);

public:
    IndexManager(StorageManager* sm, int degree);
    void insert(int k, RecordPointer p);
    RecordPointer search(int k);
    
    void remove(int id) {
        this->insert(id, {-1, -1}); 
    }
};

#endif