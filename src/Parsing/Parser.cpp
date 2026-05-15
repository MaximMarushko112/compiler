#include "../../include/Parsing/Parser.hpp"
#include <cassert>
#include <iostream>
#include <optional>
#include <memory>

namespace Parsing {
namespace detail {

using TokenIter = std::vector<Tokenization::TokenInfo>::const_iterator;

auto printToken = [](TokenIter it, TokenIter end, const char* func) {
    if (it == end)
        std::cerr << func << ": END OF TOKENS\n";
    else
        std::cerr << func << ": token = " << it->token << " at " << it->position.toString() << "\n";
};

// ---------- Утилиты ----------
[[noreturn]] void throwSyntaxError(const Util::Position& pos, const std::string& message) {
    throw std::runtime_error("Syntax error at " + pos.toString() + ": " + message);
}

const Util::Position& currentPosition(TokenIter it, TokenIter end) {
    if (it == end) throw std::runtime_error("Unexpected end of tokens");
    return it->position;
}

void expectToken(TokenIter& it, TokenIter end, auto tokenType, const std::string& expected) {
    if (it == end) throwSyntaxError(it[-1].position, "Unexpected end, expected " + expected);
    if (!std::holds_alternative<decltype(tokenType)>(it->token))
        throwSyntaxError(it->position, "Expected " + expected);
    ++it;
}

template<typename T>
bool isToken(const Tokenization::TokenVariant& tv) {
    return std::holds_alternative<T>(tv);
}

template<typename T>
auto getLiteralValue(const Tokenization::TokenVariant& tv) {
    return std::get<T>(tv).value;
}

// ---------- Парсер типов ----------
class TypeParser {
public:
    static std::unique_ptr<Type> parse(TokenIter& it, TokenIter end) {
        return parsePrimary(it, end);
    }

private:
    static std::unique_ptr<Type> parsePrimary(TokenIter& it, TokenIter end) {
        printToken(it, end, "TypeParser::parsePrimary");
        if (it == end) throwSyntaxError(Util::Position(), "Expected type");

        if (isToken<Tokenization::Int>(it->token)) {
            ++it; return std::make_unique<IntType>();
        }
        if (isToken<Tokenization::Unsigned>(it->token)) {
            ++it; return std::make_unique<UnsignedType>();
        }
        if (isToken<Tokenization::Float>(it->token)) {
            ++it; return std::make_unique<FloatType>();
        }
        if (isToken<Tokenization::Bool>(it->token)) {
            ++it; return std::make_unique<BoolType>();
        }
        if (isToken<Tokenization::String>(it->token)) {
            ++it; return std::make_unique<StringType>();
        }
        if (isToken<Tokenization::Void>(it->token)) {
            ++it; return std::make_unique<VoidType>();
        }
        if (isToken<Tokenization::Struct>(it->token)) {
            ++it;
            if (it == end || !isToken<Tokenization::Identifier>(it->token))
                throwSyntaxError(currentPosition(it, end), "Expected struct name");
            std::string name = std::get<Tokenization::Identifier>(it->token).name;
            ++it;
            return std::make_unique<NamedType>(name);
        }
        if (isToken<Tokenization::Enum>(it->token)) {
            ++it;
            if (it == end || !isToken<Tokenization::Identifier>(it->token))
                throwSyntaxError(currentPosition(it, end), "Expected enum name");
            std::string name = std::get<Tokenization::Identifier>(it->token).name;
            ++it;
            return std::make_unique<NamedType>(name);
        }
        if (isToken<Tokenization::LeftParenthesis>(it->token)) {
            ++it;
            auto inner = parse(it, end);
            if (it == end || !isToken<Tokenization::RightParenthesis>(it->token))
                throwSyntaxError(currentPosition(it, end), "Expected ')'");
            ++it;
            return inner;
        }
        if (isToken<Tokenization::Identifier>(it->token)) {
            std::string name = std::get<Tokenization::Identifier>(it->token).name;
            ++it;
            return std::make_unique<NamedType>(name);
        }
        throwSyntaxError(currentPosition(it, end), "Expected type");
    }

public:
    static std::unique_ptr<Type> parse(TokenIter& it, TokenIter end, bool allowPointer, bool allowArray) {
        printToken(it, end, "TypeParser::parse(allow)");
        auto type = parsePrimary(it, end);
        while (it != end) {
            if (allowPointer && isToken<Tokenization::Star>(it->token)) {
                ++it;
                type = std::make_unique<PointerType>(std::move(type));
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
                type = std::make_unique<ArrayType>(std::move(type), size);
            }
            else break;
        }
        return type;
    }
};

// ---------- Парсер выражений ----------
class ExprParser {
public:
    static std::unique_ptr<Expr> parse(TokenIter& it, TokenIter end) {
        return parseAssignment(it, end);
    }

private:
    static std::unique_ptr<Expr> parsePrimary(TokenIter& it, TokenIter end) {
        printToken(it, end, "ExprParser::parsePrimary");
        if (it == end) throwSyntaxError(Util::Position(), "Expected expression");

        if (auto* lit = std::get_if<Tokenization::IntLiteral>(&it->token)) {
            ++it;
            return std::make_unique<IntLiteral>(lit->value);
        }
        if (auto* lit = std::get_if<Tokenization::FloatLiteral>(&it->token)) {
            ++it;
            return std::make_unique<FloatLiteral>(lit->value);
        }
        if (auto* lit = std::get_if<Tokenization::StringLiteral>(&it->token)) {
            ++it;
            return std::make_unique<StringLiteral>(lit->value);
        }
        if (isToken<Tokenization::True>(it->token)) {
            ++it;
            return std::make_unique<BoolLiteral>(true);
        }
        if (isToken<Tokenization::False>(it->token)) {
            ++it;
            return std::make_unique<BoolLiteral>(false);
        }
        if (auto* id = std::get_if<Tokenization::Identifier>(&it->token)) {
            ++it;
            return std::make_unique<VariableExpr>(id->name);
        }
        if (isToken<Tokenization::LeftParenthesis>(it->token)) {
            ++it;
            auto expr = parse(it, end);
            if (it == end || !isToken<Tokenization::RightParenthesis>(it->token))
                throwSyntaxError(currentPosition(it, end), "Expected ')'");
            ++it;
            return expr;
        }
        if (isToken<Tokenization::Sizeof>(it->token)) {
            ++it;
            if (it == end) throwSyntaxError(Util::Position(), "Expected expression or type after sizeof");
            if (isToken<Tokenization::LeftParenthesis>(it->token)) {
                ++it;
                auto type = TypeParser::parse(it, end, true, true);
                if (it == end || !isToken<Tokenization::RightParenthesis>(it->token))
                    throwSyntaxError(currentPosition(it, end), "Expected ')'");
                ++it;
                return std::make_unique<SizeofExpr>(std::move(type));
            } else {
                auto expr = parseUnary(it, end);
                return std::make_unique<SizeofExpr>(std::move(expr));
            }
        }
        throwSyntaxError(currentPosition(it, end), "Expected expression");
    }

    static std::unique_ptr<Expr> parseUnary(TokenIter& it, TokenIter end) {
        printToken(it, end, "ExprParser::parseUnary");
        if (it == end) throwSyntaxError(Util::Position(), "Expected unary expression");

        if (isToken<Tokenization::Not>(it->token)) {
            ++it;
            auto operand = parseUnary(it, end);
            return std::make_unique<UnaryOp>(UnaryOp::Op::Not, std::move(operand));
        }
        if (isToken<Tokenization::Minus>(it->token)) {
            ++it;
            auto operand = parseUnary(it, end);
            return std::make_unique<UnaryOp>(UnaryOp::Op::Minus, std::move(operand));
        }
        if (isToken<Tokenization::Plus>(it->token)) {
            ++it;
            auto operand = parseUnary(it, end);
            return std::make_unique<UnaryOp>(UnaryOp::Op::Plus, std::move(operand));
        }
        if (isToken<Tokenization::And>(it->token)) {
            ++it;
            auto operand = parseUnary(it, end);
            return std::make_unique<UnaryOp>(UnaryOp::Op::AddressOf, std::move(operand));
        }
        if (isToken<Tokenization::Star>(it->token)) {
            ++it;
            auto operand = parseUnary(it, end);
            return std::make_unique<UnaryOp>(UnaryOp::Op::Dereference, std::move(operand));
        }
        return parsePostfix(it, end);
    }

    static std::unique_ptr<Expr> parsePostfix(TokenIter& it, TokenIter end) {
        printToken(it, end, "ExprParser::parsePostfix");
        auto left = parsePrimary(it, end);
        while (it != end) {
            if (isToken<Tokenization::LeftBracket>(it->token)) {
                ++it;
                auto index = parse(it, end);
                if (it == end || !isToken<Tokenization::RightBracket>(it->token))
                    throwSyntaxError(currentPosition(it, end), "Expected ']'");
                ++it;
                left = std::make_unique<IndexExpr>(std::move(left), std::move(index));
            }
            else if (isToken<Tokenization::LeftParenthesis>(it->token)) {
                ++it;
                std::vector<std::unique_ptr<Expr>> args;
                if (!isToken<Tokenization::RightParenthesis>(it->token)) {
                    do {
                        args.push_back(parse(it, end));
                        if (it != end && isToken<Tokenization::Comma>(it->token)) ++it;
                        else break;
                    } while (true);
                }
                if (it == end || !isToken<Tokenization::RightParenthesis>(it->token))
                    throwSyntaxError(currentPosition(it, end), "Expected ')'");
                ++it;
                left = std::make_unique<CallExpr>(std::move(left), std::move(args));
            }
            else if (isToken<Tokenization::Dot>(it->token)) {
                ++it;
                if (it == end || !isToken<Tokenization::Identifier>(it->token))
                    throwSyntaxError(currentPosition(it, end), "Expected field name");
                std::string field = std::get<Tokenization::Identifier>(it->token).name;
                ++it;
                left = std::make_unique<FieldAccessExpr>(std::move(left), field, false);
            }
            else if (isToken<Tokenization::Arrow>(it->token)) {
                ++it;
                if (it == end || !isToken<Tokenization::Identifier>(it->token))
                    throwSyntaxError(currentPosition(it, end), "Expected field name");
                std::string field = std::get<Tokenization::Identifier>(it->token).name;
                ++it;
                left = std::make_unique<FieldAccessExpr>(std::move(left), field, true);
            }
            else break;
        }
        return left;
    }

    template<typename NextParser>
    static std::unique_ptr<Expr> parseBinary(TokenIter& it, TokenIter end,
                                             NextParser&& nextParser,
                                             const std::vector<std::pair<Tokenization::TokenVariant, BinaryOp::Op>>& ops) {
        auto left = nextParser(it, end);
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
            auto right = nextParser(it, end);
            left = std::make_unique<BinaryOp>(op, std::move(left), std::move(right));
        }
        return left;
    }

public:
    static std::unique_ptr<Expr> parseAssignment(TokenIter& it, TokenIter end) {
        printToken(it, end, "ExprParser::parseAssignment");
        auto saved = it;
        std::vector<std::unique_ptr<Expr>> lefts;
        bool isMultiAssign = false;
        try {
            while (true) {
                lefts.push_back(parsePostfix(it, end));
                if (it == end || !isToken<Tokenization::Comma>(it->token)) break;
                ++it;
            }
            if (it != end && isToken<Tokenization::Assign>(it->token)) {
                isMultiAssign = true;
                ++it;
                std::vector<std::unique_ptr<Expr>> rights;
                while (true) {
                    rights.push_back(parseAssignment(it, end));
                    if (it == end || !isToken<Tokenization::Comma>(it->token)) break;
                    ++it;
                }
                return std::make_unique<MultiAssignExpr>(std::move(lefts), std::move(rights));
            }
        } catch (...) {
            it = saved;
        }
        if (!isMultiAssign) it = saved;

        auto left = parseConditional(it, end);
        if (it != end) {
            if (isToken<Tokenization::Assign>(it->token)) {
                ++it;
                auto right = parseAssignment(it, end);
                return std::make_unique<AssignExpr>(std::move(left), std::move(right), AssignExpr::Op::Assign);
            }
            #define COMPOUND_ASSIGN(op_token, op_enum) \
            if (isToken<Tokenization::op_token>(it->token)) { \
                ++it; auto right = parseAssignment(it, end); \
                return std::make_unique<AssignExpr>(std::move(left), std::move(right), AssignExpr::Op::op_enum); \
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

    static std::unique_ptr<Expr> parseConditional(TokenIter& it, TokenIter end) {
        auto cond = parseLogicalOr(it, end);
        if (it != end && isToken<Tokenization::Question>(it->token)) {
            ++it;
            auto thenExpr = parseAssignment(it, end);
            if (it == end || !isToken<Tokenization::Colon>(it->token))
                throwSyntaxError(currentPosition(it, end), "Expected ':'");
            ++it;
            auto elseExpr = parseAssignment(it, end);
            return std::make_unique<ConditionalExpr>(std::move(cond), std::move(thenExpr), std::move(elseExpr));
        }
        return cond;
    }

    static std::unique_ptr<Expr> parseLogicalOr(TokenIter& it, TokenIter end) {
        static const std::vector<std::pair<Tokenization::TokenVariant, BinaryOp::Op>> ops = {
            {Tokenization::TokenVariant{Tokenization::OrOr{}}, BinaryOp::Op::LogicalOr}
        };
        auto next = [&](TokenIter& i, TokenIter e) { return parseLogicalAnd(i, e); };
        return parseBinary(it, end, next, ops);
    }

    static std::unique_ptr<Expr> parseLogicalAnd(TokenIter& it, TokenIter end) {
        static const std::vector<std::pair<Tokenization::TokenVariant, BinaryOp::Op>> ops = {
            {Tokenization::TokenVariant{Tokenization::AndAnd{}}, BinaryOp::Op::LogicalAnd}
        };
        auto next = [&](TokenIter& i, TokenIter e) { return parseBitOr(i, e); };
        return parseBinary(it, end, next, ops);
    }

    static std::unique_ptr<Expr> parseBitOr(TokenIter& it, TokenIter end) {
        static const std::vector<std::pair<Tokenization::TokenVariant, BinaryOp::Op>> ops = {
            {Tokenization::TokenVariant{Tokenization::Or{}}, BinaryOp::Op::BitOr}
        };
        auto next = [&](TokenIter& i, TokenIter e) { return parseBitXor(i, e); };
        return parseBinary(it, end, next, ops);
    }

    static std::unique_ptr<Expr> parseBitXor(TokenIter& it, TokenIter end) {
        static const std::vector<std::pair<Tokenization::TokenVariant, BinaryOp::Op>> ops = {
            {Tokenization::TokenVariant{Tokenization::Xor{}}, BinaryOp::Op::BitXor}
        };
        auto next = [&](TokenIter& i, TokenIter e) { return parseBitAnd(i, e); };
        return parseBinary(it, end, next, ops);
    }

    static std::unique_ptr<Expr> parseBitAnd(TokenIter& it, TokenIter end) {
        static const std::vector<std::pair<Tokenization::TokenVariant, BinaryOp::Op>> ops = {
            {Tokenization::TokenVariant{Tokenization::And{}}, BinaryOp::Op::BitAnd}
        };
        auto next = [&](TokenIter& i, TokenIter e) { return parseEquality(i, e); };
        return parseBinary(it, end, next, ops);
    }

    static std::unique_ptr<Expr> parseEquality(TokenIter& it, TokenIter end) {
        static const std::vector<std::pair<Tokenization::TokenVariant, BinaryOp::Op>> ops = {
            {Tokenization::TokenVariant{Tokenization::Equal{}}, BinaryOp::Op::Equal},
            {Tokenization::TokenVariant{Tokenization::NotEqual{}}, BinaryOp::Op::NotEqual}
        };
        auto next = [&](TokenIter& i, TokenIter e) { return parseRelational(i, e); };
        return parseBinary(it, end, next, ops);
    }

    static std::unique_ptr<Expr> parseRelational(TokenIter& it, TokenIter end) {
        static const std::vector<std::pair<Tokenization::TokenVariant, BinaryOp::Op>> ops = {
            {Tokenization::TokenVariant{Tokenization::Less{}}, BinaryOp::Op::Less},
            {Tokenization::TokenVariant{Tokenization::LessEqual{}}, BinaryOp::Op::LessEqual},
            {Tokenization::TokenVariant{Tokenization::Greater{}}, BinaryOp::Op::Greater},
            {Tokenization::TokenVariant{Tokenization::GreaterEqual{}}, BinaryOp::Op::GreaterEqual}
        };
        auto next = [&](TokenIter& i, TokenIter e) { return parseShift(i, e); };
        return parseBinary(it, end, next, ops);
    }

    static std::unique_ptr<Expr> parseShift(TokenIter& it, TokenIter end) {
        static const std::vector<std::pair<Tokenization::TokenVariant, BinaryOp::Op>> ops = {
            {Tokenization::TokenVariant{Tokenization::ShiftLeft{}}, BinaryOp::Op::ShiftLeft},
            {Tokenization::TokenVariant{Tokenization::ShiftRight{}}, BinaryOp::Op::ShiftRight}
        };
        auto next = [&](TokenIter& i, TokenIter e) { return parseAdditive(i, e); };
        return parseBinary(it, end, next, ops);
    }

    static std::unique_ptr<Expr> parseAdditive(TokenIter& it, TokenIter end) {
        static const std::vector<std::pair<Tokenization::TokenVariant, BinaryOp::Op>> ops = {
            {Tokenization::TokenVariant{Tokenization::Plus{}}, BinaryOp::Op::Add},
            {Tokenization::TokenVariant{Tokenization::Minus{}}, BinaryOp::Op::Sub}
        };
        auto next = [&](TokenIter& i, TokenIter e) { return parseMultiplicative(i, e); };
        return parseBinary(it, end, next, ops);
    }

    static std::unique_ptr<Expr> parseMultiplicative(TokenIter& it, TokenIter end) {
        static const std::vector<std::pair<Tokenization::TokenVariant, BinaryOp::Op>> ops = {
            {Tokenization::TokenVariant{Tokenization::Star{}}, BinaryOp::Op::Mul},
            {Tokenization::TokenVariant{Tokenization::Slash{}}, BinaryOp::Op::Div},
            {Tokenization::TokenVariant{Tokenization::Percent{}}, BinaryOp::Op::Rem}
        };
        auto next = [&](TokenIter& i, TokenIter e) { return parseUnary(i, e); };
        return parseBinary(it, end, next, ops);
    }
};

// ---------- Парсер операторов ----------
class StmtParser {
public:
    static std::unique_ptr<Stmt> parse(TokenIter& it, TokenIter end) {
        return parseStatement(it, end);
    }

    static BlockStmt parseBlock(TokenIter& it, TokenIter end) {
        printToken(it, end, "StmtParser::parseBlock");
        if (it == end || !isToken<Tokenization::LeftBrace>(it->token))
            throwSyntaxError(currentPosition(it, end), "Expected '{'");
        ++it;
        std::vector<std::unique_ptr<Stmt>> stmts;
        while (it != end && !isToken<Tokenization::RightBrace>(it->token)) {
            stmts.push_back(parseStatement(it, end));
        }
        if (it == end) throwSyntaxError(Util::Position(), "Expected '}'");
        ++it;
        return BlockStmt(std::move(stmts));
    }

private:
    static std::unique_ptr<Stmt> parseStatement(TokenIter& it, TokenIter end) {
        printToken(it, end, "StmtParser::parseStatement");
        if (it == end) throwSyntaxError(Util::Position(), "Expected statement");

        if (isToken<Tokenization::If>(it->token)) return parseIfStatement(it, end);
        if (isToken<Tokenization::While>(it->token)) return parseWhileStatement(it, end);
        if (isToken<Tokenization::Do>(it->token)) return parseDoWhileStatement(it, end);
        if (isToken<Tokenization::For>(it->token)) return parseForStatement(it, end);
        if (isToken<Tokenization::Switch>(it->token)) return parseSwitchStatement(it, end);
        if (isToken<Tokenization::Break>(it->token)) return parseBreakStatement(it, end);
        if (isToken<Tokenization::Continue>(it->token)) return parseContinueStatement(it, end);
        if (isToken<Tokenization::Return>(it->token)) return parseReturnStatement(it, end);
        if (isToken<Tokenization::Goto>(it->token)) return parseGotoStatement(it, end);
        if (auto* id = std::get_if<Tokenization::Identifier>(&it->token))
            return parseLabelOrExpressionStatement(it, end, *id);
        if (isToken<Tokenization::LeftBrace>(it->token)) return parseBlockStatement(it, end);
        if (isToken<Tokenization::Struct>(it->token) || isToken<Tokenization::Enum>(it->token))
            return parseStructEnumDeclaration(it, end);
        return parseVarDeclaration(it, end);
    }

    static std::unique_ptr<Stmt> parseIfStatement(TokenIter& it, TokenIter end) {
        ++it;
        if (it == end || !isToken<Tokenization::LeftParenthesis>(it->token))
            throwSyntaxError(currentPosition(it, end), "Expected '(' after if");
        ++it;
        auto cond = ExprParser::parse(it, end);
        if (it == end || !isToken<Tokenization::RightParenthesis>(it->token))
            throwSyntaxError(currentPosition(it, end), "Expected ')'");
        ++it;
        auto thenStmt = parseStatement(it, end);
        std::optional<std::unique_ptr<Stmt>> elseStmt;
        if (it != end && isToken<Tokenization::Else>(it->token)) {
            ++it;
            elseStmt = parseStatement(it, end);
        }
        return std::make_unique<IfStmt>(std::move(cond), std::move(thenStmt), std::move(elseStmt));
    }

    static std::unique_ptr<Stmt> parseWhileStatement(TokenIter& it, TokenIter end) {
        ++it;
        if (it == end || !isToken<Tokenization::LeftParenthesis>(it->token))
            throwSyntaxError(currentPosition(it, end), "Expected '(' after while");
        ++it;
        auto cond = ExprParser::parse(it, end);
        if (it == end || !isToken<Tokenization::RightParenthesis>(it->token))
            throwSyntaxError(currentPosition(it, end), "Expected ')'");
        ++it;
        auto body = parseStatement(it, end);
        return std::make_unique<WhileStmt>(std::move(cond), std::move(body));
    }

    static std::unique_ptr<Stmt> parseDoWhileStatement(TokenIter& it, TokenIter end) {
        ++it;
        auto body = parseStatement(it, end);
        if (it == end || !isToken<Tokenization::While>(it->token))
            throwSyntaxError(currentPosition(it, end), "Expected 'while' after do body");
        ++it;
        if (it == end || !isToken<Tokenization::LeftParenthesis>(it->token))
            throwSyntaxError(currentPosition(it, end), "Expected '(' after while");
        ++it;
        auto cond = ExprParser::parse(it, end);
        if (it == end || !isToken<Tokenization::RightParenthesis>(it->token))
            throwSyntaxError(currentPosition(it, end), "Expected ')'");
        ++it;
        if (it == end || !isToken<Tokenization::Semicolon>(it->token))
            throwSyntaxError(currentPosition(it, end), "Expected ';' after do-while");
        ++it;
        return std::make_unique<DoWhileStmt>(std::move(body), std::move(cond));
    }

    static std::unique_ptr<Stmt> parseForStatement(TokenIter& it, TokenIter end) {
        ++it;
        if (it == end || !isToken<Tokenization::LeftParenthesis>(it->token))
            throwSyntaxError(currentPosition(it, end), "Expected '(' after for");
        ++it;
        std::optional<std::unique_ptr<Stmt>> init;
        if (!isToken<Tokenization::Semicolon>(it->token)) {
            if (isToken<Tokenization::Int>(it->token) || isToken<Tokenization::Unsigned>(it->token) ||
                isToken<Tokenization::Float>(it->token) || isToken<Tokenization::Bool>(it->token) ||
                isToken<Tokenization::String>(it->token) || isToken<Tokenization::Struct>(it->token) ||
                isToken<Tokenization::Enum>(it->token)) {
                auto type = TypeParser::parse(it, end, true, true);
                if (it == end || !isToken<Tokenization::Identifier>(it->token))
                    throwSyntaxError(currentPosition(it, end), "Expected variable name");
                std::string name = std::get<Tokenization::Identifier>(it->token).name;
                ++it;
                std::optional<std::unique_ptr<Expr>> initializer;
                if (it != end && isToken<Tokenization::Assign>(it->token)) {
                    ++it;
                    initializer = ExprParser::parse(it, end);
                }
                init = std::make_unique<VarDeclStmt>(std::move(type), name, std::move(initializer));
            } else {
                auto expr = ExprParser::parse(it, end);
                init = std::make_unique<ExprStmt>(std::move(expr));
            }
        }
        if (it == end || !isToken<Tokenization::Semicolon>(it->token))
            throwSyntaxError(currentPosition(it, end), "Expected ';' after for init");
        ++it;
        std::optional<std::unique_ptr<Expr>> condition;
        if (!isToken<Tokenization::Semicolon>(it->token)) {
            condition = ExprParser::parse(it, end);
        }
        if (it == end || !isToken<Tokenization::Semicolon>(it->token))
            throwSyntaxError(currentPosition(it, end), "Expected ';' after for condition");
        ++it;
        std::optional<std::unique_ptr<Expr>> increment;
        if (!isToken<Tokenization::RightParenthesis>(it->token)) {
            increment = ExprParser::parse(it, end);
        }
        if (it == end || !isToken<Tokenization::RightParenthesis>(it->token))
            throwSyntaxError(currentPosition(it, end), "Expected ')' after for clauses");
        ++it;
        auto body = parseStatement(it, end);
        return std::make_unique<ForStmt>(std::move(init), std::move(condition), std::move(increment), std::move(body));
    }

    static std::unique_ptr<Stmt> parseSwitchStatement(TokenIter& it, TokenIter end) {
        ++it;
        if (it == end || !isToken<Tokenization::LeftParenthesis>(it->token))
            throwSyntaxError(currentPosition(it, end), "Expected '(' after switch");
        ++it;
        auto control = ExprParser::parse(it, end);
        if (it == end || !isToken<Tokenization::RightParenthesis>(it->token))
            throwSyntaxError(currentPosition(it, end), "Expected ')'");
        ++it;
        auto body = parseStatement(it, end);
        return std::make_unique<SwitchStmt>(std::move(control), std::move(body));
    }

    static std::unique_ptr<Stmt> parseBreakStatement(TokenIter& it, TokenIter end) {
        ++it;
        if (it == end || !isToken<Tokenization::Semicolon>(it->token))
            throwSyntaxError(currentPosition(it, end), "Expected ';' after break");
        ++it;
        return std::make_unique<BreakStmt>();
    }

    static std::unique_ptr<Stmt> parseContinueStatement(TokenIter& it, TokenIter end) {
        ++it;
        if (it == end || !isToken<Tokenization::Semicolon>(it->token))
            throwSyntaxError(currentPosition(it, end), "Expected ';' after continue");
        ++it;
        return std::make_unique<ContinueStmt>();
    }

    static std::unique_ptr<Stmt> parseReturnStatement(TokenIter& it, TokenIter end) {
        ++it;
        std::vector<std::unique_ptr<Expr>> values;
        if (it != end && !isToken<Tokenization::Semicolon>(it->token)) {
            do {
                values.push_back(ExprParser::parse(it, end));
                if (it != end && isToken<Tokenization::Comma>(it->token)) ++it;
                else break;
            } while (true);
        }
        if (it == end || !isToken<Tokenization::Semicolon>(it->token))
            throwSyntaxError(currentPosition(it, end), "Expected ';' after return");
        ++it;
        return std::make_unique<ReturnStmt>(std::move(values));
    }

    static std::unique_ptr<Stmt> parseGotoStatement(TokenIter& it, TokenIter end) {
        ++it;
        if (it == end || !isToken<Tokenization::Identifier>(it->token))
            throwSyntaxError(currentPosition(it, end), "Expected label name after goto");
        std::string label = std::get<Tokenization::Identifier>(it->token).name;
        ++it;
        if (it == end || !isToken<Tokenization::Semicolon>(it->token))
            throwSyntaxError(currentPosition(it, end), "Expected ';' after goto");
        ++it;
        return std::make_unique<GotoStmt>(label);
    }

    static std::unique_ptr<Stmt> parseLabelOrExpressionStatement(TokenIter& it, TokenIter end, const Tokenization::Identifier& id) {
        auto saved = it;
        ++it;
        if (it != end && isToken<Tokenization::Colon>(it->token)) {
            ++it;
            auto stmt = parseStatement(it, end);
            return std::make_unique<LabelStmt>(id.name, std::move(stmt));
        }
        it = saved;
        auto expr = ExprParser::parse(it, end);
        if (it == end || !isToken<Tokenization::Semicolon>(it->token))
            throwSyntaxError(currentPosition(it, end), "Expected ';' after expression statement");
        ++it;
        return std::make_unique<ExprStmt>(std::move(expr));
    }

    static std::unique_ptr<Stmt> parseBlockStatement(TokenIter& it, TokenIter end) {
        BlockStmt block = parseBlock(it, end);
        return std::make_unique<BlockStmt>(std::move(block));
    }

    static std::unique_ptr<Stmt> parseStructEnumDeclaration(TokenIter& it, TokenIter end) {
        bool isStruct = isToken<Tokenization::Struct>(it->token);
        ++it;
        if (it == end || !isToken<Tokenization::Identifier>(it->token))
            throwSyntaxError(currentPosition(it, end), "Expected struct/enum name");
        std::string typeName = std::get<Tokenization::Identifier>(it->token).name;
        ++it;
        auto type = std::make_unique<NamedType>(typeName);
        if (it == end || !isToken<Tokenization::Identifier>(it->token))
            throwSyntaxError(currentPosition(it, end), "Expected variable name");
        std::string varName = std::get<Tokenization::Identifier>(it->token).name;
        ++it;
        std::optional<std::unique_ptr<Expr>> initializer;
        if (it != end && isToken<Tokenization::Assign>(it->token)) {
            ++it;
            if (isToken<Tokenization::LeftBrace>(it->token)) {
                ++it;
                std::vector<std::unique_ptr<Expr>> initValues;
                while (it != end && !isToken<Tokenization::RightBrace>(it->token)) {
                    initValues.push_back(ExprParser::parse(it, end));
                    if (it != end && isToken<Tokenization::Comma>(it->token)) ++it;
                    else break;
                }
                if (it == end || !isToken<Tokenization::RightBrace>(it->token))
                    throwSyntaxError(currentPosition(it, end), "Expected '}' after initializer list");
                ++it;
                initializer = std::make_unique<InitListExpr>(std::move(initValues));
            } else {
                initializer = ExprParser::parse(it, end);
            }
        }
        if (it == end || !isToken<Tokenization::Semicolon>(it->token))
            throwSyntaxError(currentPosition(it, end), "Expected ';' after variable declaration");
        ++it;
        return std::make_unique<VarDeclStmt>(std::move(type), varName, std::move(initializer));
    }

    static std::unique_ptr<Stmt> parseVarDeclaration(TokenIter& it, TokenIter end) {
        auto type = TypeParser::parse(it, end, true, true);
        if (it == end || !isToken<Tokenization::Identifier>(it->token))
            throwSyntaxError(currentPosition(it, end), "Expected variable name");
        std::string name = std::get<Tokenization::Identifier>(it->token).name;
        ++it;
        std::optional<std::unique_ptr<Expr>> initializer;
        if (it != end && isToken<Tokenization::Assign>(it->token)) {
            ++it;
            if (isToken<Tokenization::LeftBrace>(it->token)) {
                ++it;
                std::vector<std::unique_ptr<Expr>> initValues;
                while (it != end && !isToken<Tokenization::RightBrace>(it->token)) {
                    initValues.push_back(ExprParser::parse(it, end));
                    if (it != end && isToken<Tokenization::Comma>(it->token)) ++it;
                }
                if (it == end || !isToken<Tokenization::RightBrace>(it->token))
                    throwSyntaxError(currentPosition(it, end), "Expected '}'");
                ++it;
                initializer = std::make_unique<InitListExpr>(std::move(initValues));
            } else {
                initializer = ExprParser::parse(it, end);
            }
        }
        if (it == end || !isToken<Tokenization::Semicolon>(it->token))
            throwSyntaxError(currentPosition(it, end), "Expected ';' after variable declaration");
        ++it;
        return std::make_unique<VarDeclStmt>(std::move(type), name, std::move(initializer));
    }
};

// ---------- Парсер определений верхнего уровня ----------
class DefParser {
public:
    static std::unique_ptr<Def> parse(TokenIter& it, TokenIter end) {
        printToken(it, end, "DefParser::parse");
        if (it == end) throwSyntaxError(Util::Position(), "Expected definition");

        if (isToken<Tokenization::Int>(it->token) || isToken<Tokenization::Unsigned>(it->token) ||
            isToken<Tokenization::Float>(it->token) || isToken<Tokenization::Bool>(it->token) ||
            isToken<Tokenization::String>(it->token) || isToken<Tokenization::Void>(it->token)) {
            return parseFunctionOrGlobal(it, end);
        }
        if (isToken<Tokenization::Struct>(it->token)) return parseStructDefinition(it, end);
        if (isToken<Tokenization::Enum>(it->token)) return parseEnumDefinition(it, end);
        throwSyntaxError(it->position, "Expected function, global variable, struct or enum definition");
    }

private:
    static std::unique_ptr<Def> parseFunctionOrGlobal(TokenIter& it, TokenIter end) {
        auto firstType = TypeParser::parse(it, end, true, true);
        std::vector<std::unique_ptr<Type>> returnTypes;
        returnTypes.push_back(std::move(firstType));

        if (it != end && isToken<Tokenization::Comma>(it->token)) {
            ++it;
            do {
                auto nextType = TypeParser::parse(it, end, true, true);
                returnTypes.push_back(std::move(nextType));
                if (it == end || !isToken<Tokenization::Comma>(it->token)) break;
                ++it;
            } while (true);
        }

        if (it == end || !isToken<Tokenization::Identifier>(it->token))
            throwSyntaxError(currentPosition(it, end), "Expected function or variable name");
        std::string name = std::get<Tokenization::Identifier>(it->token).name;
        ++it;

        if (it != end && isToken<Tokenization::LeftParenthesis>(it->token))
            return parseFunction(it, end, std::move(returnTypes), name);
        else
            return parseGlobalVariable(it, end, std::move(returnTypes), name);
    }

    static std::unique_ptr<Def> parseFunction(TokenIter& it, TokenIter end,
                                               std::vector<std::unique_ptr<Type>> returnTypes,
                                               const std::string& name) {
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
                auto paramType = TypeParser::parse(it, end, true, true);
                std::string paramName;
                if (it != end && isToken<Tokenization::Identifier>(it->token)) {
                    paramName = std::get<Tokenization::Identifier>(it->token).name;
                    ++it;
                }
                params.emplace_back(paramName, std::move(paramType));
                if (it != end && isToken<Tokenization::Comma>(it->token)) ++it;
                else break;
            } while (true);
        }
        if (it == end || !isToken<Tokenization::RightParenthesis>(it->token))
            throwSyntaxError(currentPosition(it, end), "Expected ')' after parameters");
        ++it;

        std::optional<BlockStmt> body;
        if (it != end && isToken<Tokenization::Semicolon>(it->token)) {
            ++it;
        } else if (it != end && isToken<Tokenization::LeftBrace>(it->token)) {
            body = StmtParser::parseBlock(it, end);
        } else {
            throwSyntaxError(currentPosition(it, end), "Expected function body or ';'");
        }
        return std::make_unique<FunctionDef>(name, std::move(params), std::move(returnTypes), variadic, std::move(body));
    }

    static std::unique_ptr<Def> parseGlobalVariable(TokenIter& it, TokenIter end,
                                                     std::vector<std::unique_ptr<Type>> returnTypes,
                                                     const std::string& name) {
        if (returnTypes.size() > 1) {
            throwSyntaxError(currentPosition(it, end), "Global variable cannot have multiple return types");
        }
        std::optional<std::unique_ptr<Expr>> initializer;
        if (it != end && isToken<Tokenization::Assign>(it->token)) {
            ++it;
            initializer = ExprParser::parse(it, end);
        }
        if (it == end || !isToken<Tokenization::Semicolon>(it->token))
            throwSyntaxError(currentPosition(it, end), "Expected ';' after global variable");
        ++it;
        return std::make_unique<GlobalVarDef>(std::move(returnTypes[0]), name, std::move(initializer));
    }

    static std::unique_ptr<Def> parseStructDefinition(TokenIter& it, TokenIter end) {
        ++it;
        if (it == end || !isToken<Tokenization::Identifier>(it->token))
            throwSyntaxError(currentPosition(it, end), "Expected struct name");
        std::string name = std::get<Tokenization::Identifier>(it->token).name;
        ++it;
        if (it == end || !isToken<Tokenization::LeftBrace>(it->token))
            throwSyntaxError(currentPosition(it, end), "Expected '{' for struct definition");
        ++it;
        std::vector<std::pair<std::string, std::unique_ptr<Type>>> fields;
        while (it != end && !isToken<Tokenization::RightBrace>(it->token)) {
            auto fieldType = TypeParser::parse(it, end, true, true);
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
            throwSyntaxError(currentPosition(it, end), "Expected ';' after struct definition");
        ++it;
        return std::make_unique<StructDef>(name, std::move(fields));
    }

    static std::unique_ptr<Def> parseEnumDefinition(TokenIter& it, TokenIter end) {
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
        ++it;
        return std::make_unique<EnumDef>(name, std::move(enumerators));
    }
};

} // namespace detail

TranslationUnit Parser::parse(const std::vector<Tokenization::TokenInfo>& tokens) {
    positions_.clear();
    errors_.clear();
    warnings_.clear();

    TranslationUnit tu;
    detail::TokenIter it = tokens.begin();
    detail::TokenIter end = tokens.end();

    while (it != end) {
        try {
            auto pos = it->position;
            auto def = detail::DefParser::parse(it, end);
            tu.definitions.push_back(std::move(def));
            recordPosition(tu.definitions.back().get(), pos);
        } catch (const std::runtime_error& e) {
            addError(e.what());
            if (it != end) ++it;
        }
    }
    return tu;
}

Util::Position Parser::getPosition(const void* node) const {
    auto it = positions_.find(node);
    if (it != positions_.end()) return it->second;
    return Util::Position(0, 0);
}

} // namespace Parsing