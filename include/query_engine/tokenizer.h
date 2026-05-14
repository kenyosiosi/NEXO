#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <string>
#include <vector>

enum class TokenType {
    CREATE, INSERT, GET, DELETE, UPDATE, SCAN, DROP, DEFINE, STATS,
    INDEX, ON, SET, INTO, FROM, WHERE, // <-- Añadidos los faltantes
    ID, STRING, NUMBER,
    LBRACKET, RBRACKET, LBRACE, RBRACE, COLON, COMMA,
    END_OF_FILE, UNKNOWN
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
    size_t cursor;

    char peek();
    char advance();
    void skipWhitespace();
    Token readString();
    Token readNumber();
    Token readIdentifier();
};

#endif















// #ifndef TOKENIZER_H
// #define TOKENIZER_H

// #include <string>
// #include <vector>

// enum class TokenType {
//     // Comandos principales
//     CREATE, INSERT, GET, DELETE, UPDATE,
//     INDEX, ON,
    
//     // Elementos de datos
//     ID,       
//     STRING,      
//     NUMBER,      
    
//     // Símbolos de estructura JSON
//     LBRACKET,     //  [
//     RBRACKET,    //  ]
//     LBRACE,      // {
//     RBRACE,      // }
//     COLON,       // :
//     COMMA,       // ,
    
//     // Control
//     END_OF_FILE, // Indica que se terminó el comando
//     UNKNOWN      // Para errores (ej. un símbolo extraño @ o $)
// };

// struct Token {
//     TokenType type;
//     std::string lexeme;
// };

// class Tokenizer {
// public:
//     Tokenizer(const std::string& input);
//     std::vector<Token> tokenize();

// private:
//     std::string source;
//     size_t cursor; // Para saber en qué letra del comando vamos

//     // Métodos auxiliares
//     char peek();    // Ver la letra actual sin avanzar
//     char advance(); // Ver la letra actual y pasar a la siguiente
//     void skipWhitespace();
//     Token readString();
//     Token readNumber();
//     Token readIdentifier();
// };

// #endif