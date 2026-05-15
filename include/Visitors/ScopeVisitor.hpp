#pragma once

#include "../Parsing/ASTNodes.hpp"
#include <map>
#include <vector>
#include <string>
#include <memory>

namespace Parsing {

// Информация о символе (переменная, функция, тип)
struct Symbol {
    enum class Kind { Variable, Function, Type };
    Kind kind;
    std::string name;
    std::unique_ptr<Type> type;      // для переменных и функций
    // для функций дополнительно можно хранить параметры
    std::vector<std::unique_ptr<Type>> paramTypes;
    bool variadic = false;
};

// Область видимости
struct Scope {
    Scope* parent = nullptr;
    std::map<std::string, Symbol> symbols;
    std::vector<std::unique_ptr<Scope>> children;
};

class ScopeVisitor : public ASTVisitor {
public:
    ScopeVisitor();
    void build(const TranslationUnit& tu);
    void printErrors() const; // выводит накопленные ошибки

    // Types
    void visit(const VoidType&) override {}
    void visit(const IntType&) override {}
    void visit(const UnsignedType&) override {}
    void visit(const FloatType&) override {}
    void visit(const BoolType&) override {}
    void visit(const StringType&) override {}
    void visit(const NamedType&) override {}
    void visit(const PointerType&) override {}
    void visit(const ArrayType&) override {}
    void visit(const FunctionType&) override {}
    void visit(const TupleType&) override {}

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
    void visit(const GotoStmt&) override;
    void visit(const LabelStmt&) override;
    void visit(const VarDeclStmt&) override;

    // Definitions
    void visit(const FunctionDef&) override;
    void visit(const GlobalVarDef&) override;
    void visit(const StructDef&) override;
    void visit(const EnumDef&) override;
    
private:
    Scope globalScope;
    Scope* currentScope;

    std::vector<std::string> errors;

    void enterScope();
    void exitScope();
    void addSymbol(const std::string& name, Symbol::Kind kind, std::unique_ptr<Type> type = nullptr);
    Symbol* lookupSymbol(const std::string& name, bool currentOnly = false);
    void error(const std::string& msg);
};

} // namespace Parsing