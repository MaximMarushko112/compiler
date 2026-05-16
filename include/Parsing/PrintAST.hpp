#pragma once

#include "../Parsing/ASTNodes.hpp"
#include <ostream>

namespace Parsing {

class PrintVisitor : public ASTVisitor {
public:
    explicit PrintVisitor(std::ostream& os, int indentStep = 2);
    void print(const TranslationUnit& tu);

    // Types
    void visit(const Type&) override;

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
    std::ostream& out;
    int indentStep;
    int currentIndent = 0;

    void printIndent();
    void increaseIndent() { currentIndent += indentStep; }
    void decreaseIndent() { currentIndent -= indentStep; }
    
    void printPointer(const PointerType&);
    void printArray(const ArrayType&);
    void printFunction(const FunctionType&);
    void printTuple(const TupleType&);
};

} // namespace Parsing