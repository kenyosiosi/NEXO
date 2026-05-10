#include "../../include/query_engine/parser.h"
#include <iostream>

Parser::Parser(const std::vector<Token>& tokens) : tokens(tokens), pos(0) {}

std::map<std::string, std::string> Parser::parse() {
    if (tokens.empty()) return {};

    Token t = peek();
    // Guarda el comando para saber qué operación hacer
    std::map<std::string, std::string> result;

    if (match(TokenType::INSERT)) {
        result = parseInsert();
        result["operation"] = "INSERT";
    } else if (match(TokenType::GET)) {
        result = parseGet();
        result["operation"] = "GET";
    } else if (match(TokenType::UPDATE)) {
        result = parseUpdate();
        result["operation"] = "UPDATE";
    } else if (match(TokenType::DELETE)) {
        result = parseDelete();
        result["operation"] = "DELETE";
    } else if (match(TokenType::CREATE)) {
        result = parseCreate();
        result["operation"] = "CREATE";
    } else {
        throw std::runtime_error("Comando desconocido: " + t.lexeme);
    }
    return result;
}

std::map<std::string, std::string> Parser::parseInsert() 
    {
    // Soportar: INSERT INTO "estudiantes" { ... }
    if (peek().lexeme == "INTO") {
        advance(); 
    }

    Token colToken = advance();
    if (colToken.type != TokenType::STRING) {
        throw std::runtime_error("Se esperaba el nombre de la coleccion (string)");
    }

    std::map<std::string, std::string> data = parseJson();
    data["_table_target"] = colToken.lexeme; 
    return data;
}

std::map<std::string, std::string> Parser::parseCreate() {
    std::map<std::string, std::string> data;
    
    // 1. Nombre de la colección
    Token nameToken = advance();
    if (nameToken.type != TokenType::STRING) 
        throw std::runtime_error("Se esperaba nombre de la coleccion");
    data["_table_target"] = nameToken.lexeme;

    // 2. Leer los campos obligatorios (Ej: ["id", "nombre"])
    consume(TokenType::LBRACKET, "Se esperaba '[' para definir los campos");
    
    std::string fields = "";
    while (peek().type != TokenType::RBRACKET) {
        Token fieldToken = advance();
        if (fieldToken.type != TokenType::STRING && fieldToken.type != TokenType::ID)
            throw std::runtime_error("Los nombres de campos deben ser strings");
        
        fields += fieldToken.lexeme + ",";
        
        if (!match(TokenType::COMMA)) break;
    }
    consume(TokenType::RBRACKET, "Se esperaba ']'");
    
    data["_fields"] = fields;
    return data;
}

std::map<std::string, std::string> Parser::parseGet() {
    // 1. Extraer el nombre de la colección 
    Token tableToken = advance(); 
    if (tableToken.type != TokenType::STRING) {
        throw std::runtime_error("Se esperaba el nombre de la coleccion para el GET");
    }

    // 2. Parsear el JSON del ID
    std::map<std::string, std::string> data = parseJson();
    
    // 3. Guardar el nombre de la tabla para el main
    data["_table_target"] = tableToken.lexeme; 
    return data;
}

std::map<std::string, std::string> Parser::parseUpdate() {
    Token tableToken = advance(); // Leer el nombre de la tabla primero
    if (tableToken.type != TokenType::STRING) {
        throw std::runtime_error("Se esperaba el nombre de la coleccion");
    }
    std::map<std::string, std::string> data = parseJson();
    data["_table_target"] = tableToken.lexeme; 
    return data;
}

std::map<std::string, std::string> Parser::parseDelete() {
    Token tableToken = advance(); 
    if (tableToken.type != TokenType::STRING) {
        throw std::runtime_error("Se esperaba el nombre de la coleccion para el DELETE");
    }

    std::map<std::string, std::string> data = parseJson();
    data["_table_target"] = tableToken.lexeme; 
    return data;
}

std::map<std::string, std::string> Parser::parseJson() {
    std::map<std::string, std::string> data;
    consume(TokenType::LBRACE, "Se esperaba '{'");

    // Bucle para leer múltiples pares llave:valor
    while (peek().type != TokenType::RBRACE) {
        // 1. Leer la llave
        Token keyToken = advance();
        if (keyToken.type != TokenType::ID && keyToken.type != TokenType::STRING) {
            throw std::runtime_error("Se esperaba una llave (string)");
        }

        consume(TokenType::COLON, "Se esperaba ':' tras la llave");

        // 2. Leer el valor
        if (peek().type == TokenType::LBRACE) {
            parseJson();
            data[keyToken.lexeme] = "[Nested Object]"; 
        } else {
            Token valueToken = advance();
            if (valueToken.type != TokenType::STRING && valueToken.type != TokenType::NUMBER) {
                throw std::runtime_error("Se esperaba un valor (string o numero)");
            }
            data[keyToken.lexeme] = valueToken.lexeme;
        }

        if (!match(TokenType::COMMA)) break;
    }
    

    consume(TokenType::RBRACE, "Se esperaba '}'");
    return data;
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