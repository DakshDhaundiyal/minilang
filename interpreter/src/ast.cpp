#include "ast.h"
#include "json.h"
#include <sstream>

std::string Value::toString() const {
    switch (type) {
        case ValueType::Int: return std::to_string(intVal);
        case ValueType::Float: {
            std::ostringstream oss;
            oss << floatVal;
            std::string s = oss.str();
            // Ensure there's a decimal point
            if (s.find('.') == std::string::npos) s += ".0";
            return s;
        }
        case ValueType::String: return strVal;
        case ValueType::Bool: return boolVal ? "true" : "false";
        case ValueType::None: return "none";
    }
    return "unknown";
}

std::string Value::typeString() const {
    switch (type) {
        case ValueType::Int: return "int";
        case ValueType::Float: return "float";
        case ValueType::String: return "string";
        case ValueType::Bool: return "bool";
        case ValueType::None: return "none";
    }
    return "unknown";
}

std::string nodeTypeToString(NodeType type) {
    switch (type) {
        case NodeType::Program: return "Program";
        case NodeType::Assign: return "Assign";
        case NodeType::Print: return "Print";
        case NodeType::If: return "If";
        case NodeType::While: return "While";
        case NodeType::BinaryExpr: return "BinaryExpr";
        case NodeType::UnaryExpr: return "UnaryExpr";
        case NodeType::Literal: return "Literal";
        case NodeType::Variable: return "Variable";
    }
    return "Unknown";
}

std::string ASTNode::ind(int level) const {
    return std::string(level * 2, ' ');
}

std::string ProgramNode::toJSON(int indent) const {
    std::string i = ind(indent);
    std::string i1 = ind(indent + 1);
    std::string result = i + "{\n";
    result += i1 + "\"type\": \"Program\",\n";
    result += i1 + "\"line\": " + std::to_string(line) + ",\n";
    result += i1 + "\"col\": " + std::to_string(col) + ",\n";
    result += i1 + "\"body\": [\n";
    for (size_t j = 0; j < statements.size(); j++) {
        result += statements[j]->toJSON(indent + 2);
        if (j + 1 < statements.size()) result += ",";
        result += "\n";
    }
    result += i1 + "]\n";
    result += i + "}";
    return result;
}

std::string AssignNode::toJSON(int indent) const {
    std::string i = ind(indent);
    std::string i1 = ind(indent + 1);
    std::string result = i + "{\n";
    result += i1 + "\"type\": \"Assign\",\n";
    result += i1 + "\"line\": " + std::to_string(line) + ",\n";
    result += i1 + "\"col\": " + std::to_string(col) + ",\n";
    result += i1 + "\"name\": " + jsonEscape(name) + ",\n";
    result += i1 + "\"value\": \n" + value->toJSON(indent + 2) + "\n";
    result += i + "}";
    return result;
}

std::string PrintNode::toJSON(int indent) const {
    std::string i = ind(indent);
    std::string i1 = ind(indent + 1);
    std::string result = i + "{\n";
    result += i1 + "\"type\": \"Print\",\n";
    result += i1 + "\"line\": " + std::to_string(line) + ",\n";
    result += i1 + "\"col\": " + std::to_string(col) + ",\n";
    result += i1 + "\"expression\": \n" + expression->toJSON(indent + 2) + "\n";
    result += i + "}";
    return result;
}

std::string IfNode::toJSON(int indent) const {
    std::string i = ind(indent);
    std::string i1 = ind(indent + 1);
    std::string result = i + "{\n";
    result += i1 + "\"type\": \"If\",\n";
    result += i1 + "\"line\": " + std::to_string(line) + ",\n";
    result += i1 + "\"col\": " + std::to_string(col) + ",\n";
    result += i1 + "\"condition\": \n" + condition->toJSON(indent + 2) + ",\n";
    result += i1 + "\"thenBranch\": [\n";
    for (size_t j = 0; j < thenBranch.size(); j++) {
        result += thenBranch[j]->toJSON(indent + 2);
        if (j + 1 < thenBranch.size()) result += ",";
        result += "\n";
    }
    result += i1 + "],\n";
    result += i1 + "\"elseBranch\": [\n";
    for (size_t j = 0; j < elseBranch.size(); j++) {
        result += elseBranch[j]->toJSON(indent + 2);
        if (j + 1 < elseBranch.size()) result += ",";
        result += "\n";
    }
    result += i1 + "]\n";
    result += i + "}";
    return result;
}

std::string WhileNode::toJSON(int indent) const {
    std::string i = ind(indent);
    std::string i1 = ind(indent + 1);
    std::string result = i + "{\n";
    result += i1 + "\"type\": \"While\",\n";
    result += i1 + "\"line\": " + std::to_string(line) + ",\n";
    result += i1 + "\"col\": " + std::to_string(col) + ",\n";
    result += i1 + "\"condition\": \n" + condition->toJSON(indent + 2) + ",\n";
    result += i1 + "\"body\": [\n";
    for (size_t j = 0; j < body.size(); j++) {
        result += body[j]->toJSON(indent + 2);
        if (j + 1 < body.size()) result += ",";
        result += "\n";
    }
    result += i1 + "]\n";
    result += i + "}";
    return result;
}

std::string BinaryExprNode::toJSON(int indent) const {
    std::string i = ind(indent);
    std::string i1 = ind(indent + 1);
    std::string result = i + "{\n";
    result += i1 + "\"type\": \"BinaryExpr\",\n";
    result += i1 + "\"line\": " + std::to_string(line) + ",\n";
    result += i1 + "\"col\": " + std::to_string(col) + ",\n";
    result += i1 + "\"op\": " + jsonEscape(op) + ",\n";
    result += i1 + "\"left\": \n" + left->toJSON(indent + 2) + ",\n";
    result += i1 + "\"right\": \n" + right->toJSON(indent + 2) + "\n";
    result += i + "}";
    return result;
}

std::string UnaryExprNode::toJSON(int indent) const {
    std::string i = ind(indent);
    std::string i1 = ind(indent + 1);
    std::string result = i + "{\n";
    result += i1 + "\"type\": \"UnaryExpr\",\n";
    result += i1 + "\"line\": " + std::to_string(line) + ",\n";
    result += i1 + "\"col\": " + std::to_string(col) + ",\n";
    result += i1 + "\"op\": " + jsonEscape(op) + ",\n";
    result += i1 + "\"operand\": \n" + operand->toJSON(indent + 2) + "\n";
    result += i + "}";
    return result;
}

std::string LiteralNode::toJSON(int indent) const {
    std::string i = ind(indent);
    std::string i1 = ind(indent + 1);
    std::string result = i + "{\n";
    result += i1 + "\"type\": \"Literal\",\n";
    result += i1 + "\"line\": " + std::to_string(line) + ",\n";
    result += i1 + "\"col\": " + std::to_string(col) + ",\n";
    result += i1 + "\"valueType\": " + jsonEscape(value.typeString()) + ",\n";
    if (value.type == ValueType::String) {
        result += i1 + "\"value\": " + jsonEscape(value.strVal) + "\n";
    } else if (value.type == ValueType::Int) {
        result += i1 + "\"value\": " + std::to_string(value.intVal) + "\n";
    } else if (value.type == ValueType::Float) {
        std::ostringstream oss;
        oss << value.floatVal;
        result += i1 + "\"value\": " + oss.str() + "\n";
    } else if (value.type == ValueType::Bool) {
        result += i1 + "\"value\": " + (value.boolVal ? "true" : "false") + "\n";
    } else {
        result += i1 + "\"value\": null\n";
    }
    result += i + "}";
    return result;
}

std::string VariableNode::toJSON(int indent) const {
    std::string i = ind(indent);
    std::string i1 = ind(indent + 1);
    std::string result = i + "{\n";
    result += i1 + "\"type\": \"Variable\",\n";
    result += i1 + "\"line\": " + std::to_string(line) + ",\n";
    result += i1 + "\"col\": " + std::to_string(col) + ",\n";
    result += i1 + "\"name\": " + jsonEscape(name) + "\n";
    result += i + "}";
    return result;
}
