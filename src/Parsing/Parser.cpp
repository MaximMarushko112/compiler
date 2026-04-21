#include "../../include/Parsing/Parser.hpp"
#include <cassert>
#include <optional>
#include <variant>
#include <../include/Utils/Overloaded.hpp>

namespace Parsing {

namespace detail {

// ---------- Утилиты ----------
[[noreturn]] void throwSyntaxError(const Position& pos, const std::string& message) {
    throw std::runtime_error("Syntax error at " + pos.toString() + ": " + message);
}

[[noreturn]] void throwUnexpectedEndOfTokenRange(const std::string& message) {
    std::string err = "Unexpected end of tokens range";
    if (!message.empty()) err += ", " + message;
    throw std::runtime_error(err);
}

template<typename Iter>
const Position& currentPosition(Iter it, Iter end) {
    if (it == end) throw std::runtime_error("Unexpected end of tokens");
    return it->position;
}

template<typename Iter>
void expectToken(Iter& it, Iter end, auto tokenType, const std::string& expected) {
    if (it == end) throwSyntaxError(it[-1].position, "Unexpected end, expected " + expected);
    if (!std::holds_alternative<decltype(tokenType)>(it->token))
        throwSyntaxError(it->position, "Expected " + expected);
    ++it;
}

// Вспомогательный шаблон для проверки типа токена
template<typename T>
bool isToken(const Tokenization::TokenVariant& tv) {
    return std::holds_alternative<T>(tv);
}

// Получить значение литерала
template<typename T>
auto getLiteralValue(const Tokenization::TokenVariant& tv) {
    return std::get<T>(tv).value;
}

// ---------- Парсер типов ----------
class TypeParser {
public:
    template<typename Iter>
    static TypeNode parse(Iter& it, Iter end) {
        return parsePrimary(it, end);
    }

private:
    template<typename Iter>
    static TypeNode parsePrimary(Iter& it, Iter end) {
        if (it == end) throwSyntaxError(Position(), "Expected type");

        return std::visit(overloaded{
            [&](const Tokenization::Int&) -> TypeNode {
                ++it; return IntType{};
            },
            [&](const Tokenization::Unsigned&) -> TypeNode {
                ++it; return UnsignedType{};
            },
            [&](const Tokenization::Float&) -> TypeNode {
                ++it; return FloatType{};
            },
            [&](const Tokenization::Bool&) -> TypeNode {
                ++it; return BoolType{};
            },
            [&](const Tokenization::String&) -> TypeNode {
                ++it; return StringType{};
            },
            [&](const Tokenization::Void&) -> TypeNode {
                ++it; return VoidType{};
            },
            [&](const Tokenization::Struct&) -> TypeNode {
                ++it;
                if (it == end || !isToken<Tokenization::Identifier>(it->token))
                    throwSyntaxError(currentPosition(it, end), "Expected struct name");
                std::string name = std::get<Tokenization::Identifier>(it->token).name;
                ++it;
                return NamedType{name};
            },
            [&](const Tokenization::Enum&) -> TypeNode {
                ++it;
                if (it == end || !isToken<Tokenization::Identifier>(it->token))
                    throwSyntaxError(currentPosition(it, end), "Expected enum name");
                std::string name = std::get<Tokenization::Identifier>(it->token).name;
                ++it;
                return NamedType{name};
            },
            [&](const Tokenization::LeftParenthesis&) -> TypeNode {
                ++it;
                TypeNode inner = parse(it, end);
                if (it == end || !isToken<Tokenization::RightParenthesis>(it->token))
                    throwSyntaxError(currentPosition(it, end), "Expected ')'");
                ++it;
                return inner;
            },
            [&](const Tokenization::Identifier& id) -> TypeNode {
                // пользовательский тип (через typedef, но в C-lite нет typedef – используем как имена struct/enum)
                ++it;
                return NamedType{id.name};
            },
            [&](auto) -> TypeNode {
                throwSyntaxError(currentPosition(it, end), "Expected type");
            }
        }, it->token);
    }

public:
    template<typename Iter>
    static TypeNode parse(Iter& it, Iter end, bool allowPointer, bool allowArray) {
        TypeNode type = parsePrimary(it, end);
        while (it != end) {
            if (allowPointer && isToken<Tokenization::Star>(it->token)) {
                ++it;
                type = PointerType{Boxed<TypeNode>(std::move(type))};
            }
            else if (allowArray && isToken<Tokenization::LeftBracket>(it->token)) {
                ++it;
                std::optional<int> size;
                if (it != end && isToken<Tokenization::IntLiteral>(it->token)) {
                    size = getLiteralValue<Tokenization::IntLiteral>(it->token);
                    ++it;
                }
                if (it == end || !isToken<Tokenization::RightBracket>(it->token))
                    throwSyntaxError(currentPosition(it, end), "Expected ']'");
                ++it;
                type = ArrayType{Boxed<TypeNode>(std::move(type)), size};
            }
            else break;
        }
        return type;
    }
};

// ---------- Парсер выражений ----------
class ExprParser {
public:
    template<typename Iter>
    static ExprNode parse(Iter& it, Iter end) {
        return parseAssignment(it, end);
    }

private:
    template<typename Iter>
    static ExprNode parsePrimary(Iter& it, Iter end) {
        if (it == end) throwSyntaxError(Position(), "Expected expression");

        return std::visit(overloaded{
            [&](const Tokenization::IntLiteral& lit) -> ExprNode {
                ++it; return IntLiteral{lit.value};
            },
            [&](const Tokenization::FloatLiteral& lit) -> ExprNode {
                ++it; return FloatLiteral{lit.value};
            },
            [&](const Tokenization::StringLiteral& lit) -> ExprNode {
                ++it; return StringLiteral{lit.value};
            },
            [&](const Tokenization::True&) -> ExprNode {
                ++it; return BoolLiteral{true};
            },
            [&](const Tokenization::False&) -> ExprNode {
                ++it; return BoolLiteral{false};
            },
            [&](const Tokenization::Identifier& id) -> ExprNode {
                ++it; return VariableExpr{id.name};
            },
            [&](const Tokenization::LeftParenthesis&) -> ExprNode {
                ++it;
                ExprNode expr = parse(it, end);
                if (it == end || !isToken<Tokenization::RightParenthesis>(it->token))
                    throwSyntaxError(currentPosition(it, end), "Expected ')'");
                ++it;
                return expr;
            },
            [&](const Tokenization::Sizeof&) -> ExprNode {
                ++it;
                if (it == end) throwSyntaxError(Position(), "Expected expression or type after sizeof");
                if (isToken<Tokenization::LeftParenthesis>(it->token)) {
                    // возможно тип в скобках
                    ++it;
                    TypeNode type = TypeParser::parse(it, end, true, true);
                    if (it == end || !isToken<Tokenization::RightParenthesis>(it->token))
                        throwSyntaxError(currentPosition(it, end), "Expected ')'");
                    ++it;
                    return SizeofExpr{type};
                } else {
                    ExprNode expr = parseUnary(it, end);
                    return SizeofExpr{Boxed<ExprNode>(std::move(expr))};
                }
            },
            [&](auto) -> ExprNode {
                // унарные операторы обрабатываются на следующем уровне
                return parseUnary(it, end);
            }
        }, it->token);
    }

    template<typename Iter>
    static ExprNode parseUnary(Iter& it, Iter end) {
        if (it == end) throwSyntaxError(Position(), "Expected unary expression");

        using enum UnaryOp::Op;
        return std::visit(overloaded{
            [&](const Tokenization::Not&) -> ExprNode {
                ++it; return UnaryOp{Not, Boxed<ExprNode>(parseUnary(it, end))};
            },
            [&](const Tokenization::Minus&) -> ExprNode {
                ++it; return UnaryOp{Minus, Boxed<ExprNode>(parseUnary(it, end))};
            },
            [&](const Tokenization::Plus&) -> ExprNode {
                ++it; return UnaryOp{Plus, Boxed<ExprNode>(parseUnary(it, end))};
            },
            [&](const Tokenization::And&) -> ExprNode {   // &
                ++it; return UnaryOp{AddressOf, Boxed<ExprNode>(parseUnary(it, end))};
            },
            [&](const Tokenization::Star&) -> ExprNode {  // *
                ++it; return UnaryOp{Dereference, Boxed<ExprNode>(parseUnary(it, end))};
            },
            [&](const Tokenization::Xor&) -> ExprNode {   // ^ (побитовое NOT в C? нет, ~)
                // В C-lite ~ не определён, но можно добавить
                throwSyntaxError(it->position, "Bitwise NOT (~) not supported");
            },
            [&](auto) -> ExprNode {
                return parsePostfix(it, end);
            }
        }, it->token);
    }

    template<typename Iter>
    static ExprNode parsePostfix(Iter& it, Iter end) {
        ExprNode left = parsePrimary(it, end);
        while (it != end) {
            if (isToken<Tokenization::LeftBracket>(it->token)) {
                ++it;
                ExprNode index = parse(it, end);
                if (it == end || !isToken<Tokenization::RightBracket>(it->token))
                    throwSyntaxError(currentPosition(it, end), "Expected ']'");
                ++it;
                left = IndexExpr{Boxed<ExprNode>(std::move(left)), Boxed<ExprNode>(std::move(index))};
            }
            else if (isToken<Tokenization::LeftParenthesis>(it->token)) {
                ++it;
                std::vector<Boxed<ExprNode>> args;
                if (!isToken<Tokenization::RightParenthesis>(it->token)) {
                    do {
                        args.push_back(Boxed<ExprNode>(parse(it, end)));
                        if (it != end && isToken<Tokenization::Comma>(it->token)) ++it;
                        else break;
                    } while (true);
                }
                if (it == end || !isToken<Tokenization::RightParenthesis>(it->token))
                    throwSyntaxError(currentPosition(it, end), "Expected ')'");
                ++it;
                left = CallExpr{Boxed<ExprNode>(std::move(left)), std::move(args)};
            }
            else if (isToken<Tokenization::Dot>(it->token)) {
                ++it;
                if (it == end || !isToken<Tokenization::Identifier>(it->token))
                    throwSyntaxError(currentPosition(it, end), "Expected field name");
                std::string field = std::get<Tokenization::Identifier>(it->token).name;
                ++it;
                left = FieldAccessExpr{Boxed<ExprNode>(std::move(left)), field, false};
            }
            else if (isToken<Tokenization::Arrow>(it->token)) {
                ++it;
                if (it == end || !isToken<Tokenization::Identifier>(it->token))
                    throwSyntaxError(currentPosition(it, end), "Expected field name");
                std::string field = std::get<Tokenization::Identifier>(it->token).name;
                ++it;
                left = FieldAccessExpr{Boxed<ExprNode>(std::move(left)), field, true};
            }
            else break;
        }
        return left;
    }

    // Бинарные операторы (обобщённая функция)
    template<typename Iter, typename NextParser, typename... OpTokens>
    static ExprNode parseBinary(Iter& it, Iter end, NextParser&& nextParser, const std::vector<std::pair<decltype(OpTokens{}), BinaryOp::Op>>& ops) {
        ExprNode left = nextParser(it, end);
        while (it != end) {
            bool matched = false;
            BinaryOp::Op op;
            for (const auto& [tok, opVal] : ops) {
                if (std::holds_alternative<decltype(tok)>(it->token)) {
                    ++it;
                    op = opVal;
                    matched = true;
                    break;
                }
            }
            if (!matched) break;
            ExprNode right = nextParser(it, end);
            left = BinaryOp{op, Boxed<ExprNode>(std::move(left)), Boxed<ExprNode>(std::move(right))};
        }
        return left;
    }

public:
    template<typename Iter>
    static ExprNode parseAssignment(Iter& it, Iter end) {
        // Попытка распознать множественное присваивание: левая часть – список lvalue через запятую
        // Сохраняем позицию, пробуем прочитать список идентификаторов до '='
        auto saved = it;
        std::vector<Boxed<ExprNode>> lefts;
        try {
            // Парсим левые части: они должны быть lvalue (переменные, разыменования, поля, индексы)
            while (true) {
                // lvalue – это postfix-expression (идентификатор, разыменование, доступ)
                ExprNode lval = parsePostfix(it, end);
                // Проверяем, что это lvalue (упрощённо: любые postfix, но семантика потом)
                lefts.push_back(Boxed<ExprNode>(std::move(lval)));
                if (it == end || !isToken<Tokenization::Comma>(it->token)) break;
                ++it;
            }
            if (it != end && isToken<Tokenization::Assign>(it->token)) {
                ++it; // пропускаем '='
                // Парсим правые выражения
                std::vector<Boxed<ExprNode>> rights;
                while (true) {
                    rights.push_back(Boxed<ExprNode>(parseAssignment(it, end)));
                    if (it == end || !isToken<Tokenization::Comma>(it->token)) break;
                    ++it;
                }
                if (lefts.size() != rights.size()) {
                    throwSyntaxError(currentPosition(it, end), "Number of left and right values in multi-assign mismatch");
                }
                return MultiAssignExpr{std::move(lefts), std::move(rights)};
            }
        } catch (...) {
            // не удалось распарсить множественное присваивание – откатываемся
            it = saved;
        }
        // Обычное присваивание или составное
        ExprNode left = parseConditional(it, end);
        if (it != end) {
            using enum AssignExpr::Op;
            if (isToken<Tokenization::Assign>(it->token)) {
                ++it;
                ExprNode right = parseAssignment(it, end);
                return AssignExpr{Boxed<ExprNode>(std::move(left)), Boxed<ExprNode>(std::move(right)), AssignExpr::Op::Assign};
            }
            // Составные присваивания
            #define COMPOUND_ASSIGN(op_token, op_enum) \
            if (isToken<Tokenization::op_token>(it->token)) { \
                ++it; ExprNode right = parseAssignment(it, end); \
                return AssignExpr{Boxed<ExprNode>(std::move(left)), Boxed<ExprNode>(std::move(right)), AssignExpr::Op::op_enum}; \
            }
            COMPOUND_ASSIGN(PlusAssign, AddAssign)
            COMPOUND_ASSIGN(MinusAssign, SubAssign)
            COMPOUND_ASSIGN(StarAssign, MulAssign)
            COMPOUND_ASSIGN(SlashAssign, DivAssign)
            COMPOUND_ASSIGN(PercentAssign, RemAssign)
            COMPOUND_ASSIGN(ShiftLeftAssign, ShiftLeftAssign)
            COMPOUND_ASSIGN(ShiftRightAssign, ShiftRightAssign)
            COMPOUND_ASSIGN(AndAssign, AndAssign)
            COMPOUND_ASSIGN(XorAssign, XorAssign)
            COMPOUND_ASSIGN(OrAssign, OrAssign)
            #undef COMPOUND_ASSIGN
        }
        return left;
    }

    template<typename Iter>
    static ExprNode parseConditional(Iter& it, Iter end) {
        ExprNode cond = parseLogicalOr(it, end);
        if (it != end && isToken<Tokenization::Question>(it->token)) {
            ++it;
            ExprNode thenExpr = parseAssignment(it, end);
            if (it == end || !isToken<Tokenization::Colon>(it->token))
                throwSyntaxError(currentPosition(it, end), "Expected ':'");
            ++it;
            ExprNode elseExpr = parseAssignment(it, end);
            return ConditionalExpr{Boxed<ExprNode>(std::move(cond)),
                                   Boxed<ExprNode>(std::move(thenExpr)),
                                   Boxed<ExprNode>(std::move(elseExpr))};
        }
        return cond;
    }

    // Уровни приоритета (следуя стандарту C)
    template<typename Iter>
    static ExprNode parseLogicalOr(Iter& it, Iter end) {
        static const std::vector<std::pair<Tokenization::OrOr, BinaryOp::Op>> ops = {{Tokenization::OrOr{}, BinaryOp::Op::LogicalOr}};
        return parseBinary(it, end, parseLogicalAnd, ops);
    }
    template<typename Iter>
    static ExprNode parseLogicalAnd(Iter& it, Iter end) {
        static const std::vector<std::pair<Tokenization::AndAnd, BinaryOp::Op>> ops = {{Tokenization::AndAnd{}, BinaryOp::Op::LogicalAnd}};
        return parseBinary(it, end, parseBitOr, ops);
    }
    template<typename Iter>
    static ExprNode parseBitOr(Iter& it, Iter end) {
        static const std::vector<std::pair<Tokenization::Or, BinaryOp::Op>> ops = {{Tokenization::Or{}, BinaryOp::Op::BitOr}};
        return parseBinary(it, end, parseBitXor, ops);
    }
    template<typename Iter>
    static ExprNode parseBitXor(Iter& it, Iter end) {
        static const std::vector<std::pair<Tokenization::Xor, BinaryOp::Op>> ops = {{Tokenization::Xor{}, BinaryOp::Op::BitXor}};
        return parseBinary(it, end, parseBitAnd, ops);
    }
    template<typename Iter>
    static ExprNode parseBitAnd(Iter& it, Iter end) {
        static const std::vector<std::pair<Tokenization::And, BinaryOp::Op>> ops = {{Tokenization::And{}, BinaryOp::Op::BitAnd}};
        return parseBinary(it, end, parseEquality, ops);
    }
    template<typename Iter>
    static ExprNode parseEquality(Iter& it, Iter end) {
        static const std::vector<std::pair<Tokenization::Equal, BinaryOp::Op>> eq = {{Tokenization::Equal{}, BinaryOp::Op::Equal}};
        static const std::vector<std::pair<Tokenization::NotEqual, BinaryOp::Op>> ne = {{Tokenization::NotEqual{}, BinaryOp::Op::NotEqual}};
        auto left = parseRelational(it, end);
        while (it != end) {
            if (isToken<Tokenization::Equal>(it->token)) {
                ++it; left = BinaryOp{BinaryOp::Op::Equal, Boxed<ExprNode>(std::move(left)), Boxed<ExprNode>(parseRelational(it, end))};
            } else if (isToken<Tokenization::NotEqual>(it->token)) {
                ++it; left = BinaryOp{BinaryOp::Op::NotEqual, Boxed<ExprNode>(std::move(left)), Boxed<ExprNode>(parseRelational(it, end))};
            } else break;
        }
        return left;
    }
    template<typename Iter>
    static ExprNode parseRelational(Iter& it, Iter end) {
        auto left = parseShift(it, end);
        while (it != end) {
            if (isToken<Tokenization::Less>(it->token)) {
                ++it; left = BinaryOp{BinaryOp::Op::Less, Boxed<ExprNode>(std::move(left)), Boxed<ExprNode>(parseShift(it, end))};
            } else if (isToken<Tokenization::LessEqual>(it->token)) {
                ++it; left = BinaryOp{BinaryOp::Op::LessEqual, Boxed<ExprNode>(std::move(left)), Boxed<ExprNode>(parseShift(it, end))};
            } else if (isToken<Tokenization::Greater>(it->token)) {
                ++it; left = BinaryOp{BinaryOp::Op::Greater, Boxed<ExprNode>(std::move(left)), Boxed<ExprNode>(parseShift(it, end))};
            } else if (isToken<Tokenization::GreaterEqual>(it->token)) {
                ++it; left = BinaryOp{BinaryOp::Op::GreaterEqual, Boxed<ExprNode>(std::move(left)), Boxed<ExprNode>(parseShift(it, end))};
            } else break;
        }
        return left;
    }
    template<typename Iter>
    static ExprNode parseShift(Iter& it, Iter end) {
        auto left = parseAdditive(it, end);
        while (it != end) {
            if (isToken<Tokenization::ShiftLeft>(it->token)) {
                ++it; left = BinaryOp{BinaryOp::Op::ShiftLeft, Boxed<ExprNode>(std::move(left)), Boxed<ExprNode>(parseAdditive(it, end))};
            } else if (isToken<Tokenization::ShiftRight>(it->token)) {
                ++it; left = BinaryOp{BinaryOp::Op::ShiftRight, Boxed<ExprNode>(std::move(left)), Boxed<ExprNode>(parseAdditive(it, end))};
            } else break;
        }
        return left;
    }
    template<typename Iter>
    static ExprNode parseAdditive(Iter& it, Iter end) {
        auto left = parseMultiplicative(it, end);
        while (it != end) {
            if (isToken<Tokenization::Plus>(it->token)) {
                ++it; left = BinaryOp{BinaryOp::Op::Add, Boxed<ExprNode>(std::move(left)), Boxed<ExprNode>(parseMultiplicative(it, end))};
            } else if (isToken<Tokenization::Minus>(it->token)) {
                ++it; left = BinaryOp{BinaryOp::Op::Sub, Boxed<ExprNode>(std::move(left)), Boxed<ExprNode>(parseMultiplicative(it, end))};
            } else break;
        }
        return left;
    }
    template<typename Iter>
    static ExprNode parseMultiplicative(Iter& it, Iter end) {
        auto left = parseUnary(it, end);
        while (it != end) {
            if (isToken<Tokenization::Star>(it->token)) {
                ++it; left = BinaryOp{BinaryOp::Op::Mul, Boxed<ExprNode>(std::move(left)), Boxed<ExprNode>(parseUnary(it, end))};
            } else if (isToken<Tokenization::Slash>(it->token)) {
                ++it; left = BinaryOp{BinaryOp::Op::Div, Boxed<ExprNode>(std::move(left)), Boxed<ExprNode>(parseUnary(it, end))};
            } else if (isToken<Tokenization::Percent>(it->token)) {
                ++it; left = BinaryOp{BinaryOp::Op::Rem, Boxed<ExprNode>(std::move(left)), Boxed<ExprNode>(parseUnary(it, end))};
            } else break;
        }
        return left;
    }
};

// ---------- Парсер операторов (statements) ----------
class StmtParser {
public:
    template<typename Iter>
    static StmtNode parse(Iter& it, Iter end) {
        return parseStatement(it, end);
    }

    template<typename Iter>
    static BlockStmt parseBlock(Iter& it, Iter end) {
        if (it == end || !isToken<Tokenization::LeftBrace>(it->token))
            throwSyntaxError(currentPosition(it, end), "Expected '{'");
        ++it;
        std::vector<Boxed<StmtNode>> stmts;
        while (it != end && !isToken<Tokenization::RightBrace>(it->token)) {
            stmts.push_back(Boxed<StmtNode>(parseStatement(it, end)));
        }
        if (it == end) throwSyntaxError(Position(), "Expected '}'");
        ++it;
        return BlockStmt{std::move(stmts)};
    }

private:
    template<typename Iter>
    static StmtNode parseStatement(Iter& it, Iter end) {
        if (it == end) throwSyntaxError(Position(), "Expected statement");

        return std::visit(overloaded{
            [&](const Tokenization::If&) -> StmtNode {
                ++it;
                if (it == end || !isToken<Tokenization::LeftParenthesis>(it->token))
                    throwSyntaxError(currentPosition(it, end), "Expected '(' after if");
                ++it;
                ExprNode cond = ExprParser::parse(it, end);
                if (it == end || !isToken<Tokenization::RightParenthesis>(it->token))
                    throwSyntaxError(currentPosition(it, end), "Expected ')'");
                ++it;
                StmtNode thenStmt = parseStatement(it, end);
                std::optional<Boxed<StmtNode>> elseStmt;
                if (it != end && isToken<Tokenization::Else>(it->token)) {
                    ++it;
                    elseStmt = Boxed<StmtNode>(parseStatement(it, end));
                }
                return IfStmt{Boxed<ExprNode>(std::move(cond)),
                             Boxed<StmtNode>(std::move(thenStmt)),
                             elseStmt ? std::optional(std::move(elseStmt)) : std::nullopt};
            },
            [&](const Tokenization::While&) -> StmtNode {
                ++it;
                if (it == end || !isToken<Tokenization::LeftParenthesis>(it->token))
                    throwSyntaxError(currentPosition(it, end), "Expected '(' after while");
                ++it;
                ExprNode cond = ExprParser::parse(it, end);
                if (it == end || !isToken<Tokenization::RightParenthesis>(it->token))
                    throwSyntaxError(currentPosition(it, end), "Expected ')'");
                ++it;
                StmtNode body = parseStatement(it, end);
                return WhileStmt{Boxed<ExprNode>(std::move(cond)), Boxed<StmtNode>(std::move(body))};
            },
            [&](const Tokenization::Do&) -> StmtNode {
                ++it;
                StmtNode body = parseStatement(it, end);
                if (it == end || !isToken<Tokenization::While>(it->token))
                    throwSyntaxError(currentPosition(it, end), "Expected 'while' after do body");
                ++it;
                if (it == end || !isToken<Tokenization::LeftParenthesis>(it->token))
                    throwSyntaxError(currentPosition(it, end), "Expected '(' after while");
                ++it;
                ExprNode cond = ExprParser::parse(it, end);
                if (it == end || !isToken<Tokenization::RightParenthesis>(it->token))
                    throwSyntaxError(currentPosition(it, end), "Expected ')'");
                ++it;
                if (it == end || !isToken<Tokenization::Semicolon>(it->token))
                    throwSyntaxError(currentPosition(it, end), "Expected ';' after do-while");
                ++it;
                return DoWhileStmt{Boxed<StmtNode>(std::move(body)), Boxed<ExprNode>(std::move(cond))};
            },
            [&](const Tokenization::For&) -> StmtNode {
                ++it;
                if (it == end || !isToken<Tokenization::LeftParenthesis>(it->token))
                    throwSyntaxError(currentPosition(it, end), "Expected '(' after for");
                ++it;
                std::optional<Boxed<StmtNode>> init;
                if (!isToken<Tokenization::Semicolon>(it->token)) {
                    // может быть объявление переменной или выражение
                    // пробуем объявление
                    if (isToken<Tokenization::Int>(it->token) || isToken<Tokenization::Unsigned>(it->token) ||
                        isToken<Tokenization::Float>(it->token) || isToken<Tokenization::Bool>(it->token) ||
                        isToken<Tokenization::String>(it->token) || isToken<Tokenization::Struct>(it->token) ||
                        isToken<Tokenization::Enum>(it->token)) {
                        // объявление переменной
                        TypeNode type = TypeParser::parse(it, end, true, true);
                        if (it == end || !isToken<Tokenization::Identifier>(it->token))
                            throwSyntaxError(currentPosition(it, end), "Expected variable name");
                        std::string name = std::get<Tokenization::Identifier>(it->token).name;
                        ++it;
                        std::optional<Boxed<ExprNode>> initializer;
                        if (it != end && isToken<Tokenization::Assign>(it->token)) {
                            ++it;
                            initializer = Boxed<ExprNode>(ExprParser::parse(it, end));
                        }
                        init = Boxed<StmtNode>(VarDeclStmt{std::move(type), name, initializer});
                    } else {
                        // выражение
                        ExprNode expr = ExprParser::parse(it, end);
                        init = Boxed<StmtNode>(ExprStmt{Boxed<ExprNode>(std::move(expr))});
                    }
                }
                if (it == end || !isToken<Tokenization::Semicolon>(it->token))
                    throwSyntaxError(currentPosition(it, end), "Expected ';' after for init");
                ++it;
                std::optional<Boxed<ExprNode>> condition;
                if (!isToken<Tokenization::Semicolon>(it->token)) {
                    condition = Boxed<ExprNode>(ExprParser::parse(it, end));
                }
                if (it == end || !isToken<Tokenization::Semicolon>(it->token))
                    throwSyntaxError(currentPosition(it, end), "Expected ';' after for condition");
                ++it;
                std::optional<Boxed<ExprNode>> increment;
                if (!isToken<Tokenization::RightParenthesis>(it->token)) {
                    increment = Boxed<ExprNode>(ExprParser::parse(it, end));
                }
                if (it == end || !isToken<Tokenization::RightParenthesis>(it->token))
                    throwSyntaxError(currentPosition(it, end), "Expected ')' after for clauses");
                ++it;
                StmtNode body = parseStatement(it, end);
                return ForStmt{init, condition, increment, Boxed<StmtNode>(std::move(body))};
            },
            [&](const Tokenization::Switch&) -> StmtNode {
                ++it;
                if (it == end || !isToken<Tokenization::LeftParenthesis>(it->token))
                    throwSyntaxError(currentPosition(it, end), "Expected '(' after switch");
                ++it;
                ExprNode control = ExprParser::parse(it, end);
                if (it == end || !isToken<Tokenization::RightParenthesis>(it->token))
                    throwSyntaxError(currentPosition(it, end), "Expected ')'");
                ++it;
                StmtNode body = parseStatement(it, end);
                return SwitchStmt{Boxed<ExprNode>(std::move(control)), Boxed<StmtNode>(std::move(body))};
            },
            [&](const Tokenization::Case&) -> StmtNode {
                // отдельно не парсится, обрабатывается внутри блока switch – упрощённо
                throwSyntaxError(it->position, "Case label outside switch");
            },
            [&](const Tokenization::Default&) -> StmtNode {
                throwSyntaxError(it->position, "Default label outside switch");
            },
            [&](const Tokenization::Break&) -> StmtNode {
                ++it;
                if (it == end || !isToken<Tokenization::Semicolon>(it->token))
                    throwSyntaxError(currentPosition(it, end), "Expected ';' after break");
                ++it;
                return BreakStmt{};
            },
            [&](const Tokenization::Continue&) -> StmtNode {
                ++it;
                if (it == end || !isToken<Tokenization::Semicolon>(it->token))
                    throwSyntaxError(currentPosition(it, end), "Expected ';' after continue");
                ++it;
                return ContinueStmt{};
            },
            [&](const Tokenization::Return&) -> StmtNode {
                ++it;
                std::vector<Boxed<ExprNode>> values;
                if (it != end && !isToken<Tokenization::Semicolon>(it->token)) {
                    do {
                        values.push_back(Boxed<ExprNode>(ExprParser::parse(it, end)));
                        if (it != end && isToken<Tokenization::Comma>(it->token)) ++it;
                        else break;
                    } while (true);
                }
                if (it == end || !isToken<Tokenization::Semicolon>(it->token))
                    throwSyntaxError(currentPosition(it, end), "Expected ';' after return");
                ++it;
                return ReturnStmt{std::move(values)};
            },
            [&](const Tokenization::Goto&) -> StmtNode {
                ++it;
                if (it == end || !isToken<Tokenization::Identifier>(it->token))
                    throwSyntaxError(currentPosition(it, end), "Expected label name after goto");
                std::string label = std::get<Tokenization::Identifier>(it->token).name;
                ++it;
                if (it == end || !isToken<Tokenization::Semicolon>(it->token))
                    throwSyntaxError(currentPosition(it, end), "Expected ';' after goto");
                ++it;
                return GotoStmt{label};
            },
            [&](const Tokenization::Identifier& id) -> StmtNode {
                // может быть меткой: идентификатор ':' или выражение-оператор
                auto saved = it;
                ++it;
                if (it != end && isToken<Tokenization::Colon>(it->token)) {
                    ++it;
                    StmtNode stmt = parseStatement(it, end);
                    return LabelStmt{id.name, Boxed<StmtNode>(std::move(stmt))};
                }
                it = saved;
                ExprNode expr = ExprParser::parse(it, end);
                if (it == end || !isToken<Tokenization::Semicolon>(it->token))
                    throwSyntaxError(currentPosition(it, end), "Expected ';' after expression statement");
                ++it;
                return ExprStmt{Boxed<ExprNode>(std::move(expr))};
            },
            [&](const Tokenization::LeftBrace&) -> StmtNode {
                BlockStmt block = parseBlock(it, end);
                return block;
            },
            [&](auto) -> StmtNode {
                // объявление переменной в блоке (не for)
                TypeNode type = TypeParser::parse(it, end, true, true);
                if (it == end || !isToken<Tokenization::Identifier>(it->token))
                    throwSyntaxError(currentPosition(it, end), "Expected variable name");
                std::string name = std::get<Tokenization::Identifier>(it->token).name;
                ++it;
                std::optional<Boxed<ExprNode>> initializer;
                if (it != end && isToken<Tokenization::Assign>(it->token)) {
                    ++it;
                    initializer = Boxed<ExprNode>(ExprParser::parse(it, end));
                }
                if (it == end || !isToken<Tokenization::Semicolon>(it->token))
                    throwSyntaxError(currentPosition(it, end), "Expected ';' after variable declaration");
                ++it;
                return VarDeclStmt{std::move(type), name, initializer};
            }
        }, it->token);
    }
};

// ---------- Парсер определений верхнего уровня ----------
class DefParser {
public:
    template<typename Iter>
    static DefNode parse(Iter& it, Iter end) {
        if (it == end) throwSyntaxError(Position(), "Expected definition");

        return std::visit(overloaded{
            [&](const Tokenization::Int&) -> DefNode {
                return parseFunctionOrGlobal(it, end, IntType{});
            },
            [&](const Tokenization::Unsigned&) -> DefNode {
                return parseFunctionOrGlobal(it, end, UnsignedType{});
            },
            [&](const Tokenization::Float&) -> DefNode {
                return parseFunctionOrGlobal(it, end, FloatType{});
            },
            [&](const Tokenization::Bool&) -> DefNode {
                return parseFunctionOrGlobal(it, end, BoolType{});
            },
            [&](const Tokenization::String&) -> DefNode {
                return parseFunctionOrGlobal(it, end, StringType{});
            },
            [&](const Tokenization::Void&) -> DefNode {
                return parseFunctionOrGlobal(it, end, VoidType{});
            },
            [&](const Tokenization::Struct&) -> DefNode {
                ++it;
                if (it == end || !isToken<Tokenization::Identifier>(it->token))
                    throwSyntaxError(currentPosition(it, end), "Expected struct name");
                std::string name = std::get<Tokenization::Identifier>(it->token).name;
                ++it;
                if (it == end || !isToken<Tokenization::LeftBrace>(it->token))
                    throwSyntaxError(currentPosition(it, end), "Expected '{' for struct definition");
                ++it;
                std::vector<std::pair<std::string, TypeNode>> fields;
                while (it != end && !isToken<Tokenization::RightBrace>(it->token)) {
                    TypeNode fieldType = TypeParser::parse(it, end, true, true);
                    if (it == end || !isToken<Tokenization::Identifier>(it->token))
                        throwSyntaxError(currentPosition(it, end), "Expected field name");
                    std::string fname = std::get<Tokenization::Identifier>(it->token).name;
                    ++it;
                    if (it != end && isToken<Tokenization::Semicolon>(it->token)) ++it;
                    else throwSyntaxError(currentPosition(it, end), "Expected ';' after field");
                    fields.emplace_back(fname, std::move(fieldType));
                }
                if (it == end) throwSyntaxError(Position(), "Expected '}'");
                ++it;
                return StructDef{name, std::move(fields)};
            },
            [&](const Tokenization::Enum&) -> DefNode {
                ++it;
                if (it == end || !isToken<Tokenization::Identifier>(it->token))
                    throwSyntaxError(currentPosition(it, end), "Expected enum name");
                std::string name = std::get<Tokenization::Identifier>(it->token).name;
                ++it;
                if (it == end || !isToken<Tokenization::LeftBrace>(it->token))
                    throwSyntaxError(currentPosition(it, end), "Expected '{' for enum definition");
                ++it;
                std::vector<std::pair<std::string, std::optional<int>>> enumerators;
                while (it != end && !isToken<Tokenization::RightBrace>(it->token)) {
                    if (it == end || !isToken<Tokenization::Identifier>(it->token))
                        throwSyntaxError(currentPosition(it, end), "Expected enumerator name");
                    std::string ename = std::get<Tokenization::Identifier>(it->token).name;
                    ++it;
                    std::optional<int> value;
                    if (it != end && isToken<Tokenization::Assign>(it->token)) {
                        ++it;
                        if (it == end || !isToken<Tokenization::IntLiteral>(it->token))
                            throwSyntaxError(currentPosition(it, end), "Expected integer constant");
                        value = std::get<Tokenization::IntLiteral>(it->token).value;
                        ++it;
                    }
                    enumerators.emplace_back(ename, value);
                    if (it != end && isToken<Tokenization::Comma>(it->token)) ++it;
                    else break;
                }
                if (it == end) throwSyntaxError(Position(), "Expected '}'");
                ++it;
                return EnumDef{name, std::move(enumerators)};
            },
            [&](auto) -> DefNode {
                throwSyntaxError(it->position, "Expected function, global variable, struct or enum definition");
            }
        }, it->token);
    }

private:
    template<typename Iter, typename BaseType>
    static DefNode parseFunctionOrGlobal(Iter& it, Iter end, BaseType baseType) {
        // Парсим полный тип (указатели, массивы)
        TypeNode type = TypeParser::parse(it, end, true, true);
        if (it == end || !isToken<Tokenization::Identifier>(it->token))
            throwSyntaxError(currentPosition(it, end), "Expected function or variable name");
        std::string name = std::get<Tokenization::Identifier>(it->token).name;
        ++it;
        // Если следующий токен '(', то это функция
        if (it != end && isToken<Tokenization::LeftParenthesis>(it->token)) {
            // Функция
            ++it;
            std::vector<Param> params;
            bool variadic = false;
            if (!isToken<Tokenization::RightParenthesis>(it->token)) {
                do {
                    if (isToken<Tokenization::Ellipsis>(it->token)) {
                        variadic = true;
                        ++it;
                        break;
                    }
                    TypeNode paramType = TypeParser::parse(it, end, true, true);
                    std::string paramName;
                    if (it != end && isToken<Tokenization::Identifier>(it->token)) {
                        paramName = std::get<Tokenization::Identifier>(it->token).name;
                        ++it;
                    }
                    params.push_back({paramName, std::move(paramType)});
                    if (it != end && isToken<Tokenization::Comma>(it->token)) ++it;
                    else break;
                } while (true);
            }
            if (it == end || !isToken<Tokenization::RightParenthesis>(it->token))
                throwSyntaxError(currentPosition(it, end), "Expected ')' after parameters");
            ++it;
            // Возвращаемые типы (множественные)
            std::vector<TypeNode> returnTypes;
            if (it != end && isToken<Tokenization::Colon>(it->token)) {
                ++it;
                // может быть кортеж или одиночный тип
                if (isToken<Tokenization::LeftParenthesis>(it->token)) {
                    ++it;
                    do {
                        returnTypes.push_back(TypeParser::parse(it, end, true, true));
                        if (it != end && isToken<Tokenization::Comma>(it->token)) ++it;
                        else break;
                    } while (true);
                    if (it == end || !isToken<Tokenization::RightParenthesis>(it->token))
                        throwSyntaxError(currentPosition(it, end), "Expected ')' after multiple return types");
                    ++it;
                } else {
                    returnTypes.push_back(TypeParser::parse(it, end, true, true));
                }
            } else {
                // по умолчанию возвращаемый тип – один (уже есть type)
                returnTypes.push_back(std::move(type));
            }
            // Тело функции (блок или ';' для объявления)
            std::optional<BlockStmt> body;
            if (it != end && isToken<Tokenization::Semicolon>(it->token)) {
                ++it;
                // объявление без тела
            } else if (it != end && isToken<Tokenization::LeftBrace>(it->token)) {
                body = StmtParser::parseBlock(it, end);
            } else {
                throwSyntaxError(currentPosition(it, end), "Expected function body or ';'");
            }
            return FunctionDef{name, std::move(params), std::move(returnTypes), variadic, body};
        } else {
            // Глобальная переменная
            std::optional<ExprNode> initializer;
            if (it != end && isToken<Tokenization::Assign>(it->token)) {
                ++it;
                initializer = ExprParser::parse(it, end);
            }
            if (it == end || !isToken<Tokenization::Semicolon>(it->token))
                throwSyntaxError(currentPosition(it, end), "Expected ';' after global variable");
            ++it;
            return GlobalVarDef{std::move(type), name, initializer};
        }
    }
};

} // namespace detail

ParsingInfo Parser::parse(const std::vector<Tokenization::TokenInfo>& tokens) {
    auto it = tokens.begin();
    auto end = tokens.end();
    TranslationUnit tu;
    std::unordered_map<const void*, Position> positions;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;

    while (it != end) {
        try {
            auto pos = it->position;
            DefNode def = detail::DefParser::parse(it, end);
            tu.definitions.push_back(std::move(def));
            positions[&tu.definitions.back()] = pos;
        } catch (const std::runtime_error& e) {
            errors.push_back(e.what());
            // пропустить до следующего определения (просто перейти к следующему токену)
            if (it != end) ++it;
        }
    }
    return {std::move(tu), std::move(positions), std::move(errors), std::move(warnings)};
}

} // namespace Parsing