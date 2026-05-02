#ifndef PARSER_H
#define PARSER_H

#include "tokenizer.h"
#include <stdexcept>
#include <vector>
#include <string>

class Parser {
public:
    Parser(const std::vector<Token>& tokens);
    int parse(); // <--- Cambiado a int

private:
    std::vector<Token> tokens;
    size_t pos;

    Token advance();
    Token peek();
    bool match(TokenType type);
    void consume(TokenType type, const std::string& message);

    // Funciones que devuelven el ID detectado
    int parseInsert(); 
    int parseGet();    
    int parseDelete(); 
    int parseUpdate();
    void parseCreate(); 
    int parseJson();   
};

#endif