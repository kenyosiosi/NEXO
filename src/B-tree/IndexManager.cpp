#include "../include/B-tree/IndexManager.h"
#include <algorithm>

IndexManager::IndexManager(const std::string& index_path, int degree)
    : indexFile(index_path), rootPageId(-1), next_page_id(1), t(3), max_keys(5) {
    std::vector<char> buffer = indexFile.readPage(0);

    if (buffer.empty() || (buffer[0] == 0 && buffer[4] == 0)) {
        // Archivo nuevo: fijar el grado solicitado y escribir la cabecera.
        if (degree >= 2 && degree <= kMaxDegree) t = degree;
        max_keys = 2 * t - 1;
        saveHeader();
        return;
    }

    // Archivo existente: el grado manda (el parámetro se ignora).
    loadHeader();
    if (t < 2 || t > kMaxDegree) {
        // Cabecera corrupta o de una versión anterior: caer al grado por
        // defecto y reescribir la cabecera para dejarla en un estado válido.
        t = 3;
        max_keys = 2 * t - 1;
        rootPageId = -1;
        next_page_id = 1;
        saveHeader();
        return;
    }
    max_keys = 2 * t - 1;
}

void IndexManager::saveHeader() {
    std::vector<char> buffer(PAGE_SIZE, 0);
    std::memcpy(&buffer[0], &rootPageId, sizeof(int));
    std::memcpy(&buffer[4], &next_page_id, sizeof(int));
    std::memcpy(&buffer[8], &t, sizeof(int));
    indexFile.writePage(0, buffer);
}

void IndexManager::loadHeader() {
    std::vector<char> buffer = indexFile.readPage(0);
    if (buffer.empty()) return;
    std::memcpy(&rootPageId, &buffer[0], sizeof(int));
    std::memcpy(&next_page_id, &buffer[4], sizeof(int));
    std::memcpy(&t, &buffer[8], sizeof(int));
}

int IndexManager::allocatePage() {
    const int id = next_page_id++;
    // Persistir el contador inmediatamente: si el proceso termina justo
    // después de un split, un next_page_id desactualizado haría que un
    // reinicio reutilice páginas ya ocupadas y corrompa el árbol.
    saveHeader();
    return id;
}

IndexManager::DiskNode IndexManager::makeEmptyNode() const {
    DiskNode node;
    node.is_leaf = true;
    node.n = 0;
    node.keys.assign(max_keys, IndexKey{});
    node.pointers.assign(max_keys, RecordPointer{ -1, -1 });
    node.children.assign(2 * t, -1);
    return node;
}

IndexKey IndexManager::makeKey(const std::string& key) {
    IndexKey k{};
    std::strncpy(k.value, key.c_str(), sizeof(k.value) - 1);
    k.value[sizeof(k.value) - 1] = '\0';
    return k;
}

void IndexManager::writeNode(int id, const DiskNode& node) {
    std::vector<char> buffer(PAGE_SIZE, 0);
    buffer[0] = node.is_leaf ? 1 : 0;
    std::memcpy(&buffer[4], &node.n, sizeof(int));

    size_t offset = 8;
    const size_t key_bytes = sizeof(IndexKey::value);

    for (int i = 0; i < node.n && offset + key_bytes <= buffer.size(); ++i) {
        std::memcpy(&buffer[offset], node.keys[i].value, key_bytes);
        offset += key_bytes;
    }
    for (int i = 0; i < node.n && offset + sizeof(RecordPointer) <= buffer.size(); ++i) {
        std::memcpy(&buffer[offset], &node.pointers[i], sizeof(RecordPointer));
        offset += sizeof(RecordPointer);
    }
    for (int i = 0; i <= node.n && offset + sizeof(int) <= buffer.size(); ++i) {
        // Solo los nodos internos usan hijos; en hojas se dejan a -1.
        if (!node.is_leaf) std::memcpy(&buffer[offset], &node.children[i], sizeof(int));
        offset += sizeof(int);
    }

    indexFile.writePage(id, buffer);
}

IndexManager::DiskNode IndexManager::readNode(int id) {
    DiskNode node = makeEmptyNode();
    std::vector<char> buffer = indexFile.readPage(id);
    if (buffer.empty()) return node;

    node.is_leaf = buffer[0] != 0;
    std::memcpy(&node.n, &buffer[4], sizeof(int));
    node.n = std::max(0, std::min(node.n, max_keys));

    size_t offset = 8;
    const size_t key_bytes = sizeof(IndexKey::value);

    for (int i = 0; i < node.n && offset + key_bytes <= buffer.size(); ++i) {
        std::memcpy(node.keys[i].value, &buffer[offset], key_bytes);
        node.keys[i].value[key_bytes - 1] = '\0';
        offset += key_bytes;
    }
    for (int i = 0; i < node.n && offset + sizeof(RecordPointer) <= buffer.size(); ++i) {
        std::memcpy(&node.pointers[i], &buffer[offset], sizeof(RecordPointer));
        offset += sizeof(RecordPointer);
    }
    for (int i = 0; i <= node.n && offset + sizeof(int) <= buffer.size(); ++i) {
        if (!node.is_leaf) std::memcpy(&node.children[i], &buffer[offset], sizeof(int));
        offset += sizeof(int);
    }

    return node;
}

void IndexManager::insert(const std::string& key, RecordPointer ptr) {
    if (rootPageId == -1) {
        rootPageId = allocatePage();
        DiskNode root = makeEmptyNode();
        root.is_leaf = true;
        root.n = 1;
        root.keys[0] = makeKey(key);
        root.pointers[0] = ptr;
        writeNode(rootPageId, root);
        saveHeader();
        return;
    }

    DiskNode root = readNode(rootPageId);
    if (root.n == max_keys) {
        const int old_root_id = rootPageId;
        const int new_root_id = allocatePage();

        DiskNode new_root = makeEmptyNode();
        new_root.is_leaf = false;
        new_root.n = 0;
        new_root.children[0] = old_root_id;

        rootPageId = new_root_id;
        writeNode(new_root_id, new_root);
        saveHeader();

        splitChild(new_root_id, 0, old_root_id);
        insertNonFull(new_root_id, key, ptr);
    } else {
        insertNonFull(rootPageId, key, ptr);
    }
}

void IndexManager::splitChild(int parent_id, int i, int child_id) {
    DiskNode parent = readNode(parent_id);
    DiskNode child = readNode(child_id);

    const int z_id = allocatePage();
    DiskNode z = makeEmptyNode();

    z.is_leaf = child.is_leaf;
    z.n = t - 1;

    // La mitad derecha del hijo lleno (claves t..2t-2) migra al nodo nuevo;
    // la clave t-1 sube al padre.
    for (int j = 0; j < t - 1; ++j) {
        z.keys[j] = child.keys[j + t];
        z.pointers[j] = child.pointers[j + t];
    }
    if (!child.is_leaf) {
        for (int j = 0; j < t; ++j) z.children[j] = child.children[j + t];
    }

    child.n = t - 1;

    // Abrir hueco en el padre para el nuevo hijo y la clave promovida.
    for (int j = parent.n; j >= i + 1; --j) parent.children[j + 1] = parent.children[j];
    parent.children[i + 1] = z_id;
    for (int j = parent.n - 1; j >= i; --j) {
        parent.keys[j + 1] = parent.keys[j];
        parent.pointers[j + 1] = parent.pointers[j];
    }
    parent.keys[i] = child.keys[t - 1];
    parent.pointers[i] = child.pointers[t - 1];
    parent.n++;

    writeNode(parent_id, parent);
    writeNode(child_id, child);
    writeNode(z_id, z);
}

void IndexManager::insertNonFull(int page_id, const std::string& key, RecordPointer ptr) {
    DiskNode node = readNode(page_id);
    int i = node.n - 1;

    if (node.is_leaf) {
        while (i >= 0 && key < keyAt(node.keys[i])) {
            node.keys[i + 1] = node.keys[i];
            node.pointers[i + 1] = node.pointers[i];
            --i;
        }
        node.keys[i + 1] = makeKey(key);
        node.pointers[i + 1] = ptr;
        node.n++;
        writeNode(page_id, node);
        return;
    }

    while (i >= 0 && key < keyAt(node.keys[i])) --i;
    ++i;

    DiskNode child = readNode(node.children[i]);
    if (child.n == max_keys) {
        splitChild(page_id, i, node.children[i]);
        // El split movió claves dentro del padre: releerlo antes de decidir
        // por qué lado continuar.
        node = readNode(page_id);
        if (key > keyAt(node.keys[i])) ++i;
    }
    insertNonFull(node.children[i], key, ptr);
}

std::vector<RecordPointer> IndexManager::search(const std::string& key) {
    std::vector<RecordPointer> results;
    if (rootPageId != -1) searchRecursive(rootPageId, key, results);
    return results;
}

void IndexManager::searchRecursive(int page_id, const std::string& key, std::vector<RecordPointer>& results) {
    const DiskNode node = readNode(page_id);

    int i = 0;
    while (i < node.n && key > keyAt(node.keys[i])) ++i;

    // Todas las entradas iguales a la clave buscada (permite duplicados).
    int last = i;
    while (last < node.n && key == keyAt(node.keys[last])) ++last;
    for (int j = i; j < last; ++j) results.push_back(node.pointers[j]);

    if (!node.is_leaf) {
        // Con duplicados la clave puede continuar en varios subárboles.
        for (int j = i; j <= last; ++j) {
            searchRecursive(node.children[j], key, results);
        }
    }
}

RecordPointer IndexManager::findOne(const std::string& key) {
    if (rootPageId == -1) return { -1, -1 };
    return findOneRecursive(rootPageId, key);
}

RecordPointer IndexManager::findOneRecursive(int page_id, const std::string& key) {
    if (page_id == -1) return { -1, -1 };

    const DiskNode node = readNode(page_id);

    int i = 0;
    while (i < node.n && key > keyAt(node.keys[i])) ++i;

    if (i < node.n && key == keyAt(node.keys[i])) return node.pointers[i];

    if (node.is_leaf) return { -1, -1 };

    return findOneRecursive(node.children[i], key);
}
