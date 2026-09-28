#pragma once
#include "ast.h"
#include "json.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <stdexcept>

struct SymbolEntry {
    std::string name;
    std::string type;
    std::string value;
    std::string scope;
};

struct RuntimeError {
    std::string type;
    std::string message;
    int line;
    int col;
};

class InterpreterException : public std::runtime_error {
public:
    RuntimeError error;
    InterpreterException(const RuntimeError& err)
        : std::runtime_error(err.message), error(err) {}
};

class MiniLangInterpreter {
public:
    MiniLangInterpreter();

    void execute(ASTNodePtr node);
    const std::vector<std::string>& getOutput() const { return output_; }
    std::vector<SymbolEntry> getSymbols() const;
    const std::vector<RuntimeError>& getErrors() const { return errors_; }
    bool hasErrors() const { return !errors_.empty(); }

private:
    std::unordered_map<std::string, Value> symbols_;
    std::vector<std::string> output_;
    std::vector<RuntimeError> errors_;
    int loopIterations_;
    static const int MAX_LOOP_ITERATIONS = 100000;

    void executeStatement(ASTNodePtr node);
    Value evaluateExpression(ASTNodePtr node);

    Value evalBinary(const std::string& op, const Value& left, const Value& right, int line, int col);
    Value evalUnary(const std::string& op, const Value& operand, int line, int col);

    [[noreturn]] void throwRuntime(const std::string& type, const std::string& message, int line, int col);
};
