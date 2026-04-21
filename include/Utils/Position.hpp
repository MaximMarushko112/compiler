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
        char buf[32];
        snprintf(buf, sizeof(buf), "%d:%d", line, column);
        return buf;
    }
};

} // namespace Util