#include <iostream>
#include <vector>
#include "../include/tokenizer.h"
#include "../include/parser.h"

std::string tokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::CREATE: return "CREATE";
        case TokenType::INSERT: return "INSERT";
        case TokenType::GET:    return "GET";
        case TokenType::DELETE: return "DELETE";
        case TokenType::ID:     return "ID_KEYWORD";
        case TokenType::STRING: return "STRING";
        case TokenType::NUMBER: return "NUMBER";
        case TokenType::LBRACE: return "LBRACE ({)";
        case TokenType::RBRACE: return "RBRACE (})";
        case TokenType::COLON:  return "COLON (:)";
        case TokenType::COMMA:  return "COMMA (,)";
        case TokenType::UNKNOWN: return "UNKNOWN";
        case TokenType::END_OF_FILE: return "EOF";
        default: return "OTHER";
    }
}

int main() {
    std::string input;

    std::cout << "--- NEXO DATABASE SYSTEM ---" << std::endl;
    std::cout << "Escribe 'exit' para salir." << std::endl;

    while (true) {
        // 1. Mostrar un indicador de comando
        std::cout << "\nNEXO > ";
        
        // 2. Leer la línea completa de la terminal
        std::getline(std::cin, input);

        // 3. Opción para salir del programa
        if (input == "exit" || input == "EXIT") {
            break;
        }

        // 4. Si la entrada está vacía, saltar al siguiente ciclo
        if (input.empty()) continue;

        try {
            // Tokenizar lo que el usuario escribió
            Tokenizer tokenizer(input);
            std::vector<Token> tokens = tokenizer.tokenize();

            // Parsear los tokens generados
            Parser parser(tokens);
            parser.parse(); 

        } catch (const std::exception& e) {
            // Si el usuario escribe mal el JSON o el comando, aquí se atrapa el error
            std::cerr << "ERROR DE SINTAXIS: " << e.what() << std::endl;
        }
    }

    std::cout << "Cerrando sistema..." << std::endl;
    return 0;
}