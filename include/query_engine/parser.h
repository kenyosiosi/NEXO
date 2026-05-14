#ifndef PARSER_H
#define PARSER_H

#include "tokenizer.h"
#include <vector>
#include <map>
#include <string>

class Parser {
public:
    Parser(const std::vector<Token>& tokens);
    std::map<std::string, std::string> parse();

private:
    std::vector<Token> tokens;
    size_t pos;

    std::map<std::string, std::string> parseInsert();
    std::map<std::string, std::string> parseGet();
    std::map<std::string, std::string> parseCreate();
    std::map<std::string, std::string> parseDelete();
    std::map<std::string, std::string> parseUpdate();
    std::map<std::string, std::string>& parseJson(std::map<std::string, std::string>& data);

    Token consume(TokenType type, const std::string& message);
    Token advance();
    Token peek();
    bool match(TokenType type);
}; 

#endif