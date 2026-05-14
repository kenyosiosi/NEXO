#ifndef INDEX_MANAGER_H
#define INDEX_MANAGER_H

#include "../../include/Storage/HeapFile.h"
#include "../../include/Core/Types.h"
#include <string>
#include <vector>
#include <cstring>

struct IndexKey {
    char value[64];
    bool operator<(const IndexKey& o) const { return std::strcmp(value, o.value) < 0; }
    bool operator>(const IndexKey& o) const { return std::strcmp(value, o.value) > 0; }
    bool operator==(const IndexKey& o) const { return std::strcmp(value, o.value) == 0; }
};

inline bool operator<(const std::string& s, const IndexKey& k) { return s < k.value; }
inline bool operator>(const std::string& s, const IndexKey& k) { return s > k.value; }
inline bool operator==(const std::string& s, const IndexKey& k) { return s == k.value; }

struct DiskNode {
    bool is_leaf;
    int n;
    IndexKey keys[5]; 
    RecordPointer pointers[5];
    int children[6];
};

class IndexManager {
private:
    HeapFile indexFile;
    int rootPageId;
    int next_page_id;
    int t;

    void loadHeader();
    void saveHeader();
    void writeNode(int id, const DiskNode& node);
    void readNode(int id, DiskNode& node);
    void splitChild(int parent_id, int i, int child_id);
    void insertNonFull(int page_id, const std::string& key, RecordPointer ptr);
    void searchRecursive(int page_id, const std::string& k, std::vector<RecordPointer>& results);

public:
    IndexManager(const std::string& index_path, int degree);
    void insert(const std::string& k, RecordPointer p_data);
    std::vector<RecordPointer> search(const std::string& k);
    std::vector<RecordPointer> searchRecursive(int page_id, const std::string& k);
};
#endif