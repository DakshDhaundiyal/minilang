#pragma once
#include "ast.h"
#include <vector>
#include <string>
#include <unordered_set>

struct SemanticError {
    std::string type;
    std::string message;
    int line;
    int col;
};

class SemanticAnalyzer {
public:
    SemanticAnalyzer();

    void analyze(ASTNodePtr node);
    const std::vector<SemanticError>& getErrors() const { return errors_; }
    bool hasErrors() const { return !errors_.empty(); }

private:
    std::vector<SemanticError> errors_;
    std::unordered_set<std::string> declaredVars_;

    void analyzeNode(ASTNodePtr node);
    void analyzeStatement(ASTNodePtr node);
    void analyzeExpression(ASTNodePtr node);
};
