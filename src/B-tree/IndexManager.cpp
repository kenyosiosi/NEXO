#include "../../include/B-tree/IndexManager.h"
#include <cstring>
#include <string>

IndexManager::IndexManager(const std::string& index_path, int degree) : indexFile(index_path), t(degree) {
    std::vector<char> buffer = indexFile.readPage(0);
    if (buffer.empty() || (buffer[0] == 0 && buffer[4] == 0)) {
        rootPageId = -1;
        next_page_id = 1;
        saveHeader();
    } else {
        loadHeader();
    }
}

void IndexManager::saveHeader() {
    std::vector<char> buffer(PAGE_SIZE, 0);
    memcpy(&buffer[0], &rootPageId, sizeof(int));
    memcpy(&buffer[4], &next_page_id, sizeof(int));
    memcpy(&buffer[8], &t, sizeof(int));
    indexFile.writePage(0, buffer);
}

void IndexManager::loadHeader() {
    std::vector<char> buffer = indexFile.readPage(0);
    memcpy(&rootPageId, &buffer[0], sizeof(int));
    memcpy(&next_page_id, &buffer[4], sizeof(int));
    memcpy(&t, &buffer[8], sizeof(int));
}

void IndexManager::writeNode(int id, const DiskNode& node) {
    std::vector<char> buffer(PAGE_SIZE, 0);
    memcpy(buffer.data(), &node, sizeof(DiskNode));
    indexFile.writePage(id, buffer);
}

void IndexManager::readNode(int id, DiskNode& node) {
    std::vector<char> buffer = indexFile.readPage(id);
    if (!buffer.empty()) memcpy(&node, buffer.data(), sizeof(DiskNode));
}

std::vector<RecordPointer> IndexManager::search(const std::string& k) {
    std::vector<RecordPointer> results;
    if (rootPageId != -1) searchRecursive(rootPageId, k, results);
    return results;
}

void IndexManager::searchRecursive(int page_id, const std::string& k, std::vector<RecordPointer>& results) {
   DiskNode node = {};
    readNode(page_id, node);
    int i = 0;
    while (i < node.n && k > std::string(node.keys[i].value)) i++;

    int temp_i = i;
    while (temp_i < node.n && k == std::string(node.keys[temp_i].value)) {
        results.push_back(node.pointers[temp_i]);
        temp_i++;
    }

    if (!node.is_leaf) {
        for (int j = i; j <= temp_i; j++) {
            searchRecursive(node.children[j], k, results);
        }
    }
}

void IndexManager::insert(const std::string& key, RecordPointer ptr) {
    if (rootPageId == -1) {
        rootPageId = next_page_id++;
        DiskNode root = {}; root.is_leaf = true; root.n = 1;
        std::strncpy(root.keys[0].value, key.c_str(), 63);
        root.keys[0].value[63] = '\0';
        root.pointers[0] = ptr;
        writeNode(rootPageId, root);
        saveHeader();
    } else {
        DiskNode root = {}; 
        readNode(rootPageId, root);
        if (root.n == 5) {
            int new_root_id = next_page_id++;
            DiskNode new_root = {};
            new_root.is_leaf = false;
            new_root.n = 0;
            new_root.children[0] = rootPageId;
            writeNode(new_root_id, new_root);
            
            int old_root_id = rootPageId; 
            rootPageId = new_root_id;
            saveHeader();
            
            splitChild(new_root_id, 0, old_root_id);
            insertNonFull(new_root_id, key, ptr);
        } else {
            insertNonFull(rootPageId, key, ptr);
        }
    }
}

void IndexManager::splitChild(int parent_id, int i, int child_id) {
   DiskNode p = {}, y = {}, z = {};
    readNode(parent_id, p);
    readNode(child_id, y);
    int z_id = next_page_id++;
    z.is_leaf = y.is_leaf;
    z.n = 2;

    for (int j = 0; j < 2; j++) {
        z.keys[j] = y.keys[j + 3];
        z.pointers[j] = y.pointers[j + 3];
    }
    if (!y.is_leaf) {
        for (int j = 0; j < 3; j++) z.children[j] = y.children[j + 3];
    }
    y.n = 2;
    for (int j = p.n; j >= i + 1; j--) p.children[j + 1] = p.children[j];
    p.children[i + 1] = z_id;
    for (int j = p.n - 1; j >= i; j--) {
        p.keys[j + 1] = p.keys[j];
        p.pointers[j + 1] = p.pointers[j];
    }
    p.keys[i] = y.keys[2];
    p.pointers[i] = y.pointers[2];
    p.n++;
    writeNode(parent_id, p); writeNode(child_id, y); writeNode(z_id, z);
}

void IndexManager::insertNonFull(int page_id, const std::string& key, RecordPointer ptr) {
    DiskNode node = {};
    readNode(page_id, node);
    int i = node.n - 1;
    if (node.is_leaf) {
        while (i >= 0 && key < std::string(node.keys[i].value)) {
            node.keys[i + 1] = node.keys[i];
            node.pointers[i + 1] = node.pointers[i];
            i--;
        }
        std::strncpy(node.keys[i + 1].value, key.c_str(), 63);
        node.keys[i + 1].value[63] = '\0';
        node.pointers[i + 1] = ptr;
        node.n++;
        writeNode(page_id, node);
    } else {
        while (i >= 0 && key < std::string(node.keys[i].value)) i--;
        i++;
        DiskNode child = {};
        readNode(node.children[i], child);
        if (child.n == 5) {
            splitChild(page_id, i, node.children[i]);
            readNode(page_id, node);
            if (key > std::string(node.keys[i].value)) i++;
        }
        insertNonFull(node.children[i], key, ptr);
    }
}

RecordPointer IndexManager::search(int page_id, const std::string& key) {
    if (page_id == -1) return {-1, -1}; // No encontrado

    DiskNode node = {};
    readNode(page_id, node);

    int i = 0;
    while (i < node.n && key > std::string(node.keys[i].value)) {
        i++;
    }

    if (i < node.n && key == std::string(node.keys[i].value)) {
        return node.pointers[i]; // ¡Encontrado!
    }

    if (node.is_leaf) {
        return {-1, -1}; // No está en el árbol
    }

    return search(node.children[i], key); // Buscar en el hijo
}

// Sobrecarga pública para facilitar el uso
RecordPointer IndexManager::findOne(const std::string& key) {
    return findOneRecursive(rootPageId, key);
}

RecordPointer IndexManager::findOneRecursive(int page_id, const std::string& key) {
    if (page_id == -1) return {-1, -1};

    DiskNode node = {};
    readNode(page_id, node);

    int i = 0;
    while (i < node.n && key > std::string(node.keys[i].value)) {
        i++;
    }

    if (i < node.n && key == std::string(node.keys[i].value)) {
        return node.pointers[i];
    }

    if (node.is_leaf) {
        return {-1, -1};
    }

    return findOneRecursive(node.children[i], key);
}