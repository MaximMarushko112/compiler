#include "../../include/Parsing/PrintAST.hpp"
#include "../../include/Utils/Overloaded.hpp"

namespace Parsing {

static std::string indentStr(int indent) {
    return std::string(indent * 2, ' ');
}

static void printType(const TypeNode& type, std::ostream& out, int indent) {
    std::visit(Util::overloaded{
        [&](const VoidType&) { out << "void"; },
        [&](const IntType&) { out << "int"; },
        [&](const UnsignedType&) { out << "unsigned"; },
        [&](const FloatType&) { out << "float"; },
        [&](const BoolType&) { out << "bool"; },
        [&](const StringType&) { out << "string"; },
        [&](const NamedType& n) { out << n.name; },
        [&](const PointerType& p) { printType(*p.pointee, out, indent); out << "*"; },
        [&](const ArrayType& a) { printType(*a.elementType, out, indent); out << "["; if (a.size) out << *a.size; out << "]"; },
        [&](const FunctionType& f) {
            out << "(";
            for (size_t i = 0; i < f.parameterTypes.size(); ++i) {
                if (i) out << ", ";
                printType(f.parameterTypes[i], out, indent);
            }
            if (f.variadic) out << ", ...";
            out << ") -> ";
            if (f.returnTypes.size() == 1) printType(f.returnTypes[0], out, indent);
            else {
                out << "(";
                for (size_t i = 0; i < f.returnTypes.size(); ++i) {
                    if (i) out << ", ";
                    printType(f.returnTypes[i], out, indent);
                }
                out << ")";
            }
        },
        [&](const TupleType& t) {
            out << "(";
            for (size_t i = 0; i < t.types.size(); ++i) {
                if (i) out << ", ";
                printType(t.types[i], out, indent);
            }
            out << ")";
        }
    }, type);
}

static void printExpr(const ExprNode& expr, std::ostream& out, int indent);
static void printStmt(const StmtNode& stmt, std::ostream& out, int indent);

static void printExpr(const ExprNode& expr, std::ostream& out, int indent) {
    std::visit(Util::overloaded{
        [&](const IntLiteral& lit) { out << lit.value; },
        [&](const FloatLiteral& lit) { out << lit.value; },
        [&](const StringLiteral& lit) { out << "\"" << lit.value << "\""; },
        [&](const BoolLiteral& lit) { out << (lit.value ? "true" : "false"); },
        [&](const VariableExpr& var) { out << var.name; },
        [&](const UnaryOp& op) {
            switch (op.op) {
                case UnaryOp::Op::Not: out << "!"; break;
                case UnaryOp::Op::Minus: out << "-"; break;
                case UnaryOp::Op::Plus: out << "+"; break;
                case UnaryOp::Op::AddressOf: out << "&"; break;
                case UnaryOp::Op::Dereference: out << "*"; break;
                default: out << "?";
            }
            printExpr(*op.operand, out, indent);
        },
        [&](const BinaryOp& op) {
            out << "(";
            printExpr(*op.left, out, indent);
            switch (op.op) {
                case BinaryOp::Op::Add: out << " + "; break;
                case BinaryOp::Op::Sub: out << " - "; break;
                case BinaryOp::Op::Mul: out << " * "; break;
                case BinaryOp::Op::Div: out << " / "; break;
                case BinaryOp::Op::Rem: out << " % "; break;
                case BinaryOp::Op::ShiftLeft: out << " << "; break;
                case BinaryOp::Op::ShiftRight: out << " >> "; break;
                case BinaryOp::Op::Less: out << " < "; break;
                case BinaryOp::Op::LessEqual: out << " <= "; break;
                case BinaryOp::Op::Greater: out << " > "; break;
                case BinaryOp::Op::GreaterEqual: out << " >= "; break;
                case BinaryOp::Op::Equal: out << " == "; break;
                case BinaryOp::Op::NotEqual: out << " != "; break;
                case BinaryOp::Op::BitAnd: out << " & "; break;
                case BinaryOp::Op::BitXor: out << " ^ "; break;
                case BinaryOp::Op::BitOr: out << " | "; break;
                case BinaryOp::Op::LogicalAnd: out << " && "; break;
                case BinaryOp::Op::LogicalOr: out << " || "; break;
            }
            printExpr(*op.right, out, indent);
            out << ")";
        },
        [&](const AssignExpr& a) {
            printExpr(*a.left, out, indent);
            switch (a.op) {
                case AssignExpr::Op::Assign: out << " = "; break;
                case AssignExpr::Op::AddAssign: out << " += "; break;
                case AssignExpr::Op::SubAssign: out << " -= "; break;
                case AssignExpr::Op::MulAssign: out << " *= "; break;
                case AssignExpr::Op::DivAssign: out << " /= "; break;
                case AssignExpr::Op::RemAssign: out << " %= "; break;
                case AssignExpr::Op::ShiftLeftAssign: out << " <<= "; break;
                case AssignExpr::Op::ShiftRightAssign: out << " >>= "; break;
                case AssignExpr::Op::AndAssign: out << " &= "; break;
                case AssignExpr::Op::XorAssign: out << " ^= "; break;
                case AssignExpr::Op::OrAssign: out << " |= "; break;
            }
            printExpr(*a.right, out, indent);
        },
        [&](const MultiAssignExpr& m) {
            for (size_t i = 0; i < m.lefts.size(); ++i) {
                if (i) out << ", ";
                printExpr(*m.lefts[i], out, indent);
            }
            out << " = ";
            for (size_t i = 0; i < m.rights.size(); ++i) {
                if (i) out << ", ";
                printExpr(*m.rights[i], out, indent);
            }
        },
        [&](const ConditionalExpr& c) {
            printExpr(*c.cond, out, indent);
            out << " ? ";
            printExpr(*c.thenExpr, out, indent);
            out << " : ";
            printExpr(*c.elseExpr, out, indent);
        },
        [&](const CallExpr& c) {
            printExpr(*c.callee, out, indent);
            out << "(";
            for (size_t i = 0; i < c.arguments.size(); ++i) {
                if (i) out << ", ";
                printExpr(*c.arguments[i], out, indent);
            }
            out << ")";
        },
        [&](const FieldAccessExpr& f) {
            printExpr(*f.object, out, indent);
            out << (f.isArrow ? "->" : ".") << f.field;
        },
        [&](const IndexExpr& i) {
            printExpr(*i.array, out, indent);
            out << "[";
            printExpr(*i.index, out, indent);
            out << "]";
        },
        [&](const CastExpr& c) {
            out << "("; printType(c.targetType, out, indent); out << ")";
            printExpr(*c.operand, out, indent);
        },
        [&](const SizeofExpr& s) {
            out << "sizeof ";
            if (std::holds_alternative<TypeNode>(s.operand)) {
                out << "("; printType(std::get<TypeNode>(s.operand), out, indent); out << ")";
            } else {
                printExpr(*std::get<Util::Boxed<ExprNode>>(s.operand), out, indent);
            }
        },
        [&](const InitListExpr& list) {
            out << "{ ";
            for (size_t i = 0; i < list.values.size(); ++i) {
                if (i) out << ", ";
                printExpr(*list.values[i], out, indent);
            }
            out << " }";
        }
    }, expr);
}

static void printStmt(const StmtNode& stmt, std::ostream& out, int indent) {
    std::visit(Util::overloaded{
        [&](const ExprStmt& e) {
            out << indentStr(indent); printExpr(*e.expr, out, indent); out << ";\n";
        },
        [&](const BlockStmt& b) {
            out << indentStr(indent) << "{\n";
            for (const auto& s : b.statements) printStmt(*s, out, indent + 1);
            out << indentStr(indent) << "}\n";
        },
        [&](const IfStmt& i) {
            out << indentStr(indent) << "if (";
            printExpr(*i.condition, out, indent);
            out << ") ";
            if (std::holds_alternative<BlockStmt>(*i.thenStmt)) {
                out << "\n";
                printStmt(*i.thenStmt, out, indent);
            } else {
                out << "\n" << indentStr(indent+1);
                printStmt(*i.thenStmt, out, indent+1);
            }
            if (i.elseStmt) {
                out << indentStr(indent) << "else ";
                if (std::holds_alternative<BlockStmt>(**i.elseStmt)) {
                    out << "\n";
                    printStmt(**i.elseStmt, out, indent);
                } else {
                    out << "\n" << indentStr(indent+1);
                    printStmt(**i.elseStmt, out, indent+1);
                }
            }
        },
        [&](const WhileStmt& w) {
            out << indentStr(indent) << "while (";
            printExpr(*w.condition, out, indent);
            out << ") ";
            printStmt(*w.body, out, indent);
        },
        [&](const DoWhileStmt& d) {
            out << indentStr(indent) << "do ";
            printStmt(*d.body, out, indent);
            out << indentStr(indent) << "while (";
            printExpr(*d.condition, out, indent);
            out << ");\n";
        },
        [&](const ForStmt& f) {
            out << indentStr(indent) << "for (";
            if (f.init) {
                if (std::holds_alternative<VarDeclStmt>(**f.init)) {
                    const auto& decl = std::get<VarDeclStmt>(**f.init);
                    printType(decl.type, out, indent);
                    out << " " << decl.name;
                    if (decl.initializer) {
                        out << " = "; printExpr(**decl.initializer, out, indent);
                    }
                } else {
                    printExpr(*std::get<ExprStmt>(**f.init).expr, out, indent);
                }
            }
            out << "; ";
            if (f.condition) printExpr(**f.condition, out, indent);
            out << "; ";
            if (f.increment) printExpr(**f.increment, out, indent);
            out << ") ";
            printStmt(*f.body, out, indent);
        },
        [&](const SwitchStmt& s) {
            out << indentStr(indent) << "switch (";
            printExpr(*s.control, out, indent);
            out << ") ";
            printStmt(*s.body, out, indent);
        },
        [&](const CaseStmt& c) {
            if (std::holds_alternative<CaseLabel>(c.label)) {
                out << indentStr(indent) << "case ";
                printExpr(*std::get<CaseLabel>(c.label).value, out, indent);
                out << ":\n";
            } else {
                out << indentStr(indent) << "default:\n";
            }
            printStmt(*c.statement, out, indent+1);
        },
        [&](const BreakStmt&) { out << indentStr(indent) << "break;\n"; },
        [&](const ContinueStmt&) { out << indentStr(indent) << "continue;\n"; },
        [&](const ReturnStmt& r) {
            out << indentStr(indent) << "return";
            if (!r.values.empty()) {
                out << " ";
                for (size_t i = 0; i < r.values.size(); ++i) {
                    if (i) out << ", ";
                    printExpr(*r.values[i], out, indent);
                }
            }
            out << ";\n";
        },
        [&](const GotoStmt& g) { out << indentStr(indent) << "goto " << g.label << ";\n"; },
        [&](const LabelStmt& l) {
            out << indentStr(indent) << l.name << ":\n";
            printStmt(*l.statement, out, indent);
        },
        [&](const VarDeclStmt& v) {
            out << indentStr(indent);
            printType(v.type, out, indent);
            out << " " << v.name;
            if (v.initializer) {
                out << " = "; printExpr(**v.initializer, out, indent);
            }
            out << ";\n";
        }
    }, stmt);
}

static void printBlockStmt(const BlockStmt& block, std::ostream& out, int indent) {
    out << indentStr(indent) << "{\n";
    for (const auto& stmt : block.statements) {
        printStmt(*stmt, out, indent + 1);
    }
    out << indentStr(indent) << "}\n";
}

static void printDef(const DefNode& def, std::ostream& out, int indent) {
    std::visit(Util::overloaded{
        [&](const FunctionDef& f) {
            out << indentStr(indent);
            // возвращаемые типы
            if (f.returnTypes.size() == 1) printType(f.returnTypes[0], out, indent);
            else {
                out << "(";
                for (size_t i = 0; i < f.returnTypes.size(); ++i) {
                    if (i) out << ", ";
                    printType(f.returnTypes[i], out, indent);
                }
                out << ")";
            }
            out << " " << f.name << "(";
            for (size_t i = 0; i < f.parameters.size(); ++i) {
                if (i) out << ", ";
                printType(f.parameters[i].type, out, indent);
                if (!f.parameters[i].name.empty()) out << " " << f.parameters[i].name;
            }
            if (f.variadic) {
                if (!f.parameters.empty()) out << ", ";
                out << "...";
            }
            out << ")";
            if (f.body) {
                out << " ";
                printBlockStmt(*f.body, out, indent);
            } else {
                out << ";\n";
            }
        },
        [&](const GlobalVarDef& g) {
            out << indentStr(indent);
            printType(g.type, out, indent);
            out << " " << g.name;
            if (g.initializer) {
                out << " = "; printExpr(*g.initializer, out, indent);
            }
            out << ";\n";
        },
        [&](const StructDef& s) {
            out << indentStr(indent) << "struct " << s.name << " {\n";
            for (const auto& [name, type] : s.fields) {
                out << indentStr(indent+1);
                printType(type, out, indent+1);
                out << " " << name << ";\n";
            }
            out << indentStr(indent) << "};\n";
        },
        [&](const EnumDef& e) {
            out << indentStr(indent) << "enum " << e.name << " {\n";
            for (size_t i = 0; i < e.enumerators.size(); ++i) {
                out << indentStr(indent+1) << e.enumerators[i].first;
                if (e.enumerators[i].second) out << " = " << *e.enumerators[i].second;
                if (i != e.enumerators.size()-1) out << ",";
                out << "\n";
            }
            out << indentStr(indent) << "};\n";
        }
    }, def);
}

void printAST(const TranslationUnit& tu, std::ostream& out, int indent) {
    for (const auto& def : tu.definitions) {
        printDef(def, out, indent);
        out << "\n";
    }
}

} // namespace Parsing