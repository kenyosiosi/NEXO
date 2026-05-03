#ifndef PARSER_H
#define PARSER_H

#include "tokenizer.h"
#include <stdexcept>
#include <vector>
#include <string>
#include <map>

class Parser {
public:
    Parser(const std::vector<Token>& tokens);
    
    //parse devuelve el mapa con los datos extraídos
    std::map<std::string, std::string> parse(); 

private:
    std::vector<Token> tokens;
    size_t pos;

    Token advance();
    Token peek();
    bool match(TokenType type);
    void consume(TokenType type, const std::string& message);

    //retornan el mapa de datos
    std::map<std::string, std::string> parseInsert();
    std::map<std::string, std::string> parseGet();    
    std::map<std::string, std::string> parseDelete(); 
    std::map<std::string, std::string> parseUpdate(); 
    std::map<std::string, std::string> parseCreate(); 
    
    std::map<std::string, std::string> parseJson();   
};

#endif