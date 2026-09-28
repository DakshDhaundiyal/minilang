#include "semantic.h"
#include <memory>

SemanticAnalyzer::SemanticAnalyzer() {}

void SemanticAnalyzer::analyze(ASTNodePtr node) {
    analyzeNode(node);
}

void SemanticAnalyzer::analyzeNode(ASTNodePtr node) {
    if (!node) return;

    switch (node->nodeType) {
        case NodeType::Program: {
            auto prog = std::dynamic_pointer_cast<ProgramNode>(node);
            for (auto& stmt : prog->statements) {
                analyzeStatement(stmt);
            }
            break;
        }
        default:
            analyzeStatement(node);
            break;
    }
}

void SemanticAnalyzer::analyzeStatement(ASTNodePtr node) {
    if (!node) return;

    switch (node->nodeType) {
        case NodeType::Assign: {
            auto assign = std::dynamic_pointer_cast<AssignNode>(node);
            // Analyze the value expression FIRST (so using x in SET x = x + 1 checks correctly)
            analyzeExpression(assign->value);
            // Then declare the variable
            declaredVars_.insert(assign->name);
            break;
        }
        case NodeType::Print: {
            auto print = std::dynamic_pointer_cast<PrintNode>(node);
            analyzeExpression(print->expression);
            break;
        }
        case NodeType::If: {
            auto ifNode = std::dynamic_pointer_cast<IfNode>(node);
            analyzeExpression(ifNode->condition);
            for (auto& stmt : ifNode->thenBranch) analyzeStatement(stmt);
            for (auto& stmt : ifNode->elseBranch) analyzeStatement(stmt);
            break;
        }
        case NodeType::While: {
            auto whileNode = std::dynamic_pointer_cast<WhileNode>(node);
            analyzeExpression(whileNode->condition);
            for (auto& stmt : whileNode->body) analyzeStatement(stmt);
            break;
        }
        default:
            break;
    }
}

void SemanticAnalyzer::analyzeExpression(ASTNodePtr node) {
    if (!node) return;

    switch (node->nodeType) {
        case NodeType::Variable: {
            auto var = std::dynamic_pointer_cast<VariableNode>(node);
            if (declaredVars_.find(var->name) == declaredVars_.end()) {
                errors_.push_back({
                    "undeclared_variable",
                    "Variable '" + var->name + "' used before declaration",
                    var->line,
                    var->col
                });
            }
            break;
        }
        case NodeType::BinaryExpr: {
            auto bin = std::dynamic_pointer_cast<BinaryExprNode>(node);
            analyzeExpression(bin->left);
            analyzeExpression(bin->right);
            break;
        }
        case NodeType::UnaryExpr: {
            auto un = std::dynamic_pointer_cast<UnaryExprNode>(node);
            analyzeExpression(un->operand);
            break;
        }
        case NodeType::Literal:
            break;
        default:
            break;
    }
}
