#ifndef INDEX_MANAGER_H
#define INDEX_MANAGER_H

#include "../Storage/HeapFile.h"
#include "../Core/Types.h"
#include <string>
#include <vector>
#include <cstring>

// Clave de índice de longitud fija (64 bytes). Las claves más largas se
// truncan al persistirlas.
struct IndexKey {
    char value[64];
    bool operator<(const IndexKey& o) const { return std::strcmp(value, o.value) < 0; }
    bool operator>(const IndexKey& o) const { return std::strcmp(value, o.value) > 0; }
    bool operator==(const IndexKey& o) const { return std::strcmp(value, o.value) == 0; }
};

/**
 * Árbol B persistido en páginas de PAGE_SIZE.
 *
 * El grado mínimo t es paramétrico: un nodo puede contener hasta 2t-1 claves
 * y hasta 2t hijos. El layout del nodo se serializa campo a campo, por lo que
 * el tamaño máximo de t está limitado por PAGE_SIZE:
 *   8 (cabecera) + (2t-1)*(64+8) + 2t*4 <= PAGE_SIZE  =>  t <= 27
 */
class IndexManager {
private:
    static constexpr int kMaxDegree = 27;

    struct DiskNode {
        bool is_leaf;
        int n;
        std::vector<IndexKey> keys;          // capacidad 2t-1
        std::vector<RecordPointer> pointers; // capacidad 2t-1
        std::vector<int> children;           // capacidad 2t
    };

    HeapFile indexFile;
    int rootPageId;   // -1 si el árbol está vacío
    int next_page_id;
    int t;
    int max_keys;     // 2t-1

    DiskNode makeEmptyNode() const;
    static IndexKey makeKey(const std::string& key);
    static std::string keyAt(const IndexKey& key) { return std::string(key.value); }

    void saveHeader();
    void loadHeader();
    int allocatePage();

    void writeNode(int id, const DiskNode& node);
    DiskNode readNode(int id);

    void splitChild(int parent_id, int i, int child_id);
    void insertNonFull(int page_id, const std::string& key, RecordPointer ptr);
    void searchRecursive(int page_id, const std::string& key, std::vector<RecordPointer>& results);
    RecordPointer findOneRecursive(int page_id, const std::string& key);

public:
    IndexManager(const std::string& index_path, int degree = 3);

    void insert(const std::string& key, RecordPointer ptr);
    std::vector<RecordPointer> search(const std::string& key);
    RecordPointer findOne(const std::string& key);

    int getDegree() const { return t; }
};

#endif // INDEX_MANAGER_H
