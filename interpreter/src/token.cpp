#include "token.h"

std::string Token::typeString() const {
    return tokenTypeToString(type);
}

bool isKeyword(const std::string& s) {
    return KEYWORDS.count(s) > 0;
}

std::string tokenTypeToString(TokenType t) {
    switch (t) {
        case TokenType::INTEGER:    return "INTEGER";
        case TokenType::FLOAT:      return "FLOAT";
        case TokenType::STRING:     return "STRING";
        case TokenType::IDENTIFIER: return "IDENTIFIER";
        case TokenType::KEYWORD:    return "KEYWORD";
        case TokenType::OPERATOR:   return "OPERATOR";
        case TokenType::LPAREN:     return "DELIMITER";
        case TokenType::RPAREN:     return "DELIMITER";
        case TokenType::EQUALS:     return "DELIMITER";
        case TokenType::NEWLINE:    return "NEWLINE";
        case TokenType::COMMENT:    return "COMMENT";
        case TokenType::EOF_TOKEN:  return "EOF";
        case TokenType::ERROR:      return "ERROR";
        default:                    return "UNKNOWN";
    }
}
