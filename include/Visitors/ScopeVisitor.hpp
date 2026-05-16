#pragma once

#include "../Parsing/ASTNodes.hpp"
#include <map>
#include <vector>
#include <string>
#include <memory>
#include <variant>

namespace Parsing {

struct VariableInfo {
    std::unique_ptr<Type> type;
};

struct FunctionInfo {
    std::vector<std::unique_ptr<Type>> paramTypes;
    std::vector<std::unique_ptr<Type>> returnTypes;
    bool variadic = false;
};

struct TypeInfo {
    std::unique_ptr<Type> type;  // для NamedType
};

using Symbol = std::variant<VariableInfo, FunctionInfo, TypeInfo>;

struct Scope {
    Scope* parent = nullptr;
    std::map<std::string, Symbol> symbols;
    std::vector<std::unique_ptr<Scope>> children;

    void addSymbol(const std::string& name, Symbol sym);
    Symbol* lookupSymbol(const std::string& name, bool currentOnly = false);
};

class ScopeVisitor : public ASTVisitor {
public:
    ScopeVisitor();
    void build(const TranslationUnit& tu);
    void printErrors() const;

    // Expressions
    void visit(const IntLiteral&) override;
    void visit(const FloatLiteral&) override;
    void visit(const StringLiteral&) override;
    void visit(const BoolLiteral&) override;
    void visit(const VariableExpr&) override;
    void visit(const UnaryOp&) override;
    void visit(const BinaryOp&) override;
    void visit(const AssignExpr&) override;
    void visit(const InitListExpr&) override;
    void visit(const MultiAssignExpr&) override;
    void visit(const ConditionalExpr&) override;
    void visit(const CallExpr&) override;
    void visit(const FieldAccessExpr&) override;
    void visit(const IndexExpr&) override;
    void visit(const CastExpr&) override;
    void visit(const SizeofExpr&) override;
    void visit(const PreIncrement&) override;
    void visit(const PostIncrement&) override;
    void visit(const PreDecrement&) override;
    void visit(const PostDecrement&) override;

    // Statements
    void visit(const ExprStmt&) override;
    void visit(const BlockStmt&) override;
    void visit(const IfStmt&) override;
    void visit(const WhileStmt&) override;
    void visit(const DoWhileStmt&) override;
    void visit(const ForStmt&) override;
    void visit(const SwitchStmt&) override;
    void visit(const CaseStmt&) override;
    void visit(const BreakStmt&) override;
    void visit(const ContinueStmt&) override;
    void visit(const ReturnStmt&) override;
    void visit(const VarDeclStmt&) override;

    // Definitions
    void visit(const FunctionDef&) override;
    void visit(const GlobalVarDef&) override;
    void visit(const StructDef&) override;
    void visit(const EnumDef&) override;

private:
    Scope globalScope;
    Scope* currentScope;
    const std::vector<std::unique_ptr<Type>>* currentFuncReturnTypes = nullptr; // для проверки return

    std::vector<std::string> errors;

    void enterScope();
    void exitScope();
    void addSymbol(const std::string& name, Symbol sym);
    Symbol* lookupSymbol(const std::string& name, bool currentOnly = false);
    void error(const std::string& msg);
    std::unique_ptr<Type> getExprType(const Expr* expr);
};

} // namespace Parsing