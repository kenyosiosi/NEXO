#include <iostream>
#include <vector>
#include "../include/tokenizer.h"
#include "../include/parser.h"

std::string tokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::CREATE: return "CREATE";
        case TokenType::INSERT: return "INSERT";
        case TokenType::GET:    return "GET";
        case TokenType::DELETE: return "DELETE";
        case TokenType::ID:     return "ID_KEYWORD";
        case TokenType::STRING: return "STRING";
        case TokenType::NUMBER: return "NUMBER";
        case TokenType::LBRACE: return "LBRACE ({)";
        case TokenType::RBRACE: return "RBRACE (})";
        case TokenType::COLON:  return "COLON (:)";
        case TokenType::COMMA:  return "COMMA (,)";
        case TokenType::UNKNOWN: return "UNKNOWN";
        case TokenType::END_OF_FILE: return "EOF";
        default: return "OTHER";
    }
}

int main() {
    std::string input = "INSERT { \"id\": 105 }";
    
    try {
        //Tokenizar
        Tokenizer tokenizer(input);
        std::vector<Token> tokens = tokenizer.tokenize();

        //Parsear
        Parser parser(tokens);
        parser.parse(); // Si algo está mal, saltará al 'catch'

    } catch (const std::exception& e) {
        std::cerr << "FALLO: " << e.what() << std::endl;
    }

    return 0;
}