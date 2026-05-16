#pragma once

namespace Parsing {

// Forward declarations
class VoidType;
class IntType;
class UnsignedType;
class FloatType;
class BoolType;
class StringType;
class NamedType;
class PointerType;
class ArrayType;
class FunctionType;
class TupleType;

class IntLiteral;
class FloatLiteral;
class StringLiteral;
class BoolLiteral;
class VariableExpr;
class UnaryOp;
class BinaryOp;
class AssignExpr;
class InitListExpr;
class MultiAssignExpr;
class ConditionalExpr;
class CallExpr;
class FieldAccessExpr;
class IndexExpr;
class CastExpr;
class SizeofExpr;
class PreIncrement;
class PostIncrement;
class PreDecrement;
class PostDecrement;

class ExprStmt;
class BlockStmt;
class IfStmt;
class WhileStmt;
class DoWhileStmt;
class ForStmt;
class SwitchStmt;
class CaseStmt;
class BreakStmt;
class ContinueStmt;
class ReturnStmt;
class VarDeclStmt;

class FunctionDef;
class GlobalVarDef;
class StructDef;
class EnumDef;

class ASTVisitor {
public:
    virtual ~ASTVisitor() = default;

    // Types
    virtual void visit(const VoidType&) {}
    virtual void visit(const IntType&) {}
    virtual void visit(const UnsignedType&) {}
    virtual void visit(const FloatType&) {}
    virtual void visit(const BoolType&) {}
    virtual void visit(const StringType&) {}
    virtual void visit(const NamedType&) {}
    virtual void visit(const PointerType&) {}
    virtual void visit(const ArrayType&) {}
    virtual void visit(const FunctionType&) {}
    virtual void visit(const TupleType&) {}

    // Expressions
    virtual void visit(const IntLiteral&) {}
    virtual void visit(const FloatLiteral&) {}
    virtual void visit(const StringLiteral&) {}
    virtual void visit(const BoolLiteral&) {}
    virtual void visit(const VariableExpr&) {}
    virtual void visit(const UnaryOp&) {}
    virtual void visit(const BinaryOp&) {}
    virtual void visit(const AssignExpr&) {}
    virtual void visit(const InitListExpr&) {}
    virtual void visit(const MultiAssignExpr&) {}
    virtual void visit(const ConditionalExpr&) {}
    virtual void visit(const CallExpr&) {}
    virtual void visit(const FieldAccessExpr&) {}
    virtual void visit(const IndexExpr&) {}
    virtual void visit(const CastExpr&) {}
    virtual void visit(const SizeofExpr&) {}
    virtual void visit(const PreIncrement&) {}
    virtual void visit(const PostIncrement&) {}
    virtual void visit(const PreDecrement&) {}
    virtual void visit(const PostDecrement&) {}

    // Statements
    virtual void visit(const ExprStmt&) {}
    virtual void visit(const BlockStmt&) {}
    virtual void visit(const IfStmt&) {}
    virtual void visit(const WhileStmt&) {}
    virtual void visit(const DoWhileStmt&) {}
    virtual void visit(const ForStmt&) {}
    virtual void visit(const SwitchStmt&) {}
    virtual void visit(const CaseStmt&) {}
    virtual void visit(const BreakStmt&) {}
    virtual void visit(const ContinueStmt&) {}
    virtual void visit(const ReturnStmt&) {}
    virtual void visit(const VarDeclStmt&) {}

    // Definitions
    virtual void visit(const FunctionDef&) {}
    virtual void visit(const GlobalVarDef&) {}
    virtual void visit(const StructDef&) {}
    virtual void visit(const EnumDef&) {}
};

} // namespace Parsing