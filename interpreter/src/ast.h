#pragma once
#include <string>
#include <vector>
#include <memory>
// Forward declaration
class ASTNode;

using ASTNodePtr = std::shared_ptr<ASTNode>;

enum class NodeType {
    Program,
    Assign,
    Print,
    If,
    While,
    BinaryExpr,
    UnaryExpr,
    Literal,
    Variable
};

// Value types in MiniLang
enum class ValueType {
    Int,
    Float,
    String,
    Bool,
    None
};

struct Value {
    ValueType type;
    int64_t intVal = 0;
    double floatVal = 0.0;
    std::string strVal;
    bool boolVal = false;

    Value() : type(ValueType::None) {}
    static Value makeInt(int64_t v) { Value val; val.type = ValueType::Int; val.intVal = v; return val; }
    static Value makeFloat(double v) { Value val; val.type = ValueType::Float; val.floatVal = v; return val; }
    static Value makeString(const std::string& v) { Value val; val.type = ValueType::String; val.strVal = v; return val; }
    static Value makeBool(bool v) { Value val; val.type = ValueType::Bool; val.boolVal = v; return val; }

    std::string toString() const;
    std::string typeString() const;
};

class ASTNode {
public:
    NodeType nodeType;
    int line;
    int col;

    virtual ~ASTNode() = default;
    virtual std::string toJSON(int indent = 0) const = 0;

protected:
    ASTNode(NodeType type, int line, int col)
        : nodeType(type), line(line), col(col) {}

    std::string ind(int level) const;
};

// Program node (root)
class ProgramNode : public ASTNode {
public:
    std::vector<ASTNodePtr> statements;

    ProgramNode() : ASTNode(NodeType::Program, 1, 1) {}
    std::string toJSON(int indent = 0) const override;
};

// SET x = expr
class AssignNode : public ASTNode {
public:
    std::string name;
    ASTNodePtr value;

    AssignNode(const std::string& name, ASTNodePtr value, int line, int col)
        : ASTNode(NodeType::Assign, line, col), name(name), value(std::move(value)) {}
    std::string toJSON(int indent = 0) const override;
};

// PRINT expr
class PrintNode : public ASTNode {
public:
    ASTNodePtr expression;

    PrintNode(ASTNodePtr expr, int line, int col)
        : ASTNode(NodeType::Print, line, col), expression(std::move(expr)) {}
    std::string toJSON(int indent = 0) const override;
};

// IF expr ... ELSE ... END
class IfNode : public ASTNode {
public:
    ASTNodePtr condition;
    std::vector<ASTNodePtr> thenBranch;
    std::vector<ASTNodePtr> elseBranch;

    IfNode(ASTNodePtr cond, int line, int col)
        : ASTNode(NodeType::If, line, col), condition(std::move(cond)) {}
    std::string toJSON(int indent = 0) const override;
};

// WHILE expr ... END
class WhileNode : public ASTNode {
public:
    ASTNodePtr condition;
    std::vector<ASTNodePtr> body;

    WhileNode(ASTNodePtr cond, int line, int col)
        : ASTNode(NodeType::While, line, col), condition(std::move(cond)) {}
    std::string toJSON(int indent = 0) const override;
};

// Binary expression: left op right
class BinaryExprNode : public ASTNode {
public:
    std::string op;
    ASTNodePtr left;
    ASTNodePtr right;

    BinaryExprNode(const std::string& op, ASTNodePtr left, ASTNodePtr right, int line, int col)
        : ASTNode(NodeType::BinaryExpr, line, col), op(op),
          left(std::move(left)), right(std::move(right)) {}
    std::string toJSON(int indent = 0) const override;
};

// Unary expression: op operand
class UnaryExprNode : public ASTNode {
public:
    std::string op;
    ASTNodePtr operand;

    UnaryExprNode(const std::string& op, ASTNodePtr operand, int line, int col)
        : ASTNode(NodeType::UnaryExpr, line, col), op(op), operand(std::move(operand)) {}
    std::string toJSON(int indent = 0) const override;
};

// Literal value
class LiteralNode : public ASTNode {
public:
    Value value;

    LiteralNode(const Value& val, int line, int col)
        : ASTNode(NodeType::Literal, line, col), value(val) {}
    std::string toJSON(int indent = 0) const override;
};

// Variable reference
class VariableNode : public ASTNode {
public:
    std::string name;

    VariableNode(const std::string& name, int line, int col)
        : ASTNode(NodeType::Variable, line, col), name(name) {}
    std::string toJSON(int indent = 0) const override;
};

std::string nodeTypeToString(NodeType type);
