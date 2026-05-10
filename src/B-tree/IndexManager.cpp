#include "../../include/B-tree/IndexManager.h"
#include <iostream>
#include <cstring>

// Constructor: Gestiona la apertura o creación del archivo de índices (.idx)
IndexManager::IndexManager(StorageManager* sm, int degree) 
    : storage(sm), t(degree) 
{
    // Le pedimos al storage la página 0 donde vive la cabecera
    std::vector<char> buffer = storage->readIndexPage(0);

    // Verificamos si es un archivo nuevo (buffer vacío o con ceros)
    bool is_new = true;
    if (!buffer.empty()) {
        for (int i = 0; i < 12; i++) {
            if (buffer[i] != 0) {
                is_new = false;
                break;
            }
        }
    }

    if (is_new) {
        rootPageId = -1;  
        next_page_id = 1; 
        saveHeader();
    } else {
        loadHeader();
    }
}

void IndexManager::loadHeader() {
    std::vector<char> buffer = storage->readIndexPage(0);
    if (buffer.size() >= 12) {
        memcpy(&rootPageId, &buffer[0], sizeof(int));
        memcpy(&next_page_id, &buffer[4], sizeof(int));
        memcpy(&t, &buffer[8], sizeof(int));
    }
}

void IndexManager::saveHeader() {
    std::vector<char> buffer(4096, 0); // PAGE_SIZE estándar
    memcpy(&buffer[0], &rootPageId, sizeof(int));
    memcpy(&buffer[4], &next_page_id, sizeof(int));
    memcpy(&buffer[8], &t, sizeof(int));
    storage->writeIndexPage(0, buffer);
}

void IndexManager::writeNode(int id, const DiskNode& node) {
    std::vector<char> buffer(4096, 0);
    memcpy(buffer.data(), &node, sizeof(DiskNode));
    storage->writeIndexPage(id, buffer);
}

void IndexManager::readNode(int id, DiskNode& node) {
    std::vector<char> buffer = storage->readIndexPage(id);
    if (!buffer.empty()) {
        memcpy(&node, buffer.data(), sizeof(DiskNode));
    }
}

void IndexManager::insert(int k, RecordPointer p) {
    if (rootPageId == -1) {
        // Primer nodo del árbol
        rootPageId = next_page_id++;
        DiskNode root;
        root.is_leaf = true;
        root.n = 1;
        root.keys[0] = k;
        root.pointers[0] = p;
        writeNode(rootPageId, root);
        saveHeader();
    } else {
        DiskNode root;
        readNode(rootPageId, root);

        // Si la raíz está llena, el árbol crece hacia arriba
        if (root.n == 2 * t - 1) {
            int new_root_id = next_page_id++;
            DiskNode new_root;
            new_root.is_leaf = false;
            new_root.n = 0;
            new_root.children[0] = rootPageId;
            
            writeNode(new_root_id, new_root);
            splitChild(new_root_id, 0, rootPageId);
            
            rootPageId = new_root_id;
            saveHeader();
            insertNonFull(rootPageId, k, p);
        } else {
            insertNonFull(rootPageId, k, p);
        }
    }
}

// FUNCIÓN QUE EVITA LOS DUPLICADOS Y PERMITE EL UPDATE
void IndexManager::insertNonFull(int page_id, int k, RecordPointer p_data) {
    DiskNode node;
    readNode(page_id, node);
    
    // Buscar la posición donde debería estar la llave
    int i = 0;
    while (i < node.n && k > node.keys[i]) i++;

    // CASO 1: LA LLAVE YA EXISTE (UPDATE)
    // Si encontramos el ID, simplemente actualizamos el puntero al archivo .bin
    if (i < node.n && node.keys[i] == k) {
        node.pointers[i] = p_data;
        writeNode(page_id, node);
        return; 
    }

    // CASO 2: ES NODO HOJA (INSERTAR NUEVO)
    if (node.is_leaf) {
        // Desplazamos elementos para abrir espacio
        for (int j = node.n - 1; j >= i; j--) {
            node.keys[j + 1] = node.keys[j];
            node.pointers[j + 1] = node.pointers[j];
        }
        node.keys[i] = k;
        node.pointers[i] = p_data;
        node.n++;
        writeNode(page_id, node); 
    } 
    // CASO 3: NODO INTERNO (BAJAR EN EL ÁRBOL)
    else {
        DiskNode child;
        readNode(node.children[i], child);

        // Si el hijo está lleno, hay que dividirlo
        if (child.n == 2 * t - 1) {
            splitChild(page_id, i, node.children[i]);
            readNode(page_id, node); 

            // Después del split, la llave mediana subió. Verificamos si es nuestra llave.
            if (node.keys[i] == k) {
                node.pointers[i] = p_data;
                writeNode(page_id, node);
                return;
            }
            if (node.keys[i] < k) i++;
        }
        insertNonFull(node.children[i], k, p_data);
    }
}

void IndexManager::splitChild(int parent_id, int i, int child_id) {
    DiskNode p, y, z;
    readNode(parent_id, p);
    readNode(child_id, y);

    int z_id = next_page_id++;
    z.is_leaf = y.is_leaf;
    z.n = t - 1;

    // Pasar la mitad superior de llaves y punteros a Z
    for (int j = 0; j < t - 1; j++) {
        z.keys[j] = y.keys[j + t];
        z.pointers[j] = y.pointers[j + t];
    }

    if (!y.is_leaf) {
        for (int j = 0; j < t; j++) {
            z.children[j] = y.children[j + t];
        }
    }

    y.n = t - 1;

    // Mover hijos en el padre
    for (int j = p.n; j >= i + 1; j--) {
        p.children[j + 1] = p.children[j];
    }
    p.children[i + 1] = z_id;

    // Mover llaves en el padre
    for (int j = p.n - 1; j >= i; j--) {
        p.keys[j + 1] = p.keys[j];
        p.pointers[j + 1] = p.pointers[j];
    }

    // Subir la llave mediana al padre
    p.keys[i] = y.keys[t - 1];
    p.pointers[i] = y.pointers[t - 1];
    p.n++;

    writeNode(child_id, y);
    writeNode(z_id, z);
    writeNode(parent_id, p);
}

RecordPointer IndexManager::search(int k) {
    if (rootPageId == -1) return {-1, -1};
    return searchRecursive(rootPageId, k);
}

RecordPointer IndexManager::searchRecursive(int page_id, int k) {
    DiskNode node;
    readNode(page_id, node);

    int i = 0;
    while (i < node.n && k > node.keys[i]) i++;

    if (i < node.n && node.keys[i] == k) {
        return node.pointers[i];
    }

    if (node.is_leaf) {
        return {-1, -1};
    }

    return searchRecursive(node.children[i], k);
}