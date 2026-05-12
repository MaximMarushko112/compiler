#pragma once

#include <string>

namespace Util {

struct Position {
    int line;
    int column;

    Position(int l = 0, int c = 0) : line(l), column(c) {}

    void advance() { ++column; }
    void newLine() { ++line; column = 0; }

    std::string toString() const {
        return std::to_string(line) + ":" + std::to_string(column);
    }
};

} // namespace Util