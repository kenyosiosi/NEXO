#ifndef PARSER_H
#define PARSER_H

#include "tokenizer.h"
#include <stdexcept>

class Parser {
public:
    Parser(const std::vector<Token>& tokens);
    void parse(); // Aquí es donde validaremos todo

private:
    std::vector<Token> tokens;
    size_t pos;

    Token advance();
    Token peek();
    bool match(TokenType type);
    void consume(TokenType type, const std::string& message);

    // Funciones de validación gramatical
    void parseInsert();
    void parseJson();
};

#endif