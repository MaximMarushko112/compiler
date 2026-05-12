#pragma once

#include "ASTNodes.hpp"
#include <Tokenization/Token.hpp>
#include <vector>
#include <unordered_map>

namespace Parsing {

class Parser {
public:
    Parser() = default;

    // Основной метод: принимает токены, возвращает AST.
    TranslationUnit parse(const std::vector<Tokenization::TokenInfo>& tokens);

    // Получить позицию узла AST по указателю на него (после вызова parse).
    Util::Position getPosition(const void* node) const;

    const std::vector<std::string>& getErrors() const { return errors_; }
    const std::vector<std::string>& getWarnings() const { return warnings_; }

private:
    std::unordered_map<const void*, Util::Position> positions_;
    std::vector<std::string> errors_;
    std::vector<std::string> warnings_;

    void addError(const std::string& msg) { errors_.push_back(msg); }
    void addWarning(const std::string& msg) { warnings_.push_back(msg); }
    void recordPosition(const void* node, const Util::Position& pos) {
        positions_[node] = pos;
    }
};

} // namespace Parsing