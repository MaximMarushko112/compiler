#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <cctype>
#include <unordered_map>
#include <stdexcept>

// ----------------------------------------------------------------------
// Перечисления для категорий токенов
// ----------------------------------------------------------------------

// Типы данных (в будущем расширяется)
enum class DataType {
    INT
};

// Ключевые слова (управляющие конструкции)
enum class Keyword {
    DECLARE,
    IF,
    ELSE,
    PRINT
};

// Операторы (все возможные, включая составные)
enum class Operator {
    // Арифметические
    PLUS,       // +
    MINUS,      // - (унарный или бинарный)
    MULTIPLY,   // *
    DIVIDE,     // /
    // Сравнения
    EQ,         // ==
    NE,         // !=
    LT,         // <
    GT,         // >
    LE,         // <=
    GE,         // >=
    // Присваивание
    ASSIGN      // =
};

// Знаки пунктуации
enum class Punctuation {
    COLON,      // :
    SEMICOLON,  // ;
    LPAREN,     // (
    RPAREN,     // )
    LBRACE,     // {
    RBRACE      // }
};

// ----------------------------------------------------------------------
// Базовый класс Token
// ----------------------------------------------------------------------
class Token {
public:
    virtual ~Token() = default;
    virtual std::string toString() const = 0;
};

// ----------------------------------------------------------------------
// Производные классы токенов
// ----------------------------------------------------------------------
class NumberToken : public Token {
    int value_;
public:
    explicit NumberToken(int value) : value_(value) {}
    std::string toString() const override {
        return "Number(" + std::to_string(value_) + ")";
    }
};

class IdentifierToken : public Token {
    std::string name_;
public:
    explicit IdentifierToken(std::string name) : name_(std::move(name)) {}
    std::string toString() const override {
        return "Identifier(" + name_ + ")";
    }
};

class DataTypeToken : public Token {
    DataType kind_;
public:
    explicit DataTypeToken(DataType kind) : kind_(kind) {}
    std::string toString() const override {
        static const std::unordered_map<DataType, std::string> map = {
            {DataType::INT, "int"}
        };
        return "DataType(" + map.at(kind_) + ")";
    }
};

class KeywordToken : public Token {
    Keyword kind_;
public:
    explicit KeywordToken(Keyword kind) : kind_(kind) {}
    std::string toString() const override {
        static const std::unordered_map<Keyword, std::string> map = {
            {Keyword::DECLARE, "declare"},
            {Keyword::IF, "if"},
            {Keyword::ELSE, "else"},
            {Keyword::PRINT, "print"}
        };
        return "Keyword(" + map.at(kind_) + ")";
    }
};

class OperatorToken : public Token {
    Operator kind_;
public:
    explicit OperatorToken(Operator kind) : kind_(kind) {}
    std::string toString() const override {
        static const std::unordered_map<Operator, std::string> map = {
            {Operator::PLUS, "+"},
            {Operator::MINUS, "-"},
            {Operator::MULTIPLY, "*"},
            {Operator::DIVIDE, "/"},
            {Operator::EQ, "=="},
            {Operator::NE, "!="},
            {Operator::LT, "<"},
            {Operator::GT, ">"},
            {Operator::LE, "<="},
            {Operator::GE, ">="},
            {Operator::ASSIGN, "="}
        };
        return "Operator(" + map.at(kind_) + ")";
    }
};

class PunctuationToken : public Token {
    Punctuation kind_;
public:
    explicit PunctuationToken(Punctuation kind) : kind_(kind) {}
    std::string toString() const override {
        static const std::unordered_map<Punctuation, std::string> map = {
            {Punctuation::COLON, ":"},
            {Punctuation::SEMICOLON, ";"},
            {Punctuation::LPAREN, "("},
            {Punctuation::RPAREN, ")"},
            {Punctuation::LBRACE, "{"},
            {Punctuation::RBRACE, "}"}
        };
        return "Punctuation(" + map.at(kind_) + ")";
    }
};

// ----------------------------------------------------------------------
// Токенизатор (разбит на небольшие функции)
// ----------------------------------------------------------------------

// Вспомогательная функция для проверки, является ли символ буквой (поддержка идентификаторов)
bool isAlpha(char c) {
    return std::isalpha(static_cast<unsigned char>(c)) != 0;
}

bool isDigit(char c) {
    return std::isdigit(static_cast<unsigned char>(c)) != 0;
}

bool isAlphaNumeric(char c) {
    return isAlpha(c) || isDigit(c);
}

// Пропуск пробельных символов
void skipWhitespace(const std::string& text, size_t& pos) {
    while (pos < text.size() && std::isspace(static_cast<unsigned char>(text[pos]))) {
        ++pos;
    }
}

// Попытка распознать число
std::unique_ptr<Token> tryTokenizeNumber(const std::string& text, size_t& pos) {
    size_t start = pos;
    if (pos < text.size() && isDigit(text[pos])) {
        int value = 0;
        while (pos < text.size() && isDigit(text[pos])) {
            value = value * 10 + (text[pos] - '0');
            ++pos;
        }
        return std::make_unique<NumberToken>(value);
    }
    return nullptr;
}

// Попытка распознать идентификатор, ключевое слово или тип данных
std::unique_ptr<Token> tryTokenizeIdentifierOrKeyword(const std::string& text, size_t& pos) {
    size_t start = pos;
    if (pos < text.size() && isAlpha(text[pos])) {
        std::string ident;
        while (pos < text.size() && isAlphaNumeric(text[pos])) {
            ident += text[pos];
            ++pos;
        }

        // Таблица ключевых слов
        static const std::unordered_map<std::string, Keyword> keywords = {
            {"declare", Keyword::DECLARE},
            {"if", Keyword::IF},
            {"else", Keyword::ELSE},
            {"print", Keyword::PRINT}
        };

        // Таблица типов данных
        static const std::unordered_map<std::string, DataType> dataTypes = {
            {"int", DataType::INT}
        };

        auto itKw = keywords.find(ident);
        if (itKw != keywords.end()) {
            return std::make_unique<KeywordToken>(itKw->second);
        }

        auto itDt = dataTypes.find(ident);
        if (itDt != dataTypes.end()) {
            return std::make_unique<DataTypeToken>(itDt->second);
        }

        return std::make_unique<IdentifierToken>(ident);
    }
    return nullptr;
}

// Попытка распознать оператор (включая составные)
std::unique_ptr<Token> tryTokenizeOperator(const std::string& text, size_t& pos) {
    if (pos >= text.size()) return nullptr;

    char c = text[pos];
    // Обработка составных операторов (двухсимвольных)
    if (c == '=' && pos + 1 < text.size() && text[pos + 1] == '=') {
        pos += 2;
        return std::make_unique<OperatorToken>(Operator::EQ);
    }
    if (c == '!' && pos + 1 < text.size() && text[pos + 1] == '=') {
        pos += 2;
        return std::make_unique<OperatorToken>(Operator::NE);
    }
    if (c == '<' && pos + 1 < text.size() && text[pos + 1] == '=') {
        pos += 2;
        return std::make_unique<OperatorToken>(Operator::LE);
    }
    if (c == '>' && pos + 1 < text.size() && text[pos + 1] == '=') {
        pos += 2;
        return std::make_unique<OperatorToken>(Operator::GE);
    }

    // Односимвольные операторы
    switch (c) {
        case '+': pos++; return std::make_unique<OperatorToken>(Operator::PLUS);
        case '-': pos++; return std::make_unique<OperatorToken>(Operator::MINUS);
        case '*': pos++; return std::make_unique<OperatorToken>(Operator::MULTIPLY);
        case '/': pos++; return std::make_unique<OperatorToken>(Operator::DIVIDE);
        case '<': pos++; return std::make_unique<OperatorToken>(Operator::LT);
        case '>': pos++; return std::make_unique<OperatorToken>(Operator::GT);
        case '=': pos++; return std::make_unique<OperatorToken>(Operator::ASSIGN);
        default: return nullptr;
    }
}

// Попытка распознать пунктуацию
std::unique_ptr<Token> tryTokenizePunctuation(const std::string& text, size_t& pos) {
    if (pos >= text.size()) return nullptr;

    char c = text[pos];
    switch (c) {
        case ':': pos++; return std::make_unique<PunctuationToken>(Punctuation::COLON);
        case ';': pos++; return std::make_unique<PunctuationToken>(Punctuation::SEMICOLON);
        case '(': pos++; return std::make_unique<PunctuationToken>(Punctuation::LPAREN);
        case ')': pos++; return std::make_unique<PunctuationToken>(Punctuation::RPAREN);
        case '{': pos++; return std::make_unique<PunctuationToken>(Punctuation::LBRACE);
        case '}': pos++; return std::make_unique<PunctuationToken>(Punctuation::RBRACE);
        default: return nullptr;
    }
}

// Основная функция токенизации
std::vector<std::unique_ptr<Token>> tokenize(const std::string& text) {
    std::vector<std::unique_ptr<Token>> tokens;
    size_t pos = 0;

    while (pos < text.size()) {
        // Пропускаем пробелы
        skipWhitespace(text, pos);
        if (pos >= text.size()) break;

        // Пробуем распознать число
        auto token = tryTokenizeNumber(text, pos);
        if (token) {
            tokens.push_back(std::move(token));
            continue;
        }

        // Пробуем распознать идентификатор/ключевое слово/тип
        token = tryTokenizeIdentifierOrKeyword(text, pos);
        if (token) {
            tokens.push_back(std::move(token));
            continue;
        }

        // Пробуем распознать оператор
        token = tryTokenizeOperator(text, pos);
        if (token) {
            tokens.push_back(std::move(token));
            continue;
        }

        // Пробуем распознать пунктуацию
        token = tryTokenizePunctuation(text, pos);
        if (token) {
            tokens.push_back(std::move(token));
            continue;
        }

        // Если ничего не подошло — ошибка
        throw std::runtime_error(std::string("Unexpected character: ") + text[pos]);
    }

    return tokens;
}

// ----------------------------------------------------------------------
// Пример использования
// ----------------------------------------------------------------------
int main() {
    std::cout << "Введите программу (Ctrl+D или пустая строка для завершения ввода):\n";

    std::string input;
    std::string line;
    while (std::getline(std::cin, line)) {
        input += line + '\n';
    }

    if (input.empty()) {
        std::cout << "Пустой ввод.\n";
        return 0;
    }

    try {
        auto tokens = tokenize(input);

        std::cout << "\nТокены:\n";
        for (const auto& tok : tokens) {
            std::cout << tok->toString() << '\n';
        }
    } catch (const std::exception& e) {
        std::cerr << "Ошибка токенизации: " << e.what() << '\n';
        return 1;
    }

    return 0;
}