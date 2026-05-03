#include <iostream>
#include <vector>
#include <map>
#include "../include/query_engine/tokenizer.h"
#include "../include/query_engine/parser.h"
#include "../include/Storage/StorageManager.h"
#include "../include/B-tree/IndexManager.h"

/*int main() {
    // 1. Inicializar los motores (estos abrirán los archivos .bin e .idx)
    StorageManager engine("data/database.bin"); 
    IndexManager index("data/usuarios.idx", 3); // <--- USA EL INDEX MANAGER
    
    std::string input;
    std::cout << "--- NEXO DATABASE SYSTEM (Persistente) ---" << std::endl;

while (true) {
    std::cout << "\nNEXO > ";
    std::getline(std::cin, input);
    if (input == "exit") break;

    try {
        Tokenizer tokenizer(input);
        auto tokens = tokenizer.tokenize();
        Parser parser(tokens);
        
        // 1. EL PARSER DA EL ID REAL DE LA TERMINAL
        int id_usuario = parser.parse(); 

        if (tokens[0].type == TokenType::INSERT) {
            std::map<std::string, std::string> data;
            data["id"] = std::to_string(id_usuario);
            
            RecordPointer ptr = engine.insertRecord(data);
            index.insert(id_usuario, ptr); // Guardo el ID real en el .idx
            std::cout << "Guardado con exito!" << std::endl;
        } 
        else if (tokens[0].type == TokenType::GET) {
            // 2. BUSCA EL ID REAL QUE ESCRIBIÓ EL USUARIO
            RecordPointer ptr = index.search(id_usuario);
            
            if (ptr.page_id != -1) {
                std::cout << "ID " << id_usuario << " encontrado en Pag " << ptr.page_id << std::endl;
            } else {
                std::cout << "El ID " << id_usuario << " NO existe en el indice." << std::endl;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
    }
}
    return 0;
}*/

int main() {
    std::string input;
    std::cout << "--- NEXO DATABASE SYSTEM ---" << std::endl;

    while (true) {
        std::cout << "\nNEXO > ";
        std::getline(std::cin, input);

        if (input == "exit") break;
        if (input.empty()) continue;

        try {
            // 1. Tokenización
            Tokenizer tokenizer(input);
            std::vector<Token> tokens = tokenizer.tokenize();

            // 2. Parsing 
            Parser parser(tokens);
            std::map<std::string, std::string> data = parser.parse();

            // 3. Mostrar resultados
            std::cout << "Operacion detectada: " << data["operation"] << std::endl;
            std::cout << "Datos extraidos:" << std::endl;
            
            for (auto const& [key, val] : data) {
                if (key != "operation") {
                    std::cout << "  [" << key << "] : " << val << std::endl;
                }
            }

        } catch (const std::exception& e) {
            std::cerr << "ERROR DE SINTAXIS: " << e.what() << std::endl;
        }
    }

    return 0;
}