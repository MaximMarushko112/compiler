#include "../../include/Parsing/ASTNodes.hpp"

namespace Parsing {

// ---------------------------------------------------------------------
// Destructors for base classes (required for vtable)
// ---------------------------------------------------------------------
Type::~Type() = default;
Expr::~Expr() = default;
Stmt::~Stmt() = default;
Def::~Def() = default;

// ---------------------------------------------------------------------
// Type accept implementations
// ---------------------------------------------------------------------
void VoidType::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void IntType::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void UnsignedType::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void FloatType::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void BoolType::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void StringType::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void NamedType::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void PointerType::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void ArrayType::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void FunctionType::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void TupleType::accept(ASTVisitor& visitor) const { visitor.visit(*this); }

// ---------------------------------------------------------------------
// Expression accept implementations
// ---------------------------------------------------------------------
void IntLiteral::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void FloatLiteral::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void StringLiteral::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void BoolLiteral::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void VariableExpr::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void UnaryOp::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void BinaryOp::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void AssignExpr::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void InitListExpr::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void MultiAssignExpr::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void ConditionalExpr::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void CallExpr::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void FieldAccessExpr::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void IndexExpr::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void CastExpr::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void SizeofExpr::accept(ASTVisitor& visitor) const { visitor.visit(*this); }

// ---------------------------------------------------------------------
// Statement accept implementations
// ---------------------------------------------------------------------
void ExprStmt::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void BlockStmt::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void IfStmt::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void WhileStmt::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void DoWhileStmt::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void ForStmt::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void SwitchStmt::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void CaseStmt::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void BreakStmt::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void ContinueStmt::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void ReturnStmt::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void GotoStmt::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void LabelStmt::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void VarDeclStmt::accept(ASTVisitor& visitor) const { visitor.visit(*this); }

// ---------------------------------------------------------------------
// Definition accept implementations
// ---------------------------------------------------------------------
void FunctionDef::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void GlobalVarDef::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void StructDef::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
void EnumDef::accept(ASTVisitor& visitor) const { visitor.visit(*this); }

} // namespace Parsing