#include "parser.h"
#include <stdexcept>

// Sentinel token for out-of-bounds
static Token eofSentinel = {-1, TokenType::EOF_TOKEN, "", 0, 0};

Parser::Parser(const std::vector<Token>& tokens) : pos_(0) {
    // Filter out comments — they are lexer output but not needed for parsing
    for (const auto& t : tokens) {
        if (t.type != TokenType::COMMENT && t.type != TokenType::ERROR) {
            tokens_.push_back(t);
        }
    }
}

const Token& Parser::current() const {
    if (pos_ < (int)tokens_.size()) return tokens_[pos_];
    return eofSentinel;
}

const Token& Parser::peek() const {
    int next = pos_ + 1;
    if (next < (int)tokens_.size()) return tokens_[next];
    return eofSentinel;
}

const Token& Parser::advance() {
    const Token& t = current();
    if (!isAtEnd()) pos_++;
    return t;
}

bool Parser::isAtEnd() const {
    return current().type == TokenType::EOF_TOKEN;
}

bool Parser::check(TokenType type) const {
    return current().type == type;
}

bool Parser::checkKeyword(const std::string& kw) const {
    return current().type == TokenType::KEYWORD && current().lexeme == kw;
}

bool Parser::match(TokenType type) {
    if (check(type)) { advance(); return true; }
    return false;
}

bool Parser::matchKeyword(const std::string& kw) {
    if (checkKeyword(kw)) { advance(); return true; }
    return false;
}

const Token& Parser::expect(TokenType type, const std::string& message) {
    if (check(type)) return advance();
    addError("unexpected_token", message + " (got '" + current().lexeme + "')", current().line, current().col);
    return current();
}

void Parser::expectKeyword(const std::string& kw, const std::string& message) {
    if (checkKeyword(kw)) { advance(); return; }
    addError("unexpected_token", message + " (got '" + current().lexeme + "')", current().line, current().col);
}

void Parser::expectNewline() {
    if (check(TokenType::NEWLINE) || isAtEnd()) {
        if (check(TokenType::NEWLINE)) advance();
        return;
    }
    addError("expected_newline", "Expected newline after statement (got '" + current().lexeme + "')", current().line, current().col);
}

void Parser::skipNewlines() {
    while (check(TokenType::NEWLINE)) advance();
}

void Parser::addError(const std::string& type, const std::string& message, int line, int col) {
    errors_.push_back({type, message, line, col});
}

void Parser::synchronize() {
    // Panic-mode recovery: skip to the next statement boundary
    while (!isAtEnd()) {
        if (check(TokenType::NEWLINE)) {
            advance();
            return;
        }
        advance();
    }
}

ASTNodePtr Parser::parse() {
    auto program = std::make_shared<ProgramNode>();
    skipNewlines();

    while (!isAtEnd()) {
        auto stmt = parseStatement();
        if (stmt) {
            program->statements.push_back(stmt);
        }
        skipNewlines();
    }

    return program;
}

ASTNodePtr Parser::parseStatement() {
    if (checkKeyword("SET")) return parseSetStatement();
    if (checkKeyword("PRINT")) return parsePrintStatement();
    if (checkKeyword("IF")) return parseIfStatement();
    if (checkKeyword("WHILE")) return parseWhileStatement();

    addError("unexpected_token", "Expected statement (SET, PRINT, IF, WHILE), got '" + current().lexeme + "'",
             current().line, current().col);
    synchronize();
    return nullptr;
}

ASTNodePtr Parser::parseSetStatement() {
    int line = current().line;
    int col = current().col;
    advance(); // consume SET

    if (!check(TokenType::IDENTIFIER)) {
        addError("expected_identifier", "Expected variable name after SET", current().line, current().col);
        synchronize();
        return nullptr;
    }
    std::string name = current().lexeme;
    advance();

    if (!match(TokenType::EQUALS)) {
        addError("expected_equals", "Expected '=' after variable name in SET", current().line, current().col);
        synchronize();
        return nullptr;
    }

    auto value = parseExpression();
    if (!value) {
        synchronize();
        return nullptr;
    }

    expectNewline();
    return std::make_shared<AssignNode>(name, value, line, col);
}

ASTNodePtr Parser::parsePrintStatement() {
    int line = current().line;
    int col = current().col;
    advance(); // consume PRINT

    auto expr = parseExpression();
    if (!expr) {
        synchronize();
        return nullptr;
    }

    expectNewline();
    return std::make_shared<PrintNode>(expr, line, col);
}

ASTNodePtr Parser::parseIfStatement() {
    int ifLine = current().line;
    int ifCol = current().col;
    advance(); // consume IF

    auto condition = parseExpression();
    if (!condition) {
        synchronize();
        return nullptr;
    }

    expectNewline();
    skipNewlines();

    auto ifNode = std::make_shared<IfNode>(condition, ifLine, ifCol);

    // Parse then-branch
    ifNode->thenBranch = parseBlock("END", "ELSE");

    // Check for ELSE
    if (checkKeyword("ELSE")) {
        advance();
        expectNewline();
        skipNewlines();
        ifNode->elseBranch = parseBlock("END", "");
    }

    // Expect END
    if (checkKeyword("END")) {
        advance();
        expectNewline();
    } else {
        addError("missing_end", "Missing END for IF statement starting at line " + std::to_string(ifLine),
                 ifLine, ifCol);
    }

    return ifNode;
}

ASTNodePtr Parser::parseWhileStatement() {
    int whileLine = current().line;
    int whileCol = current().col;
    advance(); // consume WHILE

    auto condition = parseExpression();
    if (!condition) {
        synchronize();
        return nullptr;
    }

    expectNewline();
    skipNewlines();

    auto whileNode = std::make_shared<WhileNode>(condition, whileLine, whileCol);
    whileNode->body = parseBlock("END", "");

    if (checkKeyword("END")) {
        advance();
        expectNewline();
    } else {
        addError("missing_end", "Missing END for WHILE statement starting at line " + std::to_string(whileLine),
                 whileLine, whileCol);
    }

    return whileNode;
}

std::vector<ASTNodePtr> Parser::parseBlock(const std::string& terminator1, const std::string& terminator2) {
    std::vector<ASTNodePtr> stmts;

    while (!isAtEnd()) {
        if (checkKeyword(terminator1)) break;
        if (!terminator2.empty() && checkKeyword(terminator2)) break;

        auto stmt = parseStatement();
        if (stmt) stmts.push_back(stmt);
        skipNewlines();
    }

    return stmts;
}

// Expression parsing — precedence climbing

ASTNodePtr Parser::parseExpression() {
    return parseOr();
}

ASTNodePtr Parser::parseOr() {
    auto left = parseAnd();
    while (checkKeyword("OR")) {
        int line = current().line, col = current().col;
        advance();
        auto right = parseAnd();
        left = std::make_shared<BinaryExprNode>("OR", left, right, line, col);
    }
    return left;
}

ASTNodePtr Parser::parseAnd() {
    auto left = parseNot();
    while (checkKeyword("AND")) {
        int line = current().line, col = current().col;
        advance();
        auto right = parseNot();
        left = std::make_shared<BinaryExprNode>("AND", left, right, line, col);
    }
    return left;
}

ASTNodePtr Parser::parseNot() {
    if (checkKeyword("NOT")) {
        int line = current().line, col = current().col;
        advance();
        auto operand = parseNot();
        return std::make_shared<UnaryExprNode>("NOT", operand, line, col);
    }
    return parseComparison();
}

ASTNodePtr Parser::parseComparison() {
    auto left = parseArithmetic();

    while (check(TokenType::OPERATOR) &&
           (current().lexeme == ">" || current().lexeme == "<" ||
            current().lexeme == ">=" || current().lexeme == "<=" ||
            current().lexeme == "==" || current().lexeme == "!=")) {
        int line = current().line, col = current().col;
        std::string op = current().lexeme;
        advance();
        auto right = parseArithmetic();
        left = std::make_shared<BinaryExprNode>(op, left, right, line, col);
    }
    return left;
}

ASTNodePtr Parser::parseArithmetic() {
    auto left = parseTerm();

    while (check(TokenType::OPERATOR) &&
           (current().lexeme == "+" || current().lexeme == "-")) {
        int line = current().line, col = current().col;
        std::string op = current().lexeme;
        advance();
        auto right = parseTerm();
        left = std::make_shared<BinaryExprNode>(op, left, right, line, col);
    }
    return left;
}

ASTNodePtr Parser::parseTerm() {
    auto left = parseUnary();

    while (check(TokenType::OPERATOR) &&
           (current().lexeme == "*" || current().lexeme == "/" || current().lexeme == "%")) {
        int line = current().line, col = current().col;
        std::string op = current().lexeme;
        advance();
        auto right = parseUnary();
        left = std::make_shared<BinaryExprNode>(op, left, right, line, col);
    }
    return left;
}

ASTNodePtr Parser::parseUnary() {
    if (check(TokenType::OPERATOR) && current().lexeme == "-") {
        int line = current().line, col = current().col;
        advance();
        auto operand = parseUnary();
        return std::make_shared<UnaryExprNode>("-", operand, line, col);
    }
    return parseFactor();
}

ASTNodePtr Parser::parseFactor() {
    const Token& t = current();

    // Integer literal
    if (t.type == TokenType::INTEGER) {
        advance();
        return std::make_shared<LiteralNode>(Value::makeInt(std::stoll(t.lexeme)), t.line, t.col);
    }

    // Float literal
    if (t.type == TokenType::FLOAT) {
        advance();
        return std::make_shared<LiteralNode>(Value::makeFloat(std::stod(t.lexeme)), t.line, t.col);
    }

    // String literal
    if (t.type == TokenType::STRING) {
        advance();
        // Remove surrounding quotes
        std::string content = t.lexeme.substr(1, t.lexeme.size() - 2);
        return std::make_shared<LiteralNode>(Value::makeString(content), t.line, t.col);
    }

    // Boolean TRUE
    if (t.type == TokenType::KEYWORD && t.lexeme == "TRUE") {
        advance();
        return std::make_shared<LiteralNode>(Value::makeBool(true), t.line, t.col);
    }

    // Boolean FALSE
    if (t.type == TokenType::KEYWORD && t.lexeme == "FALSE") {
        advance();
        return std::make_shared<LiteralNode>(Value::makeBool(false), t.line, t.col);
    }

    // Identifier (variable reference)
    if (t.type == TokenType::IDENTIFIER) {
        advance();
        return std::make_shared<VariableNode>(t.lexeme, t.line, t.col);
    }

    // Parenthesized expression
    if (t.type == TokenType::LPAREN) {
        advance();
        auto expr = parseExpression();
        expect(TokenType::RPAREN, "Expected ')' after expression");
        return expr;
    }

    addError("unexpected_token", "Expected expression, got '" + t.lexeme + "'", t.line, t.col);
    advance(); // skip the bad token
    return nullptr;
}
