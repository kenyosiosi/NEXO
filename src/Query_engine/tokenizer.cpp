#include "../../include/query_engine/tokenizer.h"
#include <cctype>

// Constructor: preparamos el string y el cursor
Tokenizer::Tokenizer(const std::string& input) : source(input), cursor(0) {}

// Método principal: recorre el string y genera la lista de tokens
std::vector<Token> Tokenizer::tokenize() {
    std::vector<Token> tokens;

    while (cursor < source.length()) {
        char c = peek();

        if (std::isspace(c)) {
            skipWhitespace();
            continue;
        }
        // Reconocimiento de símbolos básicos
        switch (c) {
            case '[': tokens.push_back({TokenType::LBRACKET, "["}); advance(); break;
            case ']': tokens.push_back({TokenType::RBRACKET, "]"}); advance(); break;
            case '{': tokens.push_back({TokenType::LBRACE, "{"}); advance(); break;
            case '}': tokens.push_back({TokenType::RBRACE, "}"}); advance(); break;
            case ':': tokens.push_back({TokenType::COLON, ":"}); advance(); break;
            case ',': tokens.push_back({TokenType::COMMA, ","}); advance(); break;
            
            case '"': 
                tokens.push_back(readString()); 
                break;
            
            default:
                if (std::isdigit(c)) {
                    tokens.push_back(readNumber());
                } else if (std::isalpha(c)) {
                    tokens.push_back(readIdentifier());
                } else {
                    // Si es un símbolo raro que no conocemos (ej: @, $)
                    tokens.push_back({TokenType::UNKNOWN, std::string(1, c)});
                    advance();
                }
                break;
        }
    }

    tokens.push_back({TokenType::END_OF_FILE, ""});
    return tokens;
}

// Métodos auxiliares de movimiento
char Tokenizer::peek() {
    if (cursor >= source.length()) return '\0';
    return source[cursor];
}

char Tokenizer::advance() {
    char c = peek();
    cursor++;
    return c;
}

void Tokenizer::skipWhitespace() {
    while (cursor < source.length() && std::isspace(peek())) {
        advance();
    }
}

// Método para leer textos entre comillas
Token Tokenizer::readString() {
    advance();
    
    std::string value = "";
    
    // Mientras no lleguemos al final del archivo y no encontremos la comilla de cierre
    while (cursor < source.length() && peek() != '"') {
        value += advance(); // Guardamos la letra y avanzamos
    }

    // Si salimos del bucle, verificamos si cerramos la comilla
    if (cursor < source.length() && peek() == '"') {
        advance(); // Saltamos la comilla de cierre '"'
    } else {
        // Si el usuario olvidó cerrar las comillas
        return {TokenType::UNKNOWN, value}; 
    }

    // Comprobamos si el texto era la palabra reservada "id"
    if (value == "id") {
        return {TokenType::ID, value};
    }

    return {TokenType::STRING, value};
}

Token Tokenizer::readNumber() {
    std::string value = "";
    while (cursor < source.length() && std::isdigit(peek())) {
        value += advance();
    }
    return {TokenType::NUMBER, value};
}

Token Tokenizer::readIdentifier() {
    std::string value = "";
    while (cursor < source.length() && std::isalnum(peek())) {
        value += advance();
    }

    // Comprobamos si es un comando conocido
    if (value == "INSERT") return {TokenType::INSERT, value};
    if (value == "GET")    return {TokenType::GET, value};
    if (value == "DELETE") return {TokenType::DELETE, value};
    if (value == "CREATE") return {TokenType::CREATE, value};
    if (value == "UPDATE") return {TokenType::UPDATE, value};

    // Si no es comando, lo tratamos como un texto normal
    return {TokenType::STRING, value};
}