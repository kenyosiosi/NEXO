#include "../../include/Query_engine/tokenizer.h"
#include <cctype>

Tokenizer::Tokenizer(const std::string& input) : source(input), cursor(0) {}

std::vector<Token> Tokenizer::tokenize() {
    std::vector<Token> tokens;
    while (cursor < source.length()) {
        char c = peek();
        if (std::isspace(c)) { skipWhitespace(); continue; }
        switch (c) {
            case '[': tokens.push_back({TokenType::LBRACKET, "["}); advance(); break;
            case ']': tokens.push_back({TokenType::RBRACKET, "]"}); advance(); break;
            case '{': tokens.push_back({TokenType::LBRACE, "{"}); advance(); break;
            case '}': tokens.push_back({TokenType::RBRACE, "}"}); advance(); break;
            case ':': tokens.push_back({TokenType::COLON, ":"}); advance(); break;
            case ',': tokens.push_back({TokenType::COMMA, ","}); advance(); break;
            case '"': tokens.push_back(readString()); break;
            default:
                if (std::isdigit(c)) tokens.push_back(readNumber());
                else if (std::isalpha(c) || c == '_') tokens.push_back(readIdentifier());
                else { tokens.push_back({TokenType::UNKNOWN, std::string(1, c)}); advance(); }
                break;
        }
    }
    tokens.push_back({TokenType::END_OF_FILE, ""});
    return tokens;
}

char Tokenizer::peek() { return cursor >= source.length() ? '\0' : source[cursor]; }
char Tokenizer::advance() { return cursor >= source.length() ? '\0' : source[cursor++]; }
void Tokenizer::skipWhitespace() { while (cursor < source.length() && std::isspace(peek())) advance(); }

Token Tokenizer::readString() {
    advance();
    std::string value = "";
    while (cursor < source.length() && peek() != '"') value += advance();
    if (cursor < source.length() && peek() == '"') advance();
    else return {TokenType::UNKNOWN, value}; 
    return {TokenType::STRING, value};
}

Token Tokenizer::readNumber() {
    std::string value = "";
    while (cursor < source.length() && std::isdigit(peek())) value += advance();
    return {TokenType::NUMBER, value};
}

Token Tokenizer::readIdentifier() {
    std::string value = "";
    while (cursor < source.length() && (std::isalnum(peek()) || peek() == '_')) value += advance();

    if (value == "INSERT") return {TokenType::INSERT, value};
    if (value == "GET")    return {TokenType::GET, value};
    if (value == "DELETE") return {TokenType::DELETE, value};
    if (value == "CREATE") return {TokenType::CREATE, value};
    if (value == "UPDATE") return {TokenType::UPDATE, value};
    if (value == "INDEX")  return {TokenType::INDEX, value}; 
    if (value == "ON")     return {TokenType::ON, value};    
    if (value == "SET")    return {TokenType::SET, value};    
    if (value == "WHERE")  return {TokenType::WHERE, value};    
    if (value == "INTO")   return {TokenType::INTO, value};    
    if (value == "FROM")   return {TokenType::FROM, value};    
    
    return {TokenType::STRING, value}; 
}