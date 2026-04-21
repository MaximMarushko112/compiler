#pragma once

#include <string>
#include <variant>

namespace Tokenization {

// Макрос для пустых токенов (операторы, ключевые слова, разделители)
#define TOKEN_STRUCT(name) struct name { \
    friend bool operator==(const name&, const name&) = default; \
};

// ---- Операторы (в порядке увеличения сложности) ----
// Арифметические
TOKEN_STRUCT(Plus)          // +
TOKEN_STRUCT(Minus)         // -
TOKEN_STRUCT(Star)          // *
TOKEN_STRUCT(Slash)         // /
TOKEN_STRUCT(Percent)       // %
TOKEN_STRUCT(PlusPlus)      // ++
TOKEN_STRUCT(MinusMinus)    // --

// Побитовые
TOKEN_STRUCT(And)           // &
TOKEN_STRUCT(Or)            // |
TOKEN_STRUCT(Xor)           // ^
TOKEN_STRUCT(Tilde)         // ~
TOKEN_STRUCT(ShiftLeft)     // <<
TOKEN_STRUCT(ShiftRight)    // >>

// Логические
TOKEN_STRUCT(AndAnd)        // &&
TOKEN_STRUCT(OrOr)          // ||
TOKEN_STRUCT(Not)           // !
TOKEN_STRUCT(NotEqual)      // !=
TOKEN_STRUCT(Equal)         // ==
TOKEN_STRUCT(Less)          // <
TOKEN_STRUCT(LessEqual)     // <=
TOKEN_STRUCT(Greater)       // >
TOKEN_STRUCT(GreaterEqual)  // >=

// Присваивания
TOKEN_STRUCT(Assign)        // =
TOKEN_STRUCT(PlusAssign)    // +=
TOKEN_STRUCT(MinusAssign)   // -=
TOKEN_STRUCT(StarAssign)    // *=
TOKEN_STRUCT(SlashAssign)   // /=
TOKEN_STRUCT(PercentAssign) // %=
TOKEN_STRUCT(ShiftLeftAssign)  // <<=
TOKEN_STRUCT(ShiftRightAssign) // >>=
TOKEN_STRUCT(AndAssign)     // &=
TOKEN_STRUCT(XorAssign)     // ^=
TOKEN_STRUCT(OrAssign)      // |=

// Прочие
TOKEN_STRUCT(Arrow)         // ->
TOKEN_STRUCT(Ellipsis)      // ...
TOKEN_STRUCT(Question)      // ?
TOKEN_STRUCT(Colon)         // :

// ---- Разделители ----
TOKEN_STRUCT(LeftParenthesis)  // (
TOKEN_STRUCT(RightParenthesis) // )
TOKEN_STRUCT(LeftBrace)        // {
TOKEN_STRUCT(RightBrace)       // }
TOKEN_STRUCT(LeftBracket)      // [
TOKEN_STRUCT(RightBracket)     // ]
TOKEN_STRUCT(Comma)            // ,
TOKEN_STRUCT(Dot)              // .
TOKEN_STRUCT(Semicolon)        // ;

// ---- Ключевые слова ----
TOKEN_STRUCT(Auto)        // auto (не используется, но можно)
TOKEN_STRUCT(Break)       // break
TOKEN_STRUCT(Case)        // case
TOKEN_STRUCT(Const)       // const
TOKEN_STRUCT(Continue)    // continue
TOKEN_STRUCT(Default)     // default
TOKEN_STRUCT(Do)          // do
TOKEN_STRUCT(Else)        // else
TOKEN_STRUCT(Enum)        // enum
TOKEN_STRUCT(Float)       // float
TOKEN_STRUCT(For)         // for
TOKEN_STRUCT(Goto)        // goto
TOKEN_STRUCT(If)          // if
TOKEN_STRUCT(Int)         // int
TOKEN_STRUCT(Return)      // return
TOKEN_STRUCT(Sizeof)      // sizeof
TOKEN_STRUCT(Struct)      // struct
TOKEN_STRUCT(Switch)      // switch
TOKEN_STRUCT(Unsigned)    // unsigned
TOKEN_STRUCT(Void)        // void
TOKEN_STRUCT(While)       // while
TOKEN_STRUCT(Bool)        // bool
TOKEN_STRUCT(String)      // string
TOKEN_STRUCT(True)        // true
TOKEN_STRUCT(False)       // false

// ---- Литералы ----
struct IntLiteral {
    int value;
    friend bool operator==(const IntLiteral&, const IntLiteral&) = default;
};

struct FloatLiteral {
    float value;
    friend bool operator==(const FloatLiteral&, const FloatLiteral&) = default;
};

struct StringLiteral {
    std::string value;
    friend bool operator==(const StringLiteral&, const StringLiteral&) = default;
};

// ---- Идентификатор ----
struct Identifier {
    std::string name;
    friend bool operator==(const Identifier&, const Identifier&) = default;
};

#undef TOKEN_STRUCT

// ---- Объединение всех токенов ----
using TokenVariant = std::variant<
    // Арифметика
    Plus, Minus, Star, Slash, Percent, PlusPlus, MinusMinus,
    // Побитовые
    And, Or, Xor, Tilde, ShiftLeft, ShiftRight,
    // Логические и сравнение
    AndAnd, OrOr, Not, NotEqual, Equal, Less, LessEqual, Greater, GreaterEqual,
    // Присваивание
    Assign, PlusAssign, MinusAssign, StarAssign, SlashAssign, PercentAssign,
    ShiftLeftAssign, ShiftRightAssign, AndAssign, XorAssign, OrAssign,
    // Прочие операторы
    Arrow, Ellipsis, Question, Colon,
    // Разделители
    LeftParenthesis, RightParenthesis, LeftBrace, RightBrace,
    LeftBracket, RightBracket, Comma, Dot, Semicolon,
    // Ключевые слова
    Auto, Break, Case, Const, Continue, Default, Do, Else, Enum,
    Float, For, Goto, If, Int, Return, Sizeof, Struct, Switch,
    Unsigned, Void, While, Bool, String, True, False,
    // Литералы
    IntLiteral, FloatLiteral, StringLiteral,
    // Идентификатор
    Identifier
>;

} // namespace Tokenization