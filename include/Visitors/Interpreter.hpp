#pragma once

#include "../Parsing/ASTNodes.hpp"
#include <map>
#include <variant>
#include <string>

namespace Parsing {

class Interpreter : public ASTVisitor {
public:
    Interpreter();
    int run(const TranslationUnit& tu);

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
    void visit(const FieldAccessExpr&) override {}
    void visit(const IndexExpr&) override {}
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
    void visit(const SwitchStmt&) override {}
    void visit(const CaseStmt&) override {}
    void visit(const BreakStmt&) override;
    void visit(const ContinueStmt&) override;
    void visit(const ReturnStmt&) override;
    void visit(const VarDeclStmt&) override;

    // Definitions
    void visit(const FunctionDef&) override;
    void visit(const GlobalVarDef&) override;
    void visit(const StructDef&) override {}
    void visit(const EnumDef&) override;

private:
    std::map<std::string, int> variables;
    std::map<std::string, int> enumConstants;     // значения перечислителей
    std::map<std::string, const FunctionDef*> functions;
    int result;
    bool breakFlag;
    bool continueFlag;
    bool returnFlag;
    int mainReturnValue;

    int getVariableValue(const std::string& name);
    void setVariableValue(const std::string& name, int value);
    void processEnum(const EnumDef& e);
    void evaluate(Expr* e);
    void execute(Stmt* s);
    void executeBlock(const std::vector<std::unique_ptr<Stmt>>& stmts);
};

} // namespace Parsing