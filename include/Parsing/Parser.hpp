#pragma once

#include "ASTNodes.hpp"
#include <Tokenization/Token.hpp>   // токены из токенизатора
#include <vector>
#include <deque>
#include <unordered_map>

namespace Parsing {

struct ParsingInfo {
    TranslationUnit ast;
    std::unordered_map<const void*, Util::Position> positions; // позиция каждого узла
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
};

class Parser {
public:
    static ParsingInfo parse(const std::vector<Tokenization::TokenInfo>& tokens);

private:
    // Вспомогательные структуры (скрыты в cpp)
};

} // namespace Parsing