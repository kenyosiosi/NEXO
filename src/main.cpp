#include <iostream>
#include <vector>
#include <map>
#include "../include/tokenizer.h"
#include "../include/parser.h"
#include "../include/StorageManager.h"
#include "../include/IndexManager.h"

//PRUEBA MANUAL DE LA BASE DE DATOS SEGURO LA BORRO DESPUES
// void pruebaPersistencia() {
//     // // 1. PRIMERA EJECUCIÓN (Inserción)
//     // {
//     //     IndexManager index("data/usuarios.idx", 3); // Grado pequeño para forzar splits
//     //     RecordPointer ptr = {1, 10}; // Simulamos un puntero al archivo .bin
//     //     index.insert(2300, ptr);
//     //     index.insert(50, {1, 20});
//     //     index.insert(150, {2, 5});
//     //     std::cout << "Datos insertados. Cerrando sistema..." << std::endl;
//     // } 
//     // Al salir del bloque {}, el objeto index se destruye y el archivo se cierra.

//     // 2. SEGUNDA EJECUCIÓN (Recuperación)
//     {
//         std::cout << "Reiniciando sistema... Leyendo desde disco." << std::endl;
//         IndexManager index("data/usuarios.idx", 3);
//         int porfaagarra=2300;
//         RecordPointer result = index.search(porfaagarra);
        
//         if (result.page_id != -1) {
//             std::cout << "exito Se encontro el ID "<<porfaagarra<<" en la pagina " << result.page_id << std::endl;
//         } else {
//             std::cout << "Error: No se encontró el registro." << std::endl;
//         }
//     }
// }

int main() {
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
}