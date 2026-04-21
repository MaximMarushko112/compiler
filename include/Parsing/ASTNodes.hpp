#pragma once

#include <vector>
#include <deque>
#include <memory>
#include <optional>
#include <string>
#include <variant>

#include <Utils/Boxed.hpp>
#include <Utils/TypeTuple.hpp>
#include <Utils/Position.hpp>

namespace Parsing {

// ---------- Типы данных ----------
struct TypeNode;

// Базовые типы
struct VoidType {};
struct IntType {};
struct UnsignedType {};
struct FloatType {};
struct BoolType {};
struct StringType {};

// Именованный тип (struct или enum)
struct NamedType {
    std::string name;
};

// Указатель
struct PointerType {
    Util::Boxed<TypeNode> pointee;
};

// Массив (размер может быть константным выражением)
struct ArrayType {
    Util::Boxed<TypeNode> elementType;
    std::optional<int> size;   // nullopt = [], иначе размер
};

// Функциональный тип (для указателей на функции)
struct FunctionType {
    std::vector<TypeNode> parameterTypes;
    std::vector<TypeNode> returnTypes;  // множественный возврат
    bool variadic;                      // есть ли ...
};

// Кортеж для множественного возврата (используется только в return_type функции)
struct TupleType {
    std::vector<TypeNode> types;
};

using TypeVariants = Util::TTuple<
    VoidType, IntType, UnsignedType, FloatType, BoolType, StringType,
    NamedType, PointerType, ArrayType, FunctionType, TupleType
>;

struct TypeNode : Util::TupleToVariant<TypeVariants>::Result {
    template<typename... Args>
    TypeNode(Args&&... args)
        : Util::TupleToVariant<TypeVariants>::Result(std::forward<Args>(args)...) {}
};

// ---------- Выражения ----------
struct ExprNode;

// Литералы
struct IntLiteral {
    int value;
};

struct FloatLiteral {
    float value;
};

struct StringLiteral {
    std::string value;
};

struct BoolLiteral {
    bool value;
};

// Переменная
struct VariableExpr {
    std::string name;
};

// Унарные операции
struct UnaryOp {
    enum class Op { Not, Minus, Plus, AddressOf, Dereference, BitNot };
    Op op;
    Util::Boxed<ExprNode> operand;
};

// Бинарные операции
struct BinaryOp {
    enum class Op {
        Add, Sub, Mul, Div, Rem,
        ShiftLeft, ShiftRight,
        Less, LessEqual, Greater, GreaterEqual,
        Equal, NotEqual,
        BitAnd, BitXor, BitOr,
        LogicalAnd, LogicalOr
    };
    Op op;
    Util::Boxed<ExprNode> left;
    Util::Boxed<ExprNode> right;
};

// Присваивание (одиночное)
struct AssignExpr {
    Util::Boxed<ExprNode> left;   // lvalue
    Util::Boxed<ExprNode> right;
    enum class Op { Assign, AddAssign, SubAssign, MulAssign, DivAssign, RemAssign,
                    ShiftLeftAssign, ShiftRightAssign, AndAssign, XorAssign, OrAssign };
    Op op;
};

// Множественное присваивание (a, b = c, d)
struct MultiAssignExpr {
    std::vector<Util::Boxed<ExprNode>> lefts;   // список lvalue
    std::vector<Util::Boxed<ExprNode>> rights;  // список выражений
    // Оператор всегда '='
};

// Тернарный оператор ?:
struct ConditionalExpr {
    Util::Boxed<ExprNode> cond;
    Util::Boxed<ExprNode> thenExpr;
    Util::Boxed<ExprNode> elseExpr;
};

// Вызов функции
struct CallExpr {
    Util::Boxed<ExprNode> callee;
    std::vector<Util::Boxed<ExprNode>> arguments;
};

// Доступ к полю (.) или (->)
struct FieldAccessExpr {
    Util::Boxed<ExprNode> object;
    std::string field;
    bool isArrow;   // true если ->, false если .
};

// Индексация массива
struct IndexExpr {
    Util::Boxed<ExprNode> array;
    Util::Boxed<ExprNode> index;
};

// Приведение типа
struct CastExpr {
    TypeNode targetType;
    Util::Boxed<ExprNode> operand;
};

// sizeof выражение или тип
struct SizeofExpr {
    std::variant<TypeNode, Util::Boxed<ExprNode>> operand;
};

// Блок (последовательность операторов) – используется в теле функции и составных операторах
struct BlockStmt;   // вперёд

// Операторы (statements)
struct StmtNode;

// Выражение-оператор
struct ExprStmt {
    Util::Boxed<ExprNode> expr;
};

// Блок
struct BlockStmt {
    std::vector<Util::Boxed<StmtNode>> statements;
};

// If
struct IfStmt {
    Util::Boxed<ExprNode> condition;
    Util::Boxed<StmtNode> thenStmt;
    std::optional<Util::Boxed<StmtNode>> elseStmt;
};

// While
struct WhileStmt {
    Util::Boxed<ExprNode> condition;
    Util::Boxed<StmtNode> body;
};

// Do-while
struct DoWhileStmt {
    Util::Boxed<StmtNode> body;
    Util::Boxed<ExprNode> condition;
};

// For
struct ForStmt {
    std::optional<Util::Boxed<StmtNode>> init;      // может быть объявление переменной или выражение
    std::optional<Util::Boxed<ExprNode>> condition;
    std::optional<Util::Boxed<ExprNode>> increment;
    Util::Boxed<StmtNode> body;
};

// Switch
struct SwitchStmt {
    Util::Boxed<ExprNode> control;
    Util::Boxed<StmtNode> body;   // тело содержит case/default
};

// Case
struct CaseLabel {
    Util::Boxed<ExprNode> value;
};

struct DefaultLabel {};

struct CaseStmt {
    std::variant<CaseLabel, DefaultLabel> label;
    Util::Boxed<StmtNode> statement;
};

// Break, Continue, Return
struct BreakStmt {};
struct ContinueStmt {};
struct ReturnStmt {
    std::vector<Util::Boxed<ExprNode>> values;   // множественный возврат
};

// Goto и метка
struct GotoStmt {
    std::string label;
};
struct LabelStmt {
    std::string name;
    Util::Boxed<StmtNode> statement;
};

// Объявление переменной (локальной)
struct VarDeclStmt {
    TypeNode type;
    std::string name;
    std::optional<Util::Boxed<ExprNode>> initializer;
};

using StmtVariants = Util::TTuple<
    ExprStmt, BlockStmt, IfStmt, WhileStmt, DoWhileStmt, ForStmt,
    SwitchStmt, CaseStmt, BreakStmt, ContinueStmt, ReturnStmt,
    GotoStmt, LabelStmt, VarDeclStmt
>;

struct StmtNode : Util::TupleToVariant<StmtVariants>::Result {
    template<typename... Args>
    StmtNode(Args&&... args)
        : Util::TupleToVariant<StmtVariants>::Result(std::forward<Args>(args)...) {}
};

// Все выражения (ExprNode)
using ExprVariants = Util::TTuple<
    IntLiteral, FloatLiteral, StringLiteral, BoolLiteral,
    VariableExpr, UnaryOp, BinaryOp, AssignExpr, MultiAssignExpr,
    ConditionalExpr, CallExpr, FieldAccessExpr, IndexExpr, CastExpr, SizeofExpr
>;

struct ExprNode : Util::TupleToVariant<ExprVariants>::Result {
    template<typename... Args>
    ExprNode(Args&&... args)
        : Util::TupleToVariant<ExprVariants>::Result(std::forward<Args>(args)...) {}
};

// ---------- Определения верхнего уровня ----------
struct Param {
    std::string name;
    TypeNode type;
};

// Функция
struct FunctionDef {
    std::string name;
    std::vector<Param> parameters;
    std::vector<TypeNode> returnTypes;   // несколько типов
    bool variadic;                       // есть ...
    std::optional<BlockStmt> body;       // тело может отсутствовать (объявление)
};

// Глобальная переменная
struct GlobalVarDef {
    TypeNode type;
    std::string name;
    std::optional<ExprNode> initializer;
};

// Структура
struct StructDef {
    std::string name;
    std::vector<std::pair<std::string, TypeNode>> fields; // имя : тип
};

// Перечисление
struct EnumDef {
    std::string name;
    std::vector<std::pair<std::string, std::optional<int>>> enumerators; // имя = значение?
};

using DefVariants = Util::TTuple<FunctionDef, GlobalVarDef, StructDef, EnumDef>;

struct DefNode : Util::TupleToVariant<DefVariants>::Result {
    template<typename... Args>
    DefNode(Args&&... args)
        : Util::TupleToVariant<DefVariants>::Result(std::forward<Args>(args)...) {}
};

// Корень программы
struct TranslationUnit {
    std::vector<DefNode> definitions;
};

} // namespace Parsing