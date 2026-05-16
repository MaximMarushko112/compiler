#include "../include/Tokenization/Tokenizer.hpp"
#include <cctype>
#include <cassert>
#include <charconv>
#include <unordered_map>
#include <algorithm>
#include <stdexcept>

namespace Tokenization {

// ---------- Таблица операторов (максимальное совпадение) ----------
static const std::vector<std::pair<std::string, TokenVariant>> OPERATOR_TABLE = {
    {"...", Ellipsis{}},
    {"->", Arrow{}},
    {"<<", ShiftLeft{}},
    {">>", ShiftRight{}},
    {"<=", LessEqual{}},
    {">=", GreaterEqual{}},
    {"==", Equal{}},
    {"!=", NotEqual{}},
    {"&&", AndAnd{}},
    {"||", OrOr{}},
    {"+", Plus{}},
    {"-", Minus{}},
    {"*", Star{}},
    {"/", Slash{}},
    {"%", Percent{}},
    {"=", Assign{}},
    {"!", Not{}},
    {"<", Less{}},
    {">", Greater{}},
    {"&", And{}},
    {"|", Or{}},
    {"^", Xor{}},
    {"++", PlusPlus{}},
    {"--", MinusMinus{}},
};

static std::vector<std::pair<std::string, TokenVariant>> buildSortedOperators() {
    auto table = OPERATOR_TABLE;
    std::sort(table.begin(), table.end(),
              [](const auto& a, const auto& b) { return a.first.size() > b.first.size(); });
    return table;
}

static const auto SORTED_OPERATORS = buildSortedOperators();

// ---------- Таблица ключевых слов ----------
static const std::unordered_map<std::string, TokenVariant> KEYWORD_MAP = {
    {"const", Const{}},
    {"continue", Continue{}},
    {"do", Do{}},
    {"else", Else{}},
    {"enum", Enum{}},
    {"float", Float{}},
    {"for", For{}},
    {"if", If{}},
    {"int", Int{}},
    {"return", Return{}},
    {"sizeof", Sizeof{}},
    {"struct", Struct{}},
    {"switch", Switch{}},
    {"unsigned", Unsigned{}},
    {"void", Void{}},
    {"while", While{}},
    {"bool", Bool{}},
    {"string", String{}},
};

// ---------- Утилиты ----------
bool Tokenizer::isWhitespace(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\f' || c == '\v';
}

bool Tokenizer::isIdentifierChar(char c) {
    return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
}

// ---------- Операторы ----------
std::optional<std::pair<std::string, TokenVariant>> Tokenizer::matchOperator(const std::string& source, size_t pos) {
    for (const auto& [opStr, tok] : SORTED_OPERATORS) {
        if (source.compare(pos, opStr.size(), opStr) == 0) {
            return std::make_pair(opStr, tok);
        }
    }
    return std::nullopt;
}

// ---------- Ключевые слова / идентификаторы ----------
TokenVariant Tokenizer::getKeywordOrIdentifier(const std::string& lexeme) {
    auto it = KEYWORD_MAP.find(lexeme);
    if (it != KEYWORD_MAP.end())
        return it->second;
    return Identifier{lexeme};
}

// ---------- Числа (целые и с плавающей точкой) ----------
TokenVariant Tokenizer::processNumber(const std::string& source, size_t& pos, Position& curPos) {
    size_t start = pos;
    bool hasDot = false;

    while (pos < source.size() && (std::isdigit(source[pos]) || source[pos] == '.')) {
        if (source[pos] == '.') {
            if (hasDot) break; // вторая точка – не число
            hasDot = true;
        }
        ++pos;
        curPos.advance();
    }

    std::string_view numStr(source.data() + start, pos - start);
    if (hasDot) {
        float val = 0.0f;
        std::from_chars(numStr.data(), numStr.data() + numStr.size(), val);
        return FloatLiteral{val};
    } else {
        int val = 0;
        std::from_chars(numStr.data(), numStr.data() + numStr.size(), val);
        return IntLiteral{val};
    }
}

// ---------- Строковые литералы (с простыми escape) ----------
TokenVariant Tokenizer::processString(const std::string& source, size_t& pos, Position& curPos) {
    if (source[pos] != '"') {
        throw std::runtime_error("Expected string literal starting with '\"'");
    }
    ++pos;
    curPos.advance();

    std::string result;
    while (pos < source.size() && source[pos] != '"') {
        if (source[pos] == '\\') {
            ++pos;
            curPos.advance();
            if (pos >= source.size()) break;
            switch (source[pos]) {
                case 'n': result += '\n'; break;
                case 't': result += '\t'; break;
                case '\\': result += '\\'; break;
                case '"': result += '"'; break;
                default: result += source[pos]; break;
            }
        } else {
            result += source[pos];
        }
        ++pos;
        curPos.advance();
    }

    if (pos >= source.size() || source[pos] != '"') {
        throw std::runtime_error("Unclosed string literal");
    }
    ++pos;
    curPos.advance();

    return StringLiteral{result};
}

// ---------- Комментарии (// и /* */) ----------
void Tokenizer::skipComment(const std::string& source, size_t& pos, Position& curPos) {
    if (pos + 1 >= source.size()) return;

    if (source[pos] == '/' && source[pos+1] == '/') {
        // однострочный
        while (pos < source.size() && source[pos] != '\n') {
            ++pos;
            curPos.advance();
        }
        // '\n' будет обработан в основном цикле
    }
    else if (source[pos] == '/' && source[pos+1] == '*') {
        // многострочный
        pos += 2;
        curPos.advance(); curPos.advance();
        while (pos + 1 < source.size() && !(source[pos] == '*' && source[pos+1] == '/')) {
            if (source[pos] == '\n') {
                curPos.newLine();
            } else {
                curPos.advance();
            }
            ++pos;
        }
        if (pos + 1 >= source.size()) {
            throw std::runtime_error("Unclosed block comment");
        }
        pos += 2;
        curPos.advance(); curPos.advance();
    }
}

// ---------- Обработка одиночных символов-разделителей ----------
void Tokenizer::handleSingleCharToken(char ch, std::vector<TokenInfo>& tokens, const Position& startPos,
                                      size_t& pos, Position& curPos) {
    switch (ch) {
        case '(': tokens.emplace_back(LeftParenthesis{}, startPos); break;
        case ')': tokens.emplace_back(RightParenthesis{}, startPos); break;
        case '{': tokens.emplace_back(LeftBrace{}, startPos); break;
        case '}': tokens.emplace_back(RightBrace{}, startPos); break;
        case ':': tokens.emplace_back(Colon{}, startPos); break;
        case ',': tokens.emplace_back(Comma{}, startPos); break;
        case '.': tokens.emplace_back(Dot{}, startPos); break;
        case ';': tokens.emplace_back(Semicolon{}, startPos); break;
        default:
            throw std::runtime_error(std::string("Unexpected character: ") + ch);
    }
    ++pos;
    curPos.advance();
}

// ---------- Новые вспомогательные методы для упрощения tokenize ----------

bool Tokenizer::tryProcessNumber(const std::string& source, size_t& pos, Position& curPos,
                                 std::vector<TokenInfo>& tokens, const Position& startPos) {
    if (std::isdigit(static_cast<unsigned char>(source[pos]))) {
        TokenVariant tok = processNumber(source, pos, curPos);
        tokens.emplace_back(tok, startPos);
        return true;
    }
    return false;
}

bool Tokenizer::tryProcessString(const std::string& source, size_t& pos, Position& curPos,
                                 std::vector<TokenInfo>& tokens, const Position& startPos) {
    if (source[pos] == '"') {
        TokenVariant tok = processString(source, pos, curPos);
        tokens.emplace_back(tok, startPos);
        return true;
    }
    return false;
}

bool Tokenizer::tryProcessOperator(const std::string& source, size_t& pos, Position& curPos,
                                   std::vector<TokenInfo>& tokens, const Position& startPos) {
    if (auto op = matchOperator(source, pos)) {
        const auto& [opStr, tok] = *op;
        tokens.emplace_back(tok, startPos);
        pos += opStr.size();
        for (size_t i = 0; i < opStr.size(); ++i) curPos.advance();
        return true;
    }
    return false;
}

bool Tokenizer::tryProcessIdentifier(const std::string& source, size_t& pos, Position& curPos,
                                     std::vector<TokenInfo>& tokens, const Position& startPos) {
    char ch = source[pos];
    if (std::isalpha(static_cast<unsigned char>(ch)) || ch == '_') {
        size_t start = pos;
        while (pos < source.size() && isIdentifierChar(source[pos])) {
            ++pos;
            curPos.advance();
        }
        std::string lexeme = source.substr(start, pos - start);
        TokenVariant tok = getKeywordOrIdentifier(lexeme);
        tokens.emplace_back(tok, startPos);
        return true;
    }
    return false;
}

void Tokenizer::processSingleChar(char ch, const Position& startPos, size_t& pos, Position& curPos,
                                  std::vector<TokenInfo>& tokens) {
    handleSingleCharToken(ch, tokens, startPos, pos, curPos);
}

// ---------- Главный метод токенизации ----------
std::vector<TokenInfo> Tokenizer::tokenize(const std::string& source) {
    std::vector<TokenInfo> tokens;
    size_t pos = 0;
    Position curPos{0, 0};

    while (pos < source.size()) {
        char ch = source[pos];

        // Новая строка
        if (ch == '\n') {
            curPos.newLine();
            ++pos;
            continue;
        }

        // Пробелы
        if (isWhitespace(ch)) {
            ++pos;
            curPos.advance();
            continue;
        }

        // Комментарии
        if (ch == '/' && pos + 1 < source.size() && (source[pos+1] == '/' || source[pos+1] == '*')) {
            skipComment(source, pos, curPos);
            continue;
        }

        Position startPos = curPos;

        // Обработка различных типов лексем (каждый метод сам обновляет pos/curPos и добавляет токен)
        if (tryProcessNumber(source, pos, curPos, tokens, startPos)) continue;
        if (tryProcessString(source, pos, curPos, tokens, startPos)) continue;
        if (tryProcessOperator(source, pos, curPos, tokens, startPos)) continue;
        if (tryProcessIdentifier(source, pos, curPos, tokens, startPos)) continue;

        // Отдельные символы (скобки, точки с запятой и т.п.)
        processSingleChar(ch, startPos, pos, curPos, tokens);
    }

    return tokens;
}

} // namespace Tokenization