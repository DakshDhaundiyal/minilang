#pragma once
#include <string>
#include <vector>
#include <unordered_set>

enum class TokenType {
    // Literals
    INTEGER,
    FLOAT,
    STRING,
    // Identifiers & Keywords
    IDENTIFIER,
    KEYWORD,
    // Operators
    OPERATOR,
    // Delimiters
    LPAREN,
    RPAREN,
    EQUALS,     // '=' (assignment)
    NEWLINE,
    // Other
    COMMENT,
    EOF_TOKEN,
    // Error
    ERROR
};

struct Token {
    int index;
    TokenType type;
    std::string lexeme;
    int line;
    int col;

    std::string typeString() const;
};

// Keywords set
const std::unordered_set<std::string> KEYWORDS = {
    "SET", "PRINT", "IF", "ELSE", "END", "WHILE",
    "AND", "OR", "NOT", "TRUE", "FALSE"
};

bool isKeyword(const std::string& s);
std::string tokenTypeToString(TokenType t);
