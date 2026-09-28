#include "interpreter.h"
#include <cmath>
#include <sstream>
#include <algorithm>

MiniLangInterpreter::MiniLangInterpreter() : loopIterations_(0) {}

void MiniLangInterpreter::throwRuntime(const std::string& type, const std::string& message, int line, int col) {
    throw InterpreterException(RuntimeError{type, message, line, col});
}

void MiniLangInterpreter::execute(ASTNodePtr node) {
    try {
        if (node->nodeType == NodeType::Program) {
            auto prog = std::dynamic_pointer_cast<ProgramNode>(node);
            for (auto& stmt : prog->statements) {
                executeStatement(stmt);
            }
        } else {
            executeStatement(node);
        }
    } catch (const InterpreterException& e) {
        errors_.push_back(e.error);
    }
}

void MiniLangInterpreter::executeStatement(ASTNodePtr node) {
    if (!node) return;

    switch (node->nodeType) {
        case NodeType::Assign: {
            auto assign = std::dynamic_pointer_cast<AssignNode>(node);
            Value val = evaluateExpression(assign->value);
            symbols_[assign->name] = val;
            break;
        }
        case NodeType::Print: {
            auto print = std::dynamic_pointer_cast<PrintNode>(node);
            Value val = evaluateExpression(print->expression);
            output_.push_back(val.toString());
            break;
        }
        case NodeType::If: {
            auto ifNode = std::dynamic_pointer_cast<IfNode>(node);
            Value cond = evaluateExpression(ifNode->condition);
            if (cond.type != ValueType::Bool) {
                throwRuntime("type_error", "IF condition must be a boolean, got " + cond.typeString(),
                             ifNode->line, ifNode->col);
            }
            if (cond.boolVal) {
                for (auto& stmt : ifNode->thenBranch) executeStatement(stmt);
            } else {
                for (auto& stmt : ifNode->elseBranch) executeStatement(stmt);
            }
            break;
        }
        case NodeType::While: {
            auto whileNode = std::dynamic_pointer_cast<WhileNode>(node);
            while (true) {
                Value cond = evaluateExpression(whileNode->condition);
                if (cond.type != ValueType::Bool) {
                    throwRuntime("type_error", "WHILE condition must be a boolean, got " + cond.typeString(),
                                 whileNode->line, whileNode->col);
                }
                if (!cond.boolVal) break;

                loopIterations_++;
                if (loopIterations_ > MAX_LOOP_ITERATIONS) {
                    throwRuntime("execution_limit", "Execution limit exceeded: more than 100000 loop iterations",
                                 whileNode->line, whileNode->col);
                }

                for (auto& stmt : whileNode->body) executeStatement(stmt);
            }
            break;
        }
        default:
            break;
    }
}

Value MiniLangInterpreter::evaluateExpression(ASTNodePtr node) {
    if (!node) return Value();

    switch (node->nodeType) {
        case NodeType::Literal: {
            auto lit = std::dynamic_pointer_cast<LiteralNode>(node);
            return lit->value;
        }
        case NodeType::Variable: {
            auto var = std::dynamic_pointer_cast<VariableNode>(node);
            auto it = symbols_.find(var->name);
            if (it == symbols_.end()) {
                throwRuntime("undefined_variable", "Undefined variable '" + var->name + "'",
                             var->line, var->col);
            }
            return it->second;
        }
        case NodeType::BinaryExpr: {
            auto bin = std::dynamic_pointer_cast<BinaryExprNode>(node);
            Value left = evaluateExpression(bin->left);
            // Short-circuit for AND/OR
            if (bin->op == "AND") {
                if (left.type != ValueType::Bool)
                    throwRuntime("type_error", "AND operand must be boolean, got " + left.typeString(), bin->line, bin->col);
                if (!left.boolVal) return Value::makeBool(false);
                Value right = evaluateExpression(bin->right);
                if (right.type != ValueType::Bool)
                    throwRuntime("type_error", "AND operand must be boolean, got " + right.typeString(), bin->line, bin->col);
                return Value::makeBool(right.boolVal);
            }
            if (bin->op == "OR") {
                if (left.type != ValueType::Bool)
                    throwRuntime("type_error", "OR operand must be boolean, got " + left.typeString(), bin->line, bin->col);
                if (left.boolVal) return Value::makeBool(true);
                Value right = evaluateExpression(bin->right);
                if (right.type != ValueType::Bool)
                    throwRuntime("type_error", "OR operand must be boolean, got " + right.typeString(), bin->line, bin->col);
                return Value::makeBool(right.boolVal);
            }
            Value right = evaluateExpression(bin->right);
            return evalBinary(bin->op, left, right, bin->line, bin->col);
        }
        case NodeType::UnaryExpr: {
            auto un = std::dynamic_pointer_cast<UnaryExprNode>(node);
            Value operand = evaluateExpression(un->operand);
            return evalUnary(un->op, operand, un->line, un->col);
        }
        default:
            return Value();
    }
}

Value MiniLangInterpreter::evalBinary(const std::string& op, const Value& left, const Value& right, int line, int col) {
    // String concatenation
    if (op == "+" && left.type == ValueType::String && right.type == ValueType::String) {
        return Value::makeString(left.strVal + right.strVal);
    }

    // Arithmetic: + - * / %
    if (op == "+" || op == "-" || op == "*" || op == "/" || op == "%") {
        // Both must be numeric
        if ((left.type != ValueType::Int && left.type != ValueType::Float) ||
            (right.type != ValueType::Int && right.type != ValueType::Float)) {
            throwRuntime("type_error",
                "Cannot apply '" + op + "' to " + left.typeString() + " and " + right.typeString(),
                line, col);
        }

        // Modulo: ints only
        if (op == "%") {
            if (left.type != ValueType::Int || right.type != ValueType::Int) {
                throwRuntime("type_error", "Modulo operator '%' requires integer operands", line, col);
            }
            if (right.intVal == 0) {
                throwRuntime("division_by_zero", "Modulo by zero", line, col);
            }
            return Value::makeInt(left.intVal % right.intVal);
        }

        // Division by zero check
        if (op == "/") {
            if (left.type == ValueType::Int && right.type == ValueType::Int) {
                if (right.intVal == 0) throwRuntime("division_by_zero", "Division by zero", line, col);
            } else {
                double rv = (right.type == ValueType::Int) ? (double)right.intVal : right.floatVal;
                if (rv == 0.0) throwRuntime("division_by_zero", "Division by zero", line, col);
            }
        }

        // int op int → int (except when result needs float)
        if (left.type == ValueType::Int && right.type == ValueType::Int) {
            int64_t l = left.intVal, r = right.intVal;
            if (op == "+") return Value::makeInt(l + r);
            if (op == "-") return Value::makeInt(l - r);
            if (op == "*") return Value::makeInt(l * r);
            if (op == "/") return Value::makeInt(l / r); // truncating
        }

        // Otherwise promote to float
        double l = (left.type == ValueType::Int) ? (double)left.intVal : left.floatVal;
        double r = (right.type == ValueType::Int) ? (double)right.intVal : right.floatVal;
        if (op == "+") return Value::makeFloat(l + r);
        if (op == "-") return Value::makeFloat(l - r);
        if (op == "*") return Value::makeFloat(l * r);
        if (op == "/") return Value::makeFloat(l / r);
    }

    // Comparison operators
    if (op == "==" || op == "!=") {
        // Allow same-type comparison
        if (left.type != right.type) {
            // Allow int/float comparison
            if ((left.type == ValueType::Int || left.type == ValueType::Float) &&
                (right.type == ValueType::Int || right.type == ValueType::Float)) {
                double l = (left.type == ValueType::Int) ? (double)left.intVal : left.floatVal;
                double r = (right.type == ValueType::Int) ? (double)right.intVal : right.floatVal;
                if (op == "==") return Value::makeBool(l == r);
                return Value::makeBool(l != r);
            }
            throwRuntime("type_error",
                "Cannot compare " + left.typeString() + " and " + right.typeString(),
                line, col);
        }
        // Same type
        bool equal = false;
        switch (left.type) {
            case ValueType::Int: equal = left.intVal == right.intVal; break;
            case ValueType::Float: equal = left.floatVal == right.floatVal; break;
            case ValueType::String: equal = left.strVal == right.strVal; break;
            case ValueType::Bool: equal = left.boolVal == right.boolVal; break;
            default: break;
        }
        if (op == "==") return Value::makeBool(equal);
        return Value::makeBool(!equal);
    }

    if (op == ">" || op == "<" || op == ">=" || op == "<=") {
        if ((left.type != ValueType::Int && left.type != ValueType::Float) ||
            (right.type != ValueType::Int && right.type != ValueType::Float)) {
            throwRuntime("type_error",
                "Cannot apply '" + op + "' to " + left.typeString() + " and " + right.typeString(),
                line, col);
        }
        double l = (left.type == ValueType::Int) ? (double)left.intVal : left.floatVal;
        double r = (right.type == ValueType::Int) ? (double)right.intVal : right.floatVal;
        if (op == ">")  return Value::makeBool(l > r);
        if (op == "<")  return Value::makeBool(l < r);
        if (op == ">=") return Value::makeBool(l >= r);
        if (op == "<=") return Value::makeBool(l <= r);
    }

    throwRuntime("type_error", "Unknown operator '" + op + "'", line, col);
    return Value(); // unreachable
}

Value MiniLangInterpreter::evalUnary(const std::string& op, const Value& operand, int line, int col) {
    if (op == "-") {
        if (operand.type == ValueType::Int) return Value::makeInt(-operand.intVal);
        if (operand.type == ValueType::Float) return Value::makeFloat(-operand.floatVal);
        throwRuntime("type_error", "Cannot negate " + operand.typeString(), line, col);
    }
    if (op == "NOT") {
        if (operand.type != ValueType::Bool) {
            throwRuntime("type_error", "NOT operand must be boolean, got " + operand.typeString(), line, col);
        }
        return Value::makeBool(!operand.boolVal);
    }
    throwRuntime("type_error", "Unknown unary operator '" + op + "'", line, col);
    return Value(); // unreachable
}

std::vector<SymbolEntry> MiniLangInterpreter::getSymbols() const {
    std::vector<SymbolEntry> result;
    for (auto it = symbols_.begin(); it != symbols_.end(); ++it) {
        SymbolEntry entry;
        entry.name = it->first;
        entry.type = it->second.typeString();
        entry.value = it->second.toString();
        entry.scope = "global";
        result.push_back(entry);
    }
    // Sort by name for deterministic output
    std::sort(result.begin(), result.end(), [](const SymbolEntry& a, const SymbolEntry& b) {
        return a.name < b.name;
    });
    return result;
}
