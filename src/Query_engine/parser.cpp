#include "../../include/Query_engine/parser.h"
#include <stdexcept>
#include <iostream>

Parser::Parser(const std::vector<Token>& tokens) : tokens(tokens), pos(0) {}

std::map<std::string, std::string> Parser::parse() {
    if (tokens.empty()) return {};
    Token t = peek();
    std::map<std::string, std::string> result;

    if (match(TokenType::INSERT)) {
        result = parseInsert();
        result["operation"] = "INSERT";
    } else if (match(TokenType::GET)) {
        result = parseGet();
        result["operation"] = "GET";
    } else if (match(TokenType::CREATE)) {
        result = parseCreate();
        if (result.find("operation") == result.end()) result["operation"] = "CREATE";
    } else if (match(TokenType::DELETE)) {
        result = parseDelete();
        result["operation"] = "DELETE";
    } else if (match(TokenType::UPDATE)) {
        result = parseUpdate();
        result["operation"] = "UPDATE";
    } else {
        throw std::runtime_error("Comando no reconocido: " + t.lexeme);
    }
    return result;
}

std::map<std::string, std::string> Parser::parseInsert() {
    std::map<std::string, std::string> data;
    if (peek().type == TokenType::INTO) advance();
    Token table = consume(TokenType::STRING, "Se esperaba nombre de tabla");
    data["_table_target"] = table.lexeme;
    return parseJson(data);
}

std::map<std::string, std::string> Parser::parseGet() {
    std::map<std::string, std::string> data;
    if (peek().type == TokenType::FROM) advance();
    Token table = consume(TokenType::STRING, "Se esperaba nombre de tabla");
    data["_table_target"] = table.lexeme;
    return parseJson(data);
}

std::map<std::string, std::string> Parser::parseDelete() {
    std::map<std::string, std::string> data;
    if (peek().type == TokenType::FROM) advance();
    Token table = consume(TokenType::STRING, "Se esperaba nombre de tabla");
    data["_table_target"] = table.lexeme;
    if (peek().type == TokenType::WHERE) advance();
    return parseJson(data);
}

std::map<std::string, std::string> Parser::parseUpdate() {
    std::map<std::string, std::string> data;
    Token table = consume(TokenType::STRING, "Se esperaba nombre de tabla");
    data["_table_target"] = table.lexeme;
    
    consume(TokenType::SET, "Se esperaba comando SET");
    std::map<std::string, std::string> new_data;
    parseJson(new_data);
    for(auto const& [k, v] : new_data) data["_set_" + k] = v;

    if (peek().type == TokenType::WHERE) advance();
    std::map<std::string, std::string> where_data;
    parseJson(where_data);
    for(auto const& [k, v] : where_data) data[k] = v;

    return data;
}

std::map<std::string, std::string> Parser::parseCreate() {
    std::map<std::string, std::string> data;
    if (match(TokenType::INDEX)) {
        Token field = consume(TokenType::STRING, "Se esperaba campo a indexar");
        consume(TokenType::ON, "Se esperaba 'ON'");
        Token table = consume(TokenType::STRING, "Se esperaba nombre de tabla");
        data["operation"] = "CREATE_INDEX";
        data["_table_target"] = table.lexeme;
        data["field"] = field.lexeme;
    } else {
        Token table = consume(TokenType::STRING, "Se esperaba nombre de coleccion");
        data["_table_target"] = table.lexeme;
        std::map<std::string, std::string> body;
        parseJson(body);
        if (body.count("fields")) data["_fields"] = body["fields"];
    }
    return data;
}

std::map<std::string, std::string>& Parser::parseJson(std::map<std::string, std::string>& data) {
    consume(TokenType::LBRACE, "Se esperaba '{'");
    while (peek().type != TokenType::RBRACE) {
        Token key = consume(TokenType::STRING, "Se esperaba llave");
        consume(TokenType::COLON, "Se esperaba ':'");
        if (peek().type == TokenType::LBRACE) {
            std::map<std::string, std::string> nested;
            parseJson(nested);
            data[key.lexeme] = "[Object]";
        } else {
            Token val = advance();
            data[key.lexeme] = val.lexeme;
        }
        if (!match(TokenType::COMMA)) break;
    }
    consume(TokenType::RBRACE, "Se esperaba '}'");
    return data;
}

Token Parser::advance() { 
    if (pos < tokens.size()) return tokens[pos++]; 
    return (tokens.empty() ? Token{TokenType::UNKNOWN, ""} : tokens.back()); 
}
Token Parser::peek() { 
    if (pos < tokens.size()) return tokens[pos]; 
    return (tokens.empty() ? Token{TokenType::UNKNOWN, ""} : tokens.back()); 
}
bool Parser::match(TokenType type) { 
    if (peek().type == type) { advance(); return true; } 
    return false; 
}
Token Parser::consume(TokenType type, const std::string& message) {
    if (peek().type == type) return advance();
    throw std::runtime_error(message + ". Se encontro: " + peek().lexeme);
}