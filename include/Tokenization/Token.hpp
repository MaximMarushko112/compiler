#pragma once

#include "TokenTypes.hpp"
#include <Utils/Position.hpp>

namespace Tokenization {

using Position = Util::Position;

struct TokenInfo {
    TokenVariant token;
    Position position;

    TokenInfo(const TokenVariant& tok, const Position& pos)
        : token(tok), position(pos) {}
};

} // namespace Tokenization