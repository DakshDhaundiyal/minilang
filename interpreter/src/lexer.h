#pragma once
#include "token.h"
#include <string>
#include <vector>

struct LexError {
    std::string type;
    std::string message;
    int line;
    int col;
};

class Lexer {
public:
    explicit Lexer(const std::string& source);

    std::vector<Token> tokenize();
    const std::vector<LexError>& getErrors() const { return errors_; }
    bool hasErrors() const { return !errors_.empty(); }

private:
    std::string source_;
    int pos_;
    int line_;
    int col_;
    int tokenIndex_;
    std::vector<LexError> errors_;

    char current() const;
    char peek() const;
    void advance();
    bool isAtEnd() const;

    Token makeToken(TokenType type, const std::string& lexeme, int line, int col);
    void addError(const std::string& type, const std::string& message, int line, int col);

    Token scanString();
    Token scanNumber();
    Token scanIdentifierOrKeyword();
    void skipWhitespace();
};
