#pragma once

#include "Types.hpp"
#include "ASTVisitor.hpp"
#include <vector>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <iostream>
#include <Utils/Position.hpp>

namespace Parsing {

// ---------------------------------------------------------------------
// Expressions
// ---------------------------------------------------------------------
class Expr {
public:
    virtual ~Expr() = 0;
    virtual void accept(ASTVisitor& visitor) const = 0;
};

class IntLiteral : public Expr {
public:
    int value;
    explicit IntLiteral(int v) : value(v) {}
    void accept(ASTVisitor& visitor) const override;
};

class FloatLiteral : public Expr {
public:
    float value;
    explicit FloatLiteral(float v) : value(v) {}
    void accept(ASTVisitor& visitor) const override;
};

class StringLiteral : public Expr {
public:
    std::string value;
    explicit StringLiteral(std::string v) : value(std::move(v)) {}
    void accept(ASTVisitor& visitor) const override;
};

class BoolLiteral : public Expr {
public:
    bool value;
    explicit BoolLiteral(bool v) : value(v) {}
    void accept(ASTVisitor& visitor) const override;
};

class VariableExpr : public Expr {
public:
    std::string name;
    explicit VariableExpr(std::string n) : name(std::move(n)) {}
    void accept(ASTVisitor& visitor) const override;
};

class UnaryOp : public Expr {
public:
    enum class Op { Not, Minus, Plus, AddressOf, Dereference, BitNot };
    Op op;
    std::unique_ptr<Expr> operand;
    UnaryOp(Op o, std::unique_ptr<Expr> e) : op(o), operand(std::move(e)) {}
    void accept(ASTVisitor& visitor) const override;
};

class BinaryOp : public Expr {
public:
    enum class Op {
        Add, Sub, Mul, Div, Rem,
        ShiftLeft, ShiftRight,
        Less, LessEqual, Greater, GreaterEqual,
        Equal, NotEqual,
        BitAnd, BitXor, BitOr,
        LogicalAnd, LogicalOr
    };
    Op op;
    std::unique_ptr<Expr> left;
    std::unique_ptr<Expr> right;
    BinaryOp(Op o, std::unique_ptr<Expr> l, std::unique_ptr<Expr> r)
        : op(o), left(std::move(l)), right(std::move(r)) {}
    void accept(ASTVisitor& visitor) const override;
};

class AssignExpr : public Expr {
public:
    enum class Op {
        Assign, AddAssign, SubAssign, MulAssign, DivAssign, RemAssign,
        ShiftLeftAssign, ShiftRightAssign, AndAssign, XorAssign, OrAssign
    };
    std::unique_ptr<Expr> left;
    std::unique_ptr<Expr> right;
    Op op;
    AssignExpr(std::unique_ptr<Expr> l, std::unique_ptr<Expr> r, Op o = Op::Assign)
        : left(std::move(l)), right(std::move(r)), op(o) {}
    void accept(ASTVisitor& visitor) const override;
};

class InitListExpr : public Expr {
public:
    std::vector<std::unique_ptr<Expr>> values;
    explicit InitListExpr(std::vector<std::unique_ptr<Expr>> vals) : values(std::move(vals)) {}
    void accept(ASTVisitor& visitor) const override;
};

class MultiAssignExpr : public Expr {
public:
    std::vector<std::unique_ptr<Expr>> lefts;
    std::vector<std::unique_ptr<Expr>> rights;
    MultiAssignExpr(std::vector<std::unique_ptr<Expr>> l, std::vector<std::unique_ptr<Expr>> r)
        : lefts(std::move(l)), rights(std::move(r)) {}
    void accept(ASTVisitor& visitor) const override;
};

class ConditionalExpr : public Expr {
public:
    std::unique_ptr<Expr> cond;
    std::unique_ptr<Expr> thenExpr;
    std::unique_ptr<Expr> elseExpr;
    ConditionalExpr(std::unique_ptr<Expr> c, std::unique_ptr<Expr> t, std::unique_ptr<Expr> e)
        : cond(std::move(c)), thenExpr(std::move(t)), elseExpr(std::move(e)) {}
    void accept(ASTVisitor& visitor) const override;
};

class CallExpr : public Expr {
public:
    std::string functionName;  // имя вызываемой функции
    std::vector<std::unique_ptr<Expr>> arguments;
    CallExpr(std::string name, std::vector<std::unique_ptr<Expr>> args)
        : functionName(std::move(name)), arguments(std::move(args)) {}
    void accept(ASTVisitor& visitor) const override;
};

class FieldAccessExpr : public Expr {
public:
    std::unique_ptr<Expr> object;
    std::string field;
    bool isArrow;
    FieldAccessExpr(std::unique_ptr<Expr> obj, std::string f, bool arrow)
        : object(std::move(obj)), field(std::move(f)), isArrow(arrow) {}
    void accept(ASTVisitor& visitor) const override;
};

class IndexExpr : public Expr {
public:
    std::unique_ptr<Expr> array;
    std::unique_ptr<Expr> index;
    IndexExpr(std::unique_ptr<Expr> arr, std::unique_ptr<Expr> idx)
        : array(std::move(arr)), index(std::move(idx)) {}
    void accept(ASTVisitor& visitor) const override;
};

class CastExpr : public Expr {
public:
    std::unique_ptr<Type> targetType;
    std::unique_ptr<Expr> operand;
    CastExpr(std::unique_ptr<Type> t, std::unique_ptr<Expr> e)
        : targetType(std::move(t)), operand(std::move(e)) {}
    void accept(ASTVisitor& visitor) const override;
};

class SizeofExpr : public Expr {
public:
    std::variant<std::unique_ptr<Type>, std::unique_ptr<Expr>> operand;
    SizeofExpr(std::unique_ptr<Type> t) : operand(std::move(t)) {}
    SizeofExpr(std::unique_ptr<Expr> e) : operand(std::move(e)) {}
    void accept(ASTVisitor& visitor) const override;
};

class PreIncrement : public Expr {
public:
    std::unique_ptr<Expr> operand;
    explicit PreIncrement(std::unique_ptr<Expr> e) : operand(std::move(e)) {}
    void accept(ASTVisitor& visitor) const override;
};

class PostIncrement : public Expr {
public:
    std::unique_ptr<Expr> operand;
    explicit PostIncrement(std::unique_ptr<Expr> e) : operand(std::move(e)) {}
    void accept(ASTVisitor& visitor) const override;
};

class PreDecrement : public Expr {
public:
    std::unique_ptr<Expr> operand;
    explicit PreDecrement(std::unique_ptr<Expr> e) : operand(std::move(e)) {}
    void accept(ASTVisitor& visitor) const override;
};

class PostDecrement : public Expr {
public:
    std::unique_ptr<Expr> operand;
    explicit PostDecrement(std::unique_ptr<Expr> e) : operand(std::move(e)) {}
    void accept(ASTVisitor& visitor) const override;
};

// ---------------------------------------------------------------------
// Statements
// ---------------------------------------------------------------------
class Stmt {
public:
    virtual ~Stmt() = 0;
    virtual void accept(ASTVisitor& visitor) const = 0;
};

class ExprStmt : public Stmt {
public:
    std::unique_ptr<Expr> expr;
    explicit ExprStmt(std::unique_ptr<Expr> e) : expr(std::move(e)) {}
    void accept(ASTVisitor& visitor) const override;
};

class BlockStmt : public Stmt {
public:
    std::vector<std::unique_ptr<Stmt>> statements;
    BlockStmt() = default;
    explicit BlockStmt(std::vector<std::unique_ptr<Stmt>> stmts) : statements(std::move(stmts)) {}
    void accept(ASTVisitor& visitor) const override;
};

class IfStmt : public Stmt {
public:
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> thenStmt;
    std::optional<std::unique_ptr<Stmt>> elseStmt;
    IfStmt(std::unique_ptr<Expr> cond, std::unique_ptr<Stmt> thenPart,
           std::optional<std::unique_ptr<Stmt>> elsePart = std::nullopt)
        : condition(std::move(cond)), thenStmt(std::move(thenPart)), elseStmt(std::move(elsePart)) {}
    void accept(ASTVisitor& visitor) const override;
};

class WhileStmt : public Stmt {
public:
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> body;
    WhileStmt(std::unique_ptr<Expr> cond, std::unique_ptr<Stmt> b)
        : condition(std::move(cond)), body(std::move(b)) {}
    void accept(ASTVisitor& visitor) const override;
};

class DoWhileStmt : public Stmt {
public:
    std::unique_ptr<Stmt> body;
    std::unique_ptr<Expr> condition;
    DoWhileStmt(std::unique_ptr<Stmt> b, std::unique_ptr<Expr> cond)
        : body(std::move(b)), condition(std::move(cond)) {}
    void accept(ASTVisitor& visitor) const override;
};

class ForStmt : public Stmt {
public:
    std::optional<std::unique_ptr<Stmt>> init;
    std::optional<std::unique_ptr<Expr>> condition;
    std::optional<std::unique_ptr<Expr>> increment;
    std::unique_ptr<Stmt> body;
    ForStmt(std::optional<std::unique_ptr<Stmt>> i,
            std::optional<std::unique_ptr<Expr>> cond,
            std::optional<std::unique_ptr<Expr>> inc,
            std::unique_ptr<Stmt> b)
        : init(std::move(i)), condition(std::move(cond)), increment(std::move(inc)), body(std::move(b)) {}
    void accept(ASTVisitor& visitor) const override;
};

class SwitchStmt : public Stmt {
public:
    std::unique_ptr<Expr> control;
    std::unique_ptr<Stmt> body;
    SwitchStmt(std::unique_ptr<Expr> c, std::unique_ptr<Stmt> b)
        : control(std::move(c)), body(std::move(b)) {}
    void accept(ASTVisitor& visitor) const override;
};

struct CaseLabel {
    std::unique_ptr<Expr> value;
    explicit CaseLabel(std::unique_ptr<Expr> v) : value(std::move(v)) {}
    CaseLabel() = default;
};

struct DefaultLabel {};

class CaseStmt : public Stmt {
public:
    std::variant<CaseLabel, DefaultLabel> label;
    std::unique_ptr<Stmt> statement;
    CaseStmt(std::variant<CaseLabel, DefaultLabel> lbl, std::unique_ptr<Stmt> stmt)
        : label(std::move(lbl)), statement(std::move(stmt)) {}
    void accept(ASTVisitor& visitor) const override;
};

class BreakStmt : public Stmt {
public:
    void accept(ASTVisitor& visitor) const override;
};

class ContinueStmt : public Stmt {
public:
    void accept(ASTVisitor& visitor) const override;
};

class ReturnStmt : public Stmt {
public:
    std::vector<std::unique_ptr<Expr>> values;
    explicit ReturnStmt(std::vector<std::unique_ptr<Expr>> vals) : values(std::move(vals)) {}
    void accept(ASTVisitor& visitor) const override;
};

class VarDeclStmt : public Stmt {
public:
    std::unique_ptr<Type> type;
    std::string name;
    std::optional<std::unique_ptr<Expr>> initializer;
    VarDeclStmt(std::unique_ptr<Type> t, std::string n,
                std::optional<std::unique_ptr<Expr>> init = std::nullopt)
        : type(std::move(t)), name(std::move(n)), initializer(std::move(init)) {}
    void accept(ASTVisitor& visitor) const override;
};

// ---------------------------------------------------------------------
// Definitions
// ---------------------------------------------------------------------
class Def {
public:
    virtual ~Def() = 0;
    virtual void accept(ASTVisitor& visitor) const = 0;
};

struct Param {
    std::string name;
    std::unique_ptr<Type> type;
    Param() = default;
    Param(std::string n, std::unique_ptr<Type> t) : name(std::move(n)), type(std::move(t)) {}
};

class FunctionDef : public Def {
public:
    std::string name;
    std::vector<Param> parameters;
    std::vector<std::unique_ptr<Type>> returnTypes;
    bool variadic = false;
    std::optional<BlockStmt> body;
    FunctionDef(std::string n, std::vector<Param> p,
                std::vector<std::unique_ptr<Type>> r, bool v,
                std::optional<BlockStmt> b)
        : name(std::move(n)), parameters(std::move(p)), returnTypes(std::move(r)),
          variadic(v), body(std::move(b)) {}
    void accept(ASTVisitor& visitor) const override;
};

class GlobalVarDef : public Def {
public:
    std::unique_ptr<Type> type;
    std::string name;
    std::optional<std::unique_ptr<Expr>> initializer;
    GlobalVarDef(std::unique_ptr<Type> t, std::string n,
                 std::optional<std::unique_ptr<Expr>> init = std::nullopt)
        : type(std::move(t)), name(std::move(n)), initializer(std::move(init)) {}
    void accept(ASTVisitor& visitor) const override;
};

class StructDef : public Def {
public:
    std::string name;
    std::vector<std::pair<std::string, std::unique_ptr<Type>>> fields;
    StructDef(std::string n, std::vector<std::pair<std::string, std::unique_ptr<Type>>> f)
        : name(std::move(n)), fields(std::move(f)) {}
    void accept(ASTVisitor& visitor) const override;
};

class EnumDef : public Def {
public:
    std::string name;
    std::vector<std::pair<std::string, std::optional<int>>> enumerators;
    EnumDef(std::string n, std::vector<std::pair<std::string, std::optional<int>>> e)
        : name(std::move(n)), enumerators(std::move(e)) {}
    void accept(ASTVisitor& visitor) const override;
};

// ---------------------------------------------------------------------
// Translation unit
// ---------------------------------------------------------------------
struct TranslationUnit {
    std::vector<std::unique_ptr<Def>> definitions;
};

// ---------------------------------------------------------------------
// Inline implementations for Expr, Stmt, Def destructors and accept methods
// ---------------------------------------------------------------------
inline Expr::~Expr() = default;
inline Stmt::~Stmt() = default;
inline Def::~Def() = default;

// Expressions
inline void IntLiteral::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
inline void FloatLiteral::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
inline void StringLiteral::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
inline void BoolLiteral::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
inline void VariableExpr::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
inline void UnaryOp::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
inline void BinaryOp::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
inline void AssignExpr::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
inline void InitListExpr::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
inline void MultiAssignExpr::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
inline void ConditionalExpr::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
inline void CallExpr::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
inline void FieldAccessExpr::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
inline void IndexExpr::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
inline void CastExpr::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
inline void SizeofExpr::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
inline void PreIncrement::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
inline void PostIncrement::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
inline void PreDecrement::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
inline void PostDecrement::accept(ASTVisitor& visitor) const { visitor.visit(*this); }

// Statements
inline void ExprStmt::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
inline void BlockStmt::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
inline void IfStmt::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
inline void WhileStmt::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
inline void DoWhileStmt::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
inline void ForStmt::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
inline void SwitchStmt::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
inline void CaseStmt::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
inline void BreakStmt::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
inline void ContinueStmt::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
inline void ReturnStmt::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
inline void VarDeclStmt::accept(ASTVisitor& visitor) const { visitor.visit(*this); }

// Definitions
inline void FunctionDef::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
inline void GlobalVarDef::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
inline void StructDef::accept(ASTVisitor& visitor) const { visitor.visit(*this); }
inline void EnumDef::accept(ASTVisitor& visitor) const { visitor.visit(*this); }

} // namespace Parsing