#include "../include/parser.h"
#include <iostream>

Parser::Parser(const std::vector<Token>& tokens) : tokens(tokens), pos(0) {}

void Parser::parse() {
    Token current = peek();
    
    if (current.type == TokenType::INSERT) {
        parseInsert();
    } else {
        throw std::runtime_error("Error: Comando no reconocido.");
    }
    
    std::cout << "!Sintaxis validada con exito!" << std::endl;
}

void Parser::parseInsert() {
    advance();
    consume(TokenType::LBRACE, "Se esperaba '{'");

    int idEncontrado = -1;

    // Recorremos el JSON buscando el campo "id"
    while (peek().type != TokenType::RBRACE && peek().type != TokenType::END_OF_FILE) {
        Token t = advance();
        
        if (t.type == TokenType::ID) {
            consume(TokenType::COLON, "Se esperaba ':' despues de 'id'");
            if (peek().type == TokenType::NUMBER) {
                idEncontrado = std::stoi(advance().value);
            }
        }
    }

    consume(TokenType::RBRACE, "Se esperaba '}'");

    if (idEncontrado != -1) {
        std::cout << "ID extraido con exito: " << idEncontrado << std::endl;
    } else {
        std::cout << "Advertencia: No se encontro un campo 'id' numerico." << std::endl;
    }
}

// --- UTILIDADES ---

// Mira el token actual sin avanzar
Token Parser::peek() {
    return tokens[pos];
}

// Devuelve el token actual y avanza a la siguiente posición
Token Parser::advance() {
    if (pos < tokens.size()) pos++;
    return tokens[pos - 1];
}

// Obliga a que el token sea de un tipo, si no, lanza error
void Parser::consume(TokenType type, const std::string& message) {
    if (peek().type == type) {
        advance();
    } else {
        throw std::runtime_error(message);
    }
}