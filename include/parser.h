#ifndef PARSER_H
#define PARSER_H

#include "tokenizer.h"
#include <stdexcept>
#include <vector>
#include <string>

class Parser {
public:
    Parser(const std::vector<Token>& tokens);
    void parse(); 

private:
    std::vector<Token> tokens;
    size_t pos;

    Token advance();
    Token peek();
    bool match(TokenType type);
    void consume(TokenType type, const std::string& message);

    // Funciones de validación gramatical (El CRUD completo)
    void parseInsert();
    void parseGet();    
    void parseDelete(); 
    void parseUpdate();
    void parseCreate(); 
    void parseJson();   
};

#endif