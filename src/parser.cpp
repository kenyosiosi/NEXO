#include "../include/parser.h"
#include <iostream>

Parser::Parser(const std::vector<Token>& tokens) : tokens(tokens), pos(0) {}

void Parser::parse() {
    if (tokens.empty()) return;

    Token t = peek();
    if (match(TokenType::INSERT)) {
        parseInsert();
    } else if (match(TokenType::GET)) {
        parseGet();
    } else if (match(TokenType::DELETE)) {
        parseDelete();
    } else if (match(TokenType::UPDATE)) { // <--- Nueva ruta para Update
        parseUpdate();
    } else if (match(TokenType::CREATE)) {
        parseCreate();
    } else if (t.type == TokenType::END_OF_FILE) {
        return;
    } else {
        throw std::runtime_error("Comando no reconocido: " + t.lexeme);
    }
}

void Parser::parseInsert() {
    parseJson();
    std::cout << "!Sintaxis de INSERT validada con exito!" << std::endl;
}

void Parser::parseGet() {
    parseJson();
    std::cout << "!Sintaxis de GET validada con exito!" << std::endl;
}

void Parser::parseDelete() {
    parseJson();
    std::cout << "!Sintaxis de DELETE validada con exito!" << std::endl;
}

void Parser::parseUpdate() {
    parseJson(); // El update necesita el ID para saber que registro cambiar
    std::cout << "!Sintaxis de UPDATE validada con exito!" << std::endl;
}

void Parser::parseCreate() {
    Token nameToken = advance();
    if (nameToken.type != TokenType::STRING) {
        throw std::runtime_error("Se esperaba el nombre de la tabla entre comillas");
    }
    std::cout << "CREANDO tabla/archivo: " << nameToken.lexeme << std::endl;
}

void Parser::parseJson() {
    consume(TokenType::LBRACE, "Se esperaba '{'");
    consume(TokenType::ID, "Se esperaba el campo 'id'");
    consume(TokenType::COLON, "Se esperaba ':'");

    Token idToken = advance();
    if (idToken.type != TokenType::NUMBER) {
        throw std::runtime_error("El valor del ID debe ser un numero");
    }

    std::cout << "ID detectado: " << idToken.lexeme << std::endl;
    consume(TokenType::RBRACE, "Se esperaba '}'");
}

// --- Funciones Auxiliares ---
Token Parser::advance() {
    if (pos < tokens.size()) return tokens[pos++];
    return tokens.back();
}

Token Parser::peek() {
    if (pos < tokens.size()) return tokens[pos];
    return tokens.back();
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