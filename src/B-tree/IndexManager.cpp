#include "../../include/B-tree/IndexManager.h"
#include "../../include/Storage/StorageManager.h"
#include <iostream>
#include <fstream>

IndexManager::IndexManager(const std::string& path, int degree) 
    : indexFile(path), t(degree) {
    
    // Verificamos si el archivo es nuevo (tamaño 0 o no existe)
    std::ifstream check(path, std::ios::binary | std::ios::ate);
    if (!check.is_open() || check.tellg() <= 0) {
        // ES UN ARCHIVO NUEVO: Inicializamos la cabecera
        rootPageId = -1;  // -1 significa que el árbol está vacío
        next_page_id = 1; // La página 0 es la cabecera, los datos empiezan en la 1
        saveHeader();
    } else {
        // EL ARCHIVO YA EXISTE: Cargamos dónde se quedó la raíz la última vez
        loadHeader();
    }
}

void IndexManager::loadHeader() {
    std::vector<char> buffer = indexFile.readPage(0);
    // Usamos memcpy para extraer los valores de los primeros bytes
    memcpy(&rootPageId, &buffer[0], sizeof(int));
    memcpy(&next_page_id, &buffer[4], sizeof(int));
    memcpy(&t, &buffer[8], sizeof(int));
}

void IndexManager::saveHeader() {
    std::vector<char> buffer(4096, 0); // Creamos una página vacía
    // Metemos los datos en los primeros bytes del buffer
    memcpy(&buffer[0], &rootPageId, sizeof(int));
    memcpy(&buffer[4], &next_page_id, sizeof(int));
    memcpy(&buffer[8], &t, sizeof(int));
    indexFile.writePage(0, buffer); // Guardamos en la página 0
}

void IndexManager::readNode(int page_id, DiskNode& node) {
    std::vector<char> buffer = indexFile.readPage(page_id);
    memcpy(&node, buffer.data(), sizeof(DiskNode));
}

void IndexManager::writeNode(int page_id, const DiskNode& node) {
    std::vector<char> buffer(4096, 0);
    memcpy(buffer.data(), &node, sizeof(DiskNode));
    indexFile.writePage(page_id, buffer);
}

void IndexManager::insert(int k, RecordPointer p) {
    if (rootPageId == -1) {
        // Árbol vacío: Creamos la primera raíz
        DiskNode root;
        root.is_leaf = true;
        root.n = 1;
        root.keys[0] = k;
        root.pointers[0] = p;
        
        rootPageId = next_page_id++;
        writeNode(rootPageId, root);
        saveHeader(); // Actualizamos la Página 0 porque la raíz cambió
    } else {
        DiskNode root;
        readNode(rootPageId, root);

        if (root.n == 2 * t - 1) {
            // La raíz está llena, hay que dividirla
            DiskNode s;
            s.is_leaf = false;
            s.n = 0;
            s.children[0] = rootPageId;

            int newRootId = next_page_id++;
            writeNode(newRootId, s); // <--- AÑADE ESTA LÍNEA AQUÍ
            splitChild(newRootId, 0, rootPageId);
            
            rootPageId = newRootId;
            insertNonFull(rootPageId, k, p);
            saveHeader(); // La raíz cambió, actualizamos la Página 0
        } else {
            insertNonFull(rootPageId, k, p);
        }
    }
}

void IndexManager::splitChild(int parent_id, int i, int child_id) {
    DiskNode z, y, p;
    readNode(child_id, y);
    readNode(parent_id, p);

    z.is_leaf = y.is_leaf;
    z.n = t - 1;

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

    for (int j = p.n; j >= i + 1; j--) {
        p.children[j + 1] = p.children[j];
    }

    int z_id = next_page_id++;
    p.children[i + 1] = z_id;

    for (int j = p.n - 1; j >= i; j--) {
        p.keys[j + 1] = p.keys[j];
        p.pointers[j + 1] = p.pointers[j];
    }

    p.keys[i] = y.keys[t - 1];
    p.pointers[i] = y.pointers[t - 1];
    p.n++;

    // Guardamos los 3 nodos modificados en el disco
    writeNode(child_id, y);
    writeNode(z_id, z);
    writeNode(parent_id, p);
}

void IndexManager::insertNonFull(int page_id, int k, RecordPointer p_data) {
    DiskNode node;
    readNode(page_id, node);
    int i = node.n - 1;

    if (node.is_leaf) {
        while (i >= 0 && node.keys[i] > k) {
            node.keys[i + 1] = node.keys[i];
            node.pointers[i + 1] = node.pointers[i];
            i--;
        }
        node.keys[i + 1] = k;
        node.pointers[i + 1] = p_data;
        node.n++;
        writeNode(page_id, node); // Guardamos los cambios
    } else {
        while (i >= 0 && node.keys[i] > k) i--;
        i++;
        DiskNode child;
        readNode(node.children[i], child);
        if (child.n == 2 * t - 1) {
            splitChild(page_id, i, node.children[i]);
            readNode(page_id, node); // Recargar padre después del split
            if (node.keys[i] < k) i++;
        }
        insertNonFull(node.children[i], k, p_data);
    }
}

RecordPointer IndexManager::search(int k) {
    if (rootPageId == -1) return {-1, -1}; // No hay nada insertado
    return searchRecursive(rootPageId, k);
}

RecordPointer IndexManager::searchRecursive(int page_id, int k) {
    DiskNode node;
    readNode(page_id, node);

    int i = 0;
    while (i < node.n && k > node.keys[i]) i++;

    // Si la encontramos, retornamos el puntero (page_id y offset del HeapFile)
    if (i < node.n && node.keys[i] == k) return node.pointers[i];

    if (node.is_leaf) return {-1, -1}; // Llegamos a una hoja y no estaba

    // Seguimos bajando por el árbol, leyendo la página del hijo correspondiente
    return searchRecursive(node.children[i], k);
}