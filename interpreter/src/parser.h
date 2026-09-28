#pragma once
#include "token.h"
#include "ast.h"
#include <vector>
#include <string>

struct ParseError {
    std::string type;
    std::string message;
    int line;
    int col;
};

class Parser {
public:
    Parser(const std::vector<Token>& tokens);

    ASTNodePtr parse();
    const std::vector<ParseError>& getErrors() const { return errors_; }
    bool hasErrors() const { return !errors_.empty(); }

private:
    std::vector<Token> tokens_;
    int pos_;
    std::vector<ParseError> errors_;

    const Token& current() const;
    const Token& peek() const;
    const Token& advance();
    bool isAtEnd() const;
    bool check(TokenType type) const;
    bool checkKeyword(const std::string& kw) const;
    bool match(TokenType type);
    bool matchKeyword(const std::string& kw);
    const Token& expect(TokenType type, const std::string& message);
    void expectKeyword(const std::string& kw, const std::string& message);
    void expectNewline();
    void skipNewlines();

    void addError(const std::string& type, const std::string& message, int line, int col);
    void synchronize();

    // Grammar productions
    ASTNodePtr parseStatement();
    ASTNodePtr parseSetStatement();
    ASTNodePtr parsePrintStatement();
    ASTNodePtr parseIfStatement();
    ASTNodePtr parseWhileStatement();
    std::vector<ASTNodePtr> parseBlock(const std::string& terminator1, const std::string& terminator2 = "");

    // Expression precedence
    ASTNodePtr parseExpression();
    ASTNodePtr parseOr();
    ASTNodePtr parseAnd();
    ASTNodePtr parseNot();
    ASTNodePtr parseComparison();
    ASTNodePtr parseArithmetic();
    ASTNodePtr parseTerm();
    ASTNodePtr parseUnary();
    ASTNodePtr parseFactor();
};
