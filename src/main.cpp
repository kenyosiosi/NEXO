#include <iostream>
#include <vector>
#include <map>
#include "../include/query_engine/tokenizer.h"
#include "../include/query_engine/parser.h"
#include "../include/Storage/StorageManager.h"
#include "../include/B-tree/IndexManager.h"

int main() {

//  StorageManager ahora centraliza AMBOS archivos (.bin y .idx)
StorageManager storage("data/nexo.bin", "data/nexo.idx"); 

//  IndexManager ya no abre archivos, ahora recibe el puntero del storage
IndexManager index(&storage, 3);
    
    std::cout << "--- NEXO DATABASE SYSTEM ---" << std::endl;

    while (true) {
        std::string input;
        std::cout << "\nNEXO > ";
        std::getline(std::cin, input);

        if (input == "exit") break;
        if (input.empty()) continue;

        try {
            Tokenizer tokenizer(input);
            std::vector<Token> tokens = tokenizer.tokenize();
            Parser parser(tokens);
            
            // El Parser devuelve el mapa de la consulta
            std::map<std::string, std::string> queryData = parser.parse();
            std::string op = queryData["operation"];

            if (op == "INSERT") {
                int id = std::stoi(queryData["id"]); 

                // 1. El StorageManager guarda el JSON y nos da el "puntero" (página y slot)
                RecordPointer ptr = storage.insertRecord(queryData); 

                // 2. El IndexManager guarda el ID y ese puntero en el B-Tree
                index.insert(id, ptr); 
                
                std::cout << "Registro " << id << " insertado correctamente." << std::endl;
            } 
            else if (op == "GET") {
                int id_a_buscar = std::stoi(queryData["id"]);

                // 1. Buscamos en el índice para obtener la ubicación física
                RecordPointer ptr = index.search(id_a_buscar);

                if (ptr.page_id != -1) {
                    // 2. Le pedimos al StorageManager que lea y deserialice esa ubicación
                    std::map<std::string, std::string> result = storage.getRecord(ptr);

                    std::cout << "--- Registro Encontrado ---" << std::endl;
                    for (auto const& [key, val] : result) {
                        if (key != "operation") { // No mostrar la operación interna
                            std::cout << key << ": " << val << std::endl;
                        }
                    }
                } else {
                    std::cout << "Error: El ID " << id_a_buscar << " no existe en la base de datos." << std::endl;
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Error de ejecución: " << e.what() << std::endl;
        }
    }
    return 0;
}