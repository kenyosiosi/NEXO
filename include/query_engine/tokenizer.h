#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <string>
#include <vector>

enum class TokenType {
    // Comandos principales
    CREATE, INSERT, GET, DELETE, UPDATE,
    
    // Elementos de datos
    ID,       
    STRING,      
    NUMBER,      
    
    // Símbolos de estructura JSON
    LBRACE,      // {
    RBRACE,      // }
    COLON,       // :
    COMMA,       // ,
    
    // Control
    END_OF_FILE, // Indica que se terminó el comando
    UNKNOWN      // Para errores (ej. un símbolo extraño @ o $)
};

struct Token {
    TokenType type;
    std::string lexeme;
};

class Tokenizer {
public:
    Tokenizer(const std::string& input);
    std::vector<Token> tokenize();

private:
    std::string source;
    size_t cursor; // Para saber en qué letra del comando vamos

    // Métodos auxiliares
    char peek();    // Ver la letra actual sin avanzar
    char advance(); // Ver la letra actual y pasar a la siguiente
    void skipWhitespace();
    Token readString();
    Token readNumber();
    Token readIdentifier();
};

#endif