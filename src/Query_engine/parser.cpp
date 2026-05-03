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

// retornan lo que parseJson extrae
std::map<std::string, std::string> Parser::parseInsert() { return parseJson(); }
std::map<std::string, std::string> Parser::parseGet()    { return parseJson(); }
std::map<std::string, std::string> Parser::parseUpdate() { return parseJson(); }
std::map<std::string, std::string> Parser::parseDelete() { return parseJson(); }

std::map<std::string, std::string> Parser::parseCreate() {
    std::map<std::string, std::string> data;
    Token nameToken = advance();
    if (nameToken.type != TokenType::STRING) throw std::runtime_error("Se esperaba nombre de tabla");
    data["table_name"] = nameToken.lexeme;
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
        Token valueToken = advance();
        if (valueToken.type != TokenType::STRING && valueToken.type != TokenType::NUMBER) {
            throw std::runtime_error("Se esperaba un valor (string o numero)");
        }

        // 3. Guardar en el mapa
        data[keyToken.lexeme] = valueToken.lexeme;

        // 4. Si hay una coma, seguimos; si no hay coma, el bucle termina si el sig es '}'
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