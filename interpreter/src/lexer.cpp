#include "lexer.h"
#include <cctype>

Lexer::Lexer(const std::string& source)
    : source_(source), pos_(0), line_(1), col_(1), tokenIndex_(0) {}

char Lexer::current() const {
    if (isAtEnd()) return '\0';
    return source_[pos_];
}

char Lexer::peek() const {
    if (pos_ + 1 >= (int)source_.size()) return '\0';
    return source_[pos_ + 1];
}

void Lexer::advance() {
    if (!isAtEnd()) {
        if (source_[pos_] == '\n') {
            line_++;
            col_ = 1;
        } else {
            col_++;
        }
        pos_++;
    }
}

bool Lexer::isAtEnd() const {
    return pos_ >= (int)source_.size();
}

Token Lexer::makeToken(TokenType type, const std::string& lexeme, int line, int col) {
    Token t;
    t.index = tokenIndex_++;
    t.type = type;
    t.lexeme = lexeme;
    t.line = line;
    t.col = col;
    return t;
}

void Lexer::addError(const std::string& type, const std::string& message, int line, int col) {
    errors_.push_back({type, message, line, col});
}

void Lexer::skipWhitespace() {
    while (!isAtEnd() && (current() == ' ' || current() == '\t' || current() == '\r')) {
        advance();
    }
}

Token Lexer::scanString() {
    int startLine = line_;
    int startCol = col_;
    advance(); // skip opening quote
    std::string value = "\"";

    while (!isAtEnd() && current() != '"' && current() != '\n') {
        value += current();
        advance();
    }

    if (isAtEnd() || current() == '\n') {
        addError("unterminated_string", "Unterminated string literal", startLine, startCol);
        return makeToken(TokenType::ERROR, value, startLine, startCol);
    }

    value += '"';
    advance(); // skip closing quote
    return makeToken(TokenType::STRING, value, startLine, startCol);
}

Token Lexer::scanNumber() {
    int startLine = line_;
    int startCol = col_;
    std::string number;
    bool isFloat = false;
    bool malformed = false;

    // Consume digits
    while (!isAtEnd() && std::isdigit(current())) {
        number += current();
        advance();
    }

    // Check for dot
    if (!isAtEnd() && current() == '.') {
        isFloat = true;
        number += '.';
        advance();

        if (isAtEnd() || !std::isdigit(current())) {
            // "1." with no digits after — malformed
            malformed = true;
            // Consume any trailing digits anyway
            while (!isAtEnd() && std::isdigit(current())) {
                number += current();
                advance();
            }
        } else {
            while (!isAtEnd() && std::isdigit(current())) {
                number += current();
                advance();
            }
            // Check for extra dots: 1.2.3
            if (!isAtEnd() && current() == '.') {
                malformed = true;
                while (!isAtEnd() && (std::isdigit(current()) || current() == '.')) {
                    number += current();
                    advance();
                }
            }
        }
    }

    // Check for trailing alpha: 12abc
    if (!isAtEnd() && (std::isalpha(current()) || current() == '_')) {
        malformed = true;
        while (!isAtEnd() && (std::isalnum(current()) || current() == '_')) {
            number += current();
            advance();
        }
    }

    if (malformed) {
        addError("malformed_number", "Malformed number literal '" + number + "'", startLine, startCol);
        return makeToken(TokenType::ERROR, number, startLine, startCol);
    }

    return makeToken(isFloat ? TokenType::FLOAT : TokenType::INTEGER, number, startLine, startCol);
}

Token Lexer::scanIdentifierOrKeyword() {
    int startLine = line_;
    int startCol = col_;
    std::string ident;

    while (!isAtEnd() && (std::isalnum(current()) || current() == '_')) {
        ident += current();
        advance();
    }

    if (isKeyword(ident)) {
        return makeToken(TokenType::KEYWORD, ident, startLine, startCol);
    }
    return makeToken(TokenType::IDENTIFIER, ident, startLine, startCol);
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;

    while (!isAtEnd()) {
        skipWhitespace();
        if (isAtEnd()) break;

        char c = current();
        int startLine = line_;
        int startCol = col_;

        // Newline
        if (c == '\n') {
            tokens.push_back(makeToken(TokenType::NEWLINE, "\\n", startLine, startCol));
            advance();
            continue;
        }

        // Comment
        if (c == '#') {
            std::string comment;
            while (!isAtEnd() && current() != '\n') {
                comment += current();
                advance();
            }
            tokens.push_back(makeToken(TokenType::COMMENT, comment, startLine, startCol));
            continue;
        }

        // String
        if (c == '"') {
            tokens.push_back(scanString());
            continue;
        }

        // Number
        if (std::isdigit(c)) {
            tokens.push_back(scanNumber());
            continue;
        }

        // Identifier or keyword
        if (std::isalpha(c) || c == '_') {
            tokens.push_back(scanIdentifierOrKeyword());
            continue;
        }

        // Parentheses
        if (c == '(') {
            tokens.push_back(makeToken(TokenType::LPAREN, "(", startLine, startCol));
            advance();
            continue;
        }
        if (c == ')') {
            tokens.push_back(makeToken(TokenType::RPAREN, ")", startLine, startCol));
            advance();
            continue;
        }

        // Two-char operators
        if ((c == '>' || c == '<' || c == '=' || c == '!') && peek() == '=') {
            std::string op;
            op += c;
            advance();
            op += current();
            advance();
            tokens.push_back(makeToken(TokenType::OPERATOR, op, startLine, startCol));
            continue;
        }

        // Single-char operators
        if (c == '+' || c == '-' || c == '*' || c == '/' || c == '%' ||
            c == '>' || c == '<') {
            std::string op(1, c);
            tokens.push_back(makeToken(TokenType::OPERATOR, op, startLine, startCol));
            advance();
            continue;
        }

        // Comparison operators that weren't caught above
        if (c == '!') {
            // standalone ! is not valid in MiniLang (we have NOT keyword)
            addError("invalid_character", "Invalid character '!'", startLine, startCol);
            tokens.push_back(makeToken(TokenType::ERROR, "!", startLine, startCol));
            advance();
            continue;
        }

        // Assignment =
        if (c == '=') {
            tokens.push_back(makeToken(TokenType::EQUALS, "=", startLine, startCol));
            advance();
            continue;
        }

        // Invalid character
        std::string ch(1, c);
        addError("invalid_character", "Invalid character '" + ch + "'", startLine, startCol);
        tokens.push_back(makeToken(TokenType::ERROR, ch, startLine, startCol));
        advance();
    }

    tokens.push_back(makeToken(TokenType::EOF_TOKEN, "", line_, col_));
    return tokens;
}
