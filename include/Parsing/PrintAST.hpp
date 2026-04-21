#pragma once

#include "ASTNodes.hpp"
#include <iostream>

namespace Parsing {

void printAST(const TranslationUnit& tu, std::ostream& out = std::cout, int indent = 0);

} // namespace Parsing