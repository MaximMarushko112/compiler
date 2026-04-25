#include "../../include/Parsing/Parser.hpp"
#include <cassert>
#include <iostream>
#include <optional>
#include <variant>
#include <../include/Utils/Overloaded.hpp>

namespace Parsing {

namespace detail {

auto printToken = [](const auto& it, const auto& end, const char* func) {
    if (it == end)
        std::cerr << func << ": END OF TOKENS\n";
    else
        std::cerr << func << ": token = " << it->token << " at " << it->position.toString() << "\n";
};

// ---------- Утилиты ----------
[[noreturn]] void throwSyntaxError(const Util::Position& pos, const std::string& message) {
    throw std::runtime_error("Syntax error at " + pos.toString() + ": " + message);
}

[[noreturn]] void throwUnexpectedEndOfTokenRange(const std::string& message) {
    std::string err = "Unexpected end of tokens range";
    if (!message.empty()) err += ", " + message;
    throw std::runtime_error(err);
}

template<typename Iter>
const Util::Position& currentPosition(Iter it, Iter end) {
    if (it == end) throw std::runtime_error("Unexpected end of tokens");
    return it->position;
}

template<typename Iter>
void expectToken(Iter& it, Iter end, auto tokenType, const std::string& expected) {
    if (it == end) throwSyntaxError(it[-1].Position, "Unexpected end, expected " + expected);
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
        std::cerr << "TypeParser: processed pointer/array, type built" << std::endl;
        return parsePrimary(it, end);
    }

private:
    template<typename Iter>
    static TypeNode parsePrimary(Iter& it, Iter end) {
        printToken(it, end, "TypeParser::parsePrimary");
        if (it == end) throwSyntaxError(Util::Position(), "Expected type");
        
        return std::visit(Util::overloaded{
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
                return std::move(inner);
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
        printToken(it, end, "TypeParser::parse(allow)");
        TypeNode type = parsePrimary(it, end);
        while (it != end) {
            if (allowPointer && isToken<Tokenization::Star>(it->token)) {
                ++it;
                type = PointerType{Util::makeBoxed<TypeNode>(std::move(type))};
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
                type = ArrayType{Util::makeBoxed<TypeNode>(std::move(type)), size};
            }
            else break;
        }
        return std::move(type);
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
        printToken(it, end, "ExprParser::parsePrimary");
        if (it == end) throwSyntaxError(Util::Position(), "Expected expression");

        return std::visit(Util::overloaded{
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
                return std::move(expr);
            },
            [&](const Tokenization::Sizeof&) -> ExprNode {
                ++it;
                if (it == end) throwSyntaxError(Util::Position(), "Expected expression or type after sizeof");
                if (isToken<Tokenization::LeftParenthesis>(it->token)) {
                    // возможно тип в скобках
                    ++it;
                    TypeNode type = TypeParser::parse(it, end, true, true);
                    if (it == end || !isToken<Tokenization::RightParenthesis>(it->token))
                        throwSyntaxError(currentPosition(it, end), "Expected ')'");
                    ++it;
                    return SizeofExpr{std::move(type)};
                } else {
                    ExprNode expr = parseUnary(it, end);
                    return SizeofExpr{Util::makeBoxed<ExprNode>(std::move(expr))};
                }
            },
            [&](auto) -> ExprNode {
                throwSyntaxError(currentPosition(it, end), "Expected expression");
            }
        }, it->token);
    }

    template<typename Iter>
    static ExprNode parseUnary(Iter& it, Iter end) {
        printToken(it, end, "ExprParser::parseUnary");
        if (it == end) throwSyntaxError(Util::Position(), "Expected unary expression");

        using enum UnaryOp::Op;
        return std::visit(Util::overloaded{
            [&](const Tokenization::Not&) -> ExprNode {
                ++it; return UnaryOp{Not, Util::makeBoxed<ExprNode>(parseUnary(it, end))};
            },
            [&](const Tokenization::Minus&) -> ExprNode {
                ++it; return UnaryOp{Minus, Util::makeBoxed<ExprNode>(parseUnary(it, end))};
            },
            [&](const Tokenization::Plus&) -> ExprNode {
                ++it; return UnaryOp{Plus, Util::makeBoxed<ExprNode>(parseUnary(it, end))};
            },
            [&](const Tokenization::And&) -> ExprNode {   // &
                ++it; return UnaryOp{AddressOf, Util::makeBoxed<ExprNode>(parseUnary(it, end))};
            },
            [&](const Tokenization::Star&) -> ExprNode {  // *
                ++it; return UnaryOp{Dereference, Util::makeBoxed<ExprNode>(parseUnary(it, end))};
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
        printToken(it, end, "ExprParser::parsePostfix");
        ExprNode left = parsePrimary(it, end);
        while (it != end) {
            if (isToken<Tokenization::LeftBracket>(it->token)) {
                std::cerr << "parsePostfix: processing '[' at " << it->position.toString() << std::endl;
                ++it;
                ExprNode index = parse(it, end);
                if (it == end || !isToken<Tokenization::RightBracket>(it->token))
                    throwSyntaxError(currentPosition(it, end), "Expected ']'");
                ++it;
                left = IndexExpr{Util::makeBoxed<ExprNode>(std::move(left)), Util::makeBoxed<ExprNode>(std::move(index))};
            }
            else if (isToken<Tokenization::LeftParenthesis>(it->token)) {
                ++it;
                std::vector<Util::Boxed<ExprNode>> args;
                if (!isToken<Tokenization::RightParenthesis>(it->token)) {
                    do {
                        args.push_back(Util::makeBoxed<ExprNode>(parse(it, end)));
                        if (it != end && isToken<Tokenization::Comma>(it->token)) ++it;
                        else break;
                    } while (true);
                }
                if (it == end || !isToken<Tokenization::RightParenthesis>(it->token))
                    throwSyntaxError(currentPosition(it, end), "Expected ')'");
                ++it;
                std::cerr << "parsePostfix: after call, token = " << (it != end ? it->token : Tokenization::TokenVariant{}) << std::endl;
                left = CallExpr{Util::makeBoxed<ExprNode>(std::move(left)), std::move(args)};
            }
            else if (isToken<Tokenization::Dot>(it->token)) {
                ++it;
                if (it == end || !isToken<Tokenization::Identifier>(it->token))
                    throwSyntaxError(currentPosition(it, end), "Expected field name");
                std::string field = std::get<Tokenization::Identifier>(it->token).name;
                ++it;
                left = FieldAccessExpr{Util::makeBoxed<ExprNode>(std::move(left)), field, false};
            }
            else if (isToken<Tokenization::Arrow>(it->token)) {
                ++it;
                if (it == end || !isToken<Tokenization::Identifier>(it->token))
                    throwSyntaxError(currentPosition(it, end), "Expected field name");
                std::string field = std::get<Tokenization::Identifier>(it->token).name;
                ++it;
                left = FieldAccessExpr{Util::makeBoxed<ExprNode>(std::move(left)), field, true};
            }
            else break;
        }
        return std::move(left);
    }

    // Бинарные операторы (обобщённая функция)
    template<typename Iter, typename NextParser>
    static ExprNode parseBinary(Iter& it, Iter end, NextParser&& nextParser, const std::vector<std::pair<Tokenization::TokenVariant, BinaryOp::Op>>& ops) {
        printToken(it, end, "ExprParser::parseBinary");
        ExprNode left = nextParser(it, end);
        while (it != end) {
            bool matched = false;
            BinaryOp::Op op;
            for (const auto& [tokType, opVal] : ops) {
                if (it->token == tokType) {
                    matched = true;
                    op = opVal;
                    ++it;
                    break;
                }
            }
            if (!matched) break;
            ExprNode right = nextParser(it, end);
            left = BinaryOp{op, Util::makeBoxed<ExprNode>(std::move(left)), Util::makeBoxed<ExprNode>(std::move(right))};
        }
        return std::move(left);
    }

public:
    template<typename Iter>
    static ExprNode parseAssignment(Iter& it, Iter end) {
        printToken(it, end, "ExprParser::parseAssignment");
        auto saved = it;
        std::vector<Util::Boxed<ExprNode>> lefts;
        bool isMultiAssign = false;
        try {
            // Парсим левые части (список lvalue через запятую)
            while (true) {
                ExprNode lval = parsePostfix(it, end);
                lefts.push_back(Util::makeBoxed<ExprNode>(std::move(lval)));
                if (it == end || !isToken<Tokenization::Comma>(it->token)) break;
                ++it;
            }
            // Если следующий токен '=', то это множественное присваивание
            if (it != end && isToken<Tokenization::Assign>(it->token)) {
                isMultiAssign = true;
                ++it; // пропускаем '='
                std::vector<Util::Boxed<ExprNode>> rights;
                while (true) {
                    rights.push_back(Util::makeBoxed<ExprNode>(parseAssignment(it, end)));
                    if (it == end || !isToken<Tokenization::Comma>(it->token)) break;
                    ++it;
                }

                std::cerr << "parseAssignment: returning MultiAssignExpr, token now = " << (it != end ? it->token : Tokenization::TokenVariant{}) << std::endl;
                return MultiAssignExpr{std::move(lefts), std::move(rights)};
            }
        } catch (...) {
            // При любой ошибке откатываемся
            it = saved;
        }
        // Если множественное присваивание не удалось, откатываем итератор
        if (!isMultiAssign) {
            it = saved;
        }
        // Обычное присваивание или составное
        ExprNode left = parseConditional(it, end);
        if (it != end) {
            using enum AssignExpr::Op;
            if (isToken<Tokenization::Assign>(it->token)) {
                ++it;
                ExprNode right = parseAssignment(it, end);
                return AssignExpr{Util::makeBoxed<ExprNode>(std::move(left)), Util::makeBoxed<ExprNode>(std::move(right)), AssignExpr::Op::Assign};
            }
            // Составные присваивания
            #define COMPOUND_ASSIGN(op_token, op_enum) \
            if (isToken<Tokenization::op_token>(it->token)) { \
                ++it; ExprNode right = parseAssignment(it, end); \
                return AssignExpr{Util::makeBoxed<ExprNode>(std::move(left)), Util::makeBoxed<ExprNode>(std::move(right)), AssignExpr::Op::op_enum}; \
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
        return std::move(left);
    }

    template<typename Iter>
    static ExprNode parseConditional(Iter& it, Iter end) {
        printToken(it, end, "ExprParser::parseConditional");
        ExprNode cond = parseLogicalOr(it, end);
        if (it != end && isToken<Tokenization::Question>(it->token)) {
            ++it;
            ExprNode thenExpr = parseAssignment(it, end);
            if (it == end || !isToken<Tokenization::Colon>(it->token))
                throwSyntaxError(currentPosition(it, end), "Expected ':'");
            ++it;
            ExprNode elseExpr = parseAssignment(it, end);
            return ConditionalExpr{Util::makeBoxed<ExprNode>(std::move(cond)),
                                   Util::makeBoxed<ExprNode>(std::move(thenExpr)),
                                   Util::makeBoxed<ExprNode>(std::move(elseExpr))};
        }
        return std::move(cond);
    }

    // Уровни приоритета (следуя стандарту C)
    template<typename Iter>
    static ExprNode parseLogicalOr(Iter& it, Iter end) {
        static const std::vector<std::pair<Tokenization::TokenVariant, BinaryOp::Op>> ops = {{Tokenization::TokenVariant{Tokenization::OrOr{}}, BinaryOp::Op::LogicalOr}};
        auto next = [&](Iter& i, Iter e) -> ExprNode { return parseLogicalAnd(i, e); };
        return parseBinary(it, end, next, ops);
    }
    template<typename Iter>
    static ExprNode parseLogicalAnd(Iter& it, Iter end) {
        static const std::vector<std::pair<Tokenization::TokenVariant, BinaryOp::Op>> ops = {{Tokenization::TokenVariant{Tokenization::AndAnd{}}, BinaryOp::Op::LogicalAnd}};
        auto next = [&](Iter& i, Iter e) -> ExprNode { return parseBitOr(i, e); };
        return parseBinary(it, end, next, ops);
    }
    template<typename Iter>
    static ExprNode parseBitOr(Iter& it, Iter end) {
        static const std::vector<std::pair<Tokenization::TokenVariant, BinaryOp::Op>> ops = {{Tokenization::TokenVariant{Tokenization::Or{}}, BinaryOp::Op::BitOr}};
        auto next = [&](Iter& i, Iter e) -> ExprNode { return parseBitXor(i, e); };
        return parseBinary(it, end, next, ops);
    }
    template<typename Iter>
    static ExprNode parseBitXor(Iter& it, Iter end) {
        static const std::vector<std::pair<Tokenization::TokenVariant, BinaryOp::Op>> ops = {{Tokenization::TokenVariant{Tokenization::Xor{}}, BinaryOp::Op::BitXor}};
        auto next = [&](Iter& i, Iter e) -> ExprNode { return parseBitAnd(i, e); };
        return parseBinary(it, end, next, ops);
    }
    template<typename Iter>
    static ExprNode parseBitAnd(Iter& it, Iter end) {
        static const std::vector<std::pair<Tokenization::TokenVariant, BinaryOp::Op>> ops = {{Tokenization::TokenVariant{Tokenization::And{}}, BinaryOp::Op::BitAnd}};
        auto next = [&](Iter& i, Iter e) -> ExprNode { return parseEquality(i, e); };
        return parseBinary(it, end, next, ops);
    }
    template<typename Iter>
    static ExprNode parseEquality(Iter& it, Iter end) {
        static const std::vector<std::pair<Tokenization::TokenVariant, BinaryOp::Op>> ops = {
            {Tokenization::TokenVariant{Tokenization::Equal{}}, BinaryOp::Op::Equal},
            {Tokenization::TokenVariant{Tokenization::NotEqual{}}, BinaryOp::Op::NotEqual}
        };
        auto next = [&](Iter& i, Iter e) -> ExprNode { return parseRelational(i, e); };
        return parseBinary(it, end, next, ops);
    }

    template<typename Iter>
    static ExprNode parseRelational(Iter& it, Iter end) {
        static const std::vector<std::pair<Tokenization::TokenVariant, BinaryOp::Op>> ops = {
            {Tokenization::TokenVariant{Tokenization::Less{}}, BinaryOp::Op::Less},
            {Tokenization::TokenVariant{Tokenization::LessEqual{}}, BinaryOp::Op::LessEqual},
            {Tokenization::TokenVariant{Tokenization::Greater{}}, BinaryOp::Op::Greater},
            {Tokenization::TokenVariant{Tokenization::GreaterEqual{}}, BinaryOp::Op::GreaterEqual}
        };
        auto next = [&](Iter& i, Iter e) -> ExprNode { return parseShift(i, e); };
        return parseBinary(it, end, next, ops);
    }

    template<typename Iter>
    static ExprNode parseShift(Iter& it, Iter end) {
        static const std::vector<std::pair<Tokenization::TokenVariant, BinaryOp::Op>> ops = {
            {Tokenization::TokenVariant{Tokenization::ShiftLeft{}}, BinaryOp::Op::ShiftLeft},
            {Tokenization::TokenVariant{Tokenization::ShiftRight{}}, BinaryOp::Op::ShiftRight}
        };
        auto next = [&](Iter& i, Iter e) -> ExprNode { return parseAdditive(i, e); };
        return parseBinary(it, end, next, ops);
    }

    template<typename Iter>
    static ExprNode parseAdditive(Iter& it, Iter end) {
        static const std::vector<std::pair<Tokenization::TokenVariant, BinaryOp::Op>> ops = {
            {Tokenization::TokenVariant{Tokenization::Plus{}}, BinaryOp::Op::Add},
            {Tokenization::TokenVariant{Tokenization::Minus{}}, BinaryOp::Op::Sub}
        };
        auto next = [&](Iter& i, Iter e) -> ExprNode { return parseMultiplicative(i, e); };
        return parseBinary(it, end, next, ops);
    }

    template<typename Iter>
    static ExprNode parseMultiplicative(Iter& it, Iter end) {
        static const std::vector<std::pair<Tokenization::TokenVariant, BinaryOp::Op>> ops = {
            {Tokenization::TokenVariant{Tokenization::Star{}}, BinaryOp::Op::Mul},
            {Tokenization::TokenVariant{Tokenization::Slash{}}, BinaryOp::Op::Div},
            {Tokenization::TokenVariant{Tokenization::Percent{}}, BinaryOp::Op::Rem}
        };
        auto next = [&](Iter& i, Iter e) -> ExprNode { return parseUnary(i, e); };
        return parseBinary(it, end, next, ops);
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
        printToken(it, end, "StmtParser::parseBlock");
        std::cerr << "parseBlock: entered" << std::endl;
        
        if (it == end || !isToken<Tokenization::LeftBrace>(it->token))
            throwSyntaxError(currentPosition(it, end), "Expected '{'");
        ++it;
        std::vector<Util::Boxed<StmtNode>> stmts;
        while (it != end && !isToken<Tokenization::RightBrace>(it->token)) {
            stmts.push_back(Util::makeBoxed<StmtNode>(parseStatement(it, end)));
        }
        if (it == end) throwSyntaxError(Util::Position(), "Expected '}'");
        ++it;
        
        std::cerr << "parseBlock: returning" << std::endl;
        return BlockStmt{std::move(stmts)};
    }

private:
    template<typename Iter>
    static StmtNode parseStatement(Iter& it, Iter end) {
        printToken(it, end, "StmtParser::parseStatement");
        if (it == end) throwSyntaxError(Util::Position(), "Expected statement");

        return std::visit(Util::overloaded{
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
                std::optional<Util::Boxed<StmtNode>> elseStmt;
                if (it != end && isToken<Tokenization::Else>(it->token)) {
                    ++it;
                    elseStmt = Util::makeBoxed<StmtNode>(parseStatement(it, end));
                }
                return IfStmt{Util::makeBoxed<ExprNode>(std::move(cond)),
                             Util::makeBoxed<StmtNode>(std::move(thenStmt)),
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
                return WhileStmt{Util::makeBoxed<ExprNode>(std::move(cond)), Util::makeBoxed<StmtNode>(std::move(body))};
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
                return DoWhileStmt{Util::makeBoxed<StmtNode>(std::move(body)), Util::makeBoxed<ExprNode>(std::move(cond))};
            },
            [&](const Tokenization::For&) -> StmtNode {
                ++it;
                if (it == end || !isToken<Tokenization::LeftParenthesis>(it->token))
                    throwSyntaxError(currentPosition(it, end), "Expected '(' after for");
                ++it;
                std::optional<Util::Boxed<StmtNode>> init;
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
                        std::optional<Util::Boxed<ExprNode>> initializer;
                        if (it != end && isToken<Tokenization::Assign>(it->token)) {
                            ++it;
                            initializer = Util::makeBoxed<ExprNode>(ExprParser::parse(it, end));
                        }
                        init = Util::makeBoxed<StmtNode>(VarDeclStmt{std::move(type), name, std::move(initializer)});
                    } else {
                        // выражение
                        ExprNode expr = ExprParser::parse(it, end);
                        init = Util::makeBoxed<StmtNode>(ExprStmt{Util::makeBoxed<ExprNode>(std::move(expr))});
                    }
                }
                if (it == end || !isToken<Tokenization::Semicolon>(it->token))
                    throwSyntaxError(currentPosition(it, end), "Expected ';' after for init");
                ++it;
                std::optional<Util::Boxed<ExprNode>> condition;
                if (!isToken<Tokenization::Semicolon>(it->token)) {
                    condition = Util::makeBoxed<ExprNode>(ExprParser::parse(it, end));
                }
                if (it == end || !isToken<Tokenization::Semicolon>(it->token))
                    throwSyntaxError(currentPosition(it, end), "Expected ';' after for condition");
                ++it;
                std::optional<Util::Boxed<ExprNode>> increment;
                if (!isToken<Tokenization::RightParenthesis>(it->token)) {
                    increment = Util::makeBoxed<ExprNode>(ExprParser::parse(it, end));
                }
                if (it == end || !isToken<Tokenization::RightParenthesis>(it->token))
                    throwSyntaxError(currentPosition(it, end), "Expected ')' after for clauses");
                ++it;
                StmtNode body = parseStatement(it, end);
                return ForStmt{std::move(init), std::move(condition), std::move(increment), Util::makeBoxed<StmtNode>(std::move(body))};
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
                return SwitchStmt{Util::makeBoxed<ExprNode>(std::move(control)), Util::makeBoxed<StmtNode>(std::move(body))};
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
                std::vector<Util::Boxed<ExprNode>> values;
                if (it != end && !isToken<Tokenization::Semicolon>(it->token)) {
                    do {
                        values.push_back(Util::makeBoxed<ExprNode>(ExprParser::parse(it, end)));
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
                    return LabelStmt{id.name, Util::makeBoxed<StmtNode>(std::move(stmt))};
                }
                it = saved;
                ExprNode expr = ExprParser::parse(it, end);
                std::cerr << "parseStatement: after parse, token = " << (it != end ? it->token : Tokenization::TokenVariant{}) << std::endl;
                if (it == end || !isToken<Tokenization::Semicolon>(it->token))
                    throwSyntaxError(currentPosition(it, end), "Expected ';' after expression statement");
                ++it;
                return ExprStmt{Util::makeBoxed<ExprNode>(std::move(expr))};
            },
            [&](const Tokenization::LeftBrace&) -> StmtNode {
                BlockStmt block = parseBlock(it, end);
                return std::move(block);
            },
            [&](const Tokenization::Struct&) -> StmtNode {
                ++it; // пропускаем 'struct'
                if (it == end || !isToken<Tokenization::Identifier>(it->token))
                    throwSyntaxError(currentPosition(it, end), "Expected struct name");
                std::string structName = std::get<Tokenization::Identifier>(it->token).name;
                ++it;
                TypeNode type = NamedType{structName};
                if (it == end || !isToken<Tokenization::Identifier>(it->token))
                    throwSyntaxError(currentPosition(it, end), "Expected variable name");
                std::string varName = std::get<Tokenization::Identifier>(it->token).name;
                ++it;
                std::optional<Util::Boxed<ExprNode>> initializer;
                if (it != end && isToken<Tokenization::Assign>(it->token)) {
                    ++it;
                    if (isToken<Tokenization::LeftBrace>(it->token)) {
                        // Составной инициализатор: { expr, expr, ... }
                        ++it;
                        std::vector<Util::Boxed<ExprNode>> initValues;
                        while (it != end && !isToken<Tokenization::RightBrace>(it->token)) {
                            initValues.push_back(Util::makeBoxed<ExprNode>(ExprParser::parse(it, end)));
                            if (it != end && isToken<Tokenization::Comma>(it->token)) ++it;
                            else break;
                        }
                        if (it == end || !isToken<Tokenization::RightBrace>(it->token))
                            throwSyntaxError(currentPosition(it, end), "Expected '}' after initializer list");
                        ++it;
                        initializer = Util::makeBoxed<ExprNode>(InitListExpr{std::move(initValues)});
                    } else {
                        initializer = Util::makeBoxed<ExprNode>(ExprParser::parse(it, end));
                    }
                }
                if (it == end || !isToken<Tokenization::Semicolon>(it->token))
                    throwSyntaxError(currentPosition(it, end), "Expected ';' after variable declaration");
                ++it;
                return VarDeclStmt{std::move(type), varName, std::move(initializer)};
            },
            [&](const Tokenization::Enum&) -> StmtNode {
                ++it; // пропускаем 'enum'
                if (it == end || !isToken<Tokenization::Identifier>(it->token))
                    throwSyntaxError(currentPosition(it, end), "Expected enum name");
                std::string enumName = std::get<Tokenization::Identifier>(it->token).name;
                ++it;
                TypeNode type = NamedType{enumName};
                if (it == end || !isToken<Tokenization::Identifier>(it->token))
                    throwSyntaxError(currentPosition(it, end), "Expected variable name");
                std::string varName = std::get<Tokenization::Identifier>(it->token).name;
                ++it;
                std::optional<Util::Boxed<ExprNode>> initializer;
                if (it != end && isToken<Tokenization::Assign>(it->token)) {
                    ++it;
                    // Для enum инициализатор – просто выражение (например, GREEN)
                    initializer = Util::makeBoxed<ExprNode>(ExprParser::parse(it, end));
                }
                if (it == end || !isToken<Tokenization::Semicolon>(it->token))
                    throwSyntaxError(currentPosition(it, end), "Expected ';' after variable declaration");
                ++it;
                return VarDeclStmt{std::move(type), varName, std::move(initializer)};
            },
            [&](auto) -> StmtNode {
                // объявление переменной в блоке (не for)
                TypeNode type = TypeParser::parse(it, end, true, true);
                if (it == end || !isToken<Tokenization::Identifier>(it->token))
                    throwSyntaxError(currentPosition(it, end), "Expected variable name");
                std::string name = std::get<Tokenization::Identifier>(it->token).name;
                ++it;
                std::optional<Util::Boxed<ExprNode>> initializer;
                if (it != end && isToken<Tokenization::Assign>(it->token)) {
                    ++it;
                    if (isToken<Tokenization::LeftBrace>(it->token)) {
                        ++it;
                        std::vector<Util::Boxed<ExprNode>> initValues;
                        while (it != end && !isToken<Tokenization::RightBrace>(it->token)) {
                            initValues.push_back(Util::makeBoxed<ExprNode>(ExprParser::parse(it, end)));
                            if (it != end && isToken<Tokenization::Comma>(it->token)) ++it;
                        }
                        if (it == end || !isToken<Tokenization::RightBrace>(it->token))
                            throwSyntaxError(currentPosition(it, end), "Expected '}'");
                        ++it;
                        initializer = Util::makeBoxed<ExprNode>(InitListExpr{std::move(initValues)});
                    } else {
                        initializer = Util::makeBoxed<ExprNode>(ExprParser::parse(it, end));
                    }
                }
                if (it == end || !isToken<Tokenization::Semicolon>(it->token))
                    throwSyntaxError(currentPosition(it, end), "Expected ';' after variable declaration");
                ++it;
                return VarDeclStmt{std::move(type), name, std::move(initializer)};
            }
        }, it->token);
    }
};

// ---------- Парсер определений верхнего уровня ----------
class DefParser {
public:
    template<typename Iter>
    static DefNode parse(Iter& it, Iter end) {
        printToken(it, end, "DefParser::parse");
        if (it == end) throwSyntaxError(Util::Position(), "Expected definition");

        std::cerr << "DefParser::parse: parsing definition at token: "  << std::endl;
        return std::visit(Util::overloaded{
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
                if (it == end) throwSyntaxError(Util::Position(), "Expected '}'");
                ++it;
                if (it == end || !isToken<Tokenization::Semicolon>(it->token))
                    throwSyntaxError(currentPosition(it, end), "Expected ';' after enum definition");
                ++it; // пропускаем ';'
                return StructDef{std::move(name), std::move(fields)};
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
                if (it == end) throwSyntaxError(Util::Position(), "Expected '}'");
                ++it;
                if (it == end || !isToken<Tokenization::Semicolon>(it->token))
                    throwSyntaxError(currentPosition(it, end), "Expected ';' after enum definition");
                ++it; // пропускаем ';'
                return EnumDef{std::move(name), std::move(enumerators)};
            },
            [&](auto) -> DefNode {
                throwSyntaxError(it->position, "Expected function, global variable, struct or enum definition");
            }
        }, it->token);
    }

private:
    template<typename Iter, typename BaseType>
    static DefNode parseFunctionOrGlobal(Iter& it, Iter end, BaseType baseType) {
        // Сначала читаем первый тип (может быть со звёздочками и скобками)
        TypeNode firstType = TypeParser::parse(it, end, true, true);
        std::vector<TypeNode> returnTypes;
        returnTypes.push_back(std::move(firstType));

        // Если после первого типа идёт запятая — значит, это список возвращаемых типов (только для функции)
        if (it != end && isToken<Tokenization::Comma>(it->token)) {
            ++it; // пропускаем ','
            do {
                TypeNode nextType = TypeParser::parse(it, end, true, true);
                returnTypes.push_back(std::move(nextType));
                if (it == end || !isToken<Tokenization::Comma>(it->token)) break;
                ++it;
            } while (true);
        }

        // Теперь должно быть имя (идентификатор)
        if (it == end || !isToken<Tokenization::Identifier>(it->token))
            throwSyntaxError(currentPosition(it, end), "Expected function or variable name");
        std::string name = std::get<Tokenization::Identifier>(it->token).name;
        ++it;

        // Если следующий токен '(', то это функция
        if (it != end && isToken<Tokenization::LeftParenthesis>(it->token)) {
            // === Функция ===
            ++it; // '('
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

            // Тело функции (блок или ';')
            std::optional<BlockStmt> body;
            if (it != end && isToken<Tokenization::Semicolon>(it->token)) {
                ++it; // объявление без тела
            } else if (it != end && isToken<Tokenization::LeftBrace>(it->token)) {
                body = StmtParser::parseBlock(it, end);
            } else {
                throwSyntaxError(currentPosition(it, end), "Expected function body or ';'");
            }

            return FunctionDef{std::move(name), std::move(params), std::move(returnTypes), variadic, std::move(body)};
        }
        else {
            // === Глобальная переменная ===
            if (returnTypes.size() > 1) {
                throwSyntaxError(currentPosition(it, end), "Global variable cannot have multiple return types");
            }
            std::optional<ExprNode> initializer;
            if (it != end && isToken<Tokenization::Assign>(it->token)) {
                ++it;
                initializer = ExprParser::parse(it, end);
            }
            if (it == end || !isToken<Tokenization::Semicolon>(it->token))
                throwSyntaxError(currentPosition(it, end), "Expected ';' after global variable");
            ++it;
            return GlobalVarDef{std::move(returnTypes[0]), std::move(name), std::move(initializer)};
        }
    }
};

} // namespace detail

ParsingInfo Parser::parse(const std::vector<Tokenization::TokenInfo>& tokens) {
    auto it = tokens.begin();
    auto end = tokens.end();
    TranslationUnit tu;
    std::unordered_map<const void*, Util::Position> positions;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;

    int i = 0;
    while (it != end) {
        try {
            auto pos = it->position;
            std::cerr << "Main parse: token = " << it->token << " at " << pos.toString() << std::endl;
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