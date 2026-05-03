#include "../../include/query_engine/parser.h"
#include <iostream>
#include <string>

Parser::Parser(const std::vector<Token>& tokens) : tokens(tokens), pos(0) {}

int Parser::parse() {
    if (tokens.empty()) return -1;

    Token t = peek();
    if (match(TokenType::INSERT)) {
        return parseInsert();
    } else if (match(TokenType::GET)) {
        return parseGet();
    } else if (match(TokenType::DELETE)) {
        return parseDelete();
    } else if (match(TokenType::UPDATE)) {
        return parseUpdate();
    } else if (match(TokenType::CREATE)) {
        parseCreate();
        return -1;
    } else if (t.type == TokenType::END_OF_FILE) {
        return -1;
    } else {
        throw std::runtime_error("Comando no reconocido: " + t.lexeme);
    }
}

int Parser::parseInsert() {
    int id = parseJson();
    std::cout << "!Sintaxis de INSERT validada con exito!" << std::endl;
    return id;
}

int Parser::parseGet() {
    int id = parseJson();
    std::cout << "!Sintaxis de GET validada con exito!" << std::endl;
    return id;
}

int Parser::parseDelete() {
    return parseJson();
}

int Parser::parseUpdate() {
    return parseJson();
}

void Parser::parseCreate() {
    Token nameToken = advance();
    if (nameToken.type != TokenType::STRING) {
        throw std::runtime_error("Se esperaba el nombre de la tabla entre comillas");
    }
    std::cout << "CREANDO tabla/archivo: " << nameToken.lexeme << std::endl;
}

int Parser::parseJson() {
    consume(TokenType::LBRACE, "Se esperaba '{'");
    consume(TokenType::ID, "Se esperaba el campo 'id'");
    consume(TokenType::COLON, "Se esperaba ':'");

    Token idToken = advance();
    if (idToken.type != TokenType::NUMBER) {
        throw std::runtime_error("El valor del ID debe ser un numero");
    }

    int idDetectado = std::stoi(idToken.lexeme);
    std::cout << "ID detectado: " << idDetectado << std::endl;
    
    consume(TokenType::RBRACE, "Se esperaba '}'");
    return idDetectado;
}

// --- FUNCIONES AUXILIARES ---

Token Parser::advance() {
    if (pos < tokens.size()) return tokens[pos++];
    return (tokens.empty() ? Token{TokenType::UNKNOWN, ""} : tokens.back());
}

Token Parser::peek() {
    if (pos < tokens.size()) return tokens[pos];
    return (tokens.empty() ? Token{TokenType::UNKNOWN, ""} : tokens.back());
}

bool Parser::match(TokenType type) {
    if (peek().type == type) {
        advance();
        return true;
    }
    return false;
}

void Parser::consume(TokenType type, const std::string& message) {
    if (peek().type == type) {
        advance();
    } else {
        throw std::runtime_error(message + " pero se encontro: " + peek().lexeme);
    }
}