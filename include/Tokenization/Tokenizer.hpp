#pragma once

#include "Token.hpp"
#include <vector>
#include <string>
#include <optional>

namespace Tokenization {

class Tokenizer {
public:
    static std::vector<TokenInfo> tokenize(const std::string& source);

private:
    // Вспомогательные методы (реализация в cpp)
    static std::optional<std::pair<std::string, TokenVariant>> matchOperator(const std::string& source, size_t pos);
    static TokenVariant getKeywordOrIdentifier(const std::string& lexeme);
    static TokenVariant processNumber(const std::string& source, size_t& pos, Position& curPos);
    static TokenVariant processString(const std::string& source, size_t& pos, Position& curPos);
    static void skipComment(const std::string& source, size_t& pos, Position& curPos);
    static bool isWhitespace(char c);
    static bool isIdentifierChar(char c);
    static void handleSingleCharToken(char ch, std::vector<TokenInfo>& tokens, const Position& startPos, size_t& pos, Position& curPos);
};

} // namespace Tokenization