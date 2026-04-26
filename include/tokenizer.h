#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <string>
#include <vector>

// 1. Definimos qué tipos de palabras entiende nuestra base de datos
enum class TokenType {
    // Comandos principales
    CREATE, INSERT, GET, DELETE, 
    
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

// 2. Estructura que representa cada pedacito de información extraído
struct Token {
    TokenType type;
    std::string value;
};

// 3. Declaración de la clase que hará el trabajo sucio
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