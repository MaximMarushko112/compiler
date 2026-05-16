#include "../../include/Parsing/PrintAST.hpp"
#include "../../include/Parsing/ASTNodes.hpp"

namespace Parsing {

PrintVisitor::PrintVisitor(std::ostream& os, int step)
    : out(os), indentStep(step) {}

void PrintVisitor::printIndent() {
    out << std::string(currentIndent, ' ');
}

void PrintVisitor::print(const TranslationUnit& tu) {
    for (const auto& def : tu.definitions) {
        def->accept(*this);
        out << '\n';
    }
}

void PrintVisitor::visit(const Type& type) {
    if (auto p = dynamic_cast<const PointerType*>(&type)) {
        printPointer(*p);
    } else if (auto a = dynamic_cast<const ArrayType*>(&type)) {
        printArray(*a);
    } else if (auto f = dynamic_cast<const FunctionType*>(&type)) {
        printFunction(*f);
    } else if (auto t = dynamic_cast<const TupleType*>(&type)) {
        printTuple(*t);
    } else if (auto n = dynamic_cast<const NamedType*>(&type)) {
        out << n->name;
    } else if (dynamic_cast<const VoidType*>(&type)) {
        out << "void";
    } else if (dynamic_cast<const IntType*>(&type)) {
        out << "int";
    } else if (dynamic_cast<const UnsignedType*>(&type)) {
        out << "unsigned";
    } else if (dynamic_cast<const FloatType*>(&type)) {
        out << "float";
    } else if (dynamic_cast<const BoolType*>(&type)) {
        out << "bool";
    } else if (dynamic_cast<const StringType*>(&type)) {
        out << "string";
    } else {
        // fallback – используем accept (не должно случиться)
        type.accept(*this);
    }
}

void PrintVisitor::printPointer(const PointerType& p) {
    visit(*p.pointee);
    out << '*';
}

void PrintVisitor::printArray(const ArrayType& a) {
    visit(*a.elementType);
    out << '[';
    if (a.size) out << *a.size;
    out << ']';
}

void PrintVisitor::printFunction(const FunctionType& f) {
    out << '(';
    for (size_t i = 0; i < f.parameterTypes.size(); ++i) {
        if (i) out << ", ";
        visit(*f.parameterTypes[i]);
    }
    if (f.variadic) out << ", ...";
    out << ") -> ";
    if (f.returnTypes.size() == 1) {
        visit(*f.returnTypes[0]);
    } else {
        out << '(';
        for (size_t i = 0; i < f.returnTypes.size(); ++i) {
            if (i) out << ", ";
            visit(*f.returnTypes[i]);
        }
        out << ')';
    }
}

void PrintVisitor::printTuple(const TupleType& t) {
    out << '(';
    for (size_t i = 0; i < t.types.size(); ++i) {
        if (i) out << ", ";
        visit(*t.types[i]);
    }
    out << ')';
}

// Expressions
void PrintVisitor::visit(const IntLiteral& lit)   { out << lit.value; }
void PrintVisitor::visit(const FloatLiteral& lit) { out << lit.value; }
void PrintVisitor::visit(const StringLiteral& lit){ out << '"' << lit.value << '"'; }
void PrintVisitor::visit(const BoolLiteral& lit)  { out << (lit.value ? "true" : "false"); }
void PrintVisitor::visit(const VariableExpr& var) { out << var.name; }

void PrintVisitor::visit(const UnaryOp& u) {
    switch (u.op) {
        case UnaryOp::Op::Not: out << '!'; break;
        case UnaryOp::Op::Minus: out << '-'; break;
        case UnaryOp::Op::Plus: out << '+'; break;
        case UnaryOp::Op::AddressOf: out << '&'; break;
        case UnaryOp::Op::Dereference: out << '*'; break;
        default: out << '?'; break;
    }
    u.operand->accept(*this);
}

void PrintVisitor::visit(const BinaryOp& b) {
    out << '(';
    b.left->accept(*this);
    switch (b.op) {
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
    b.right->accept(*this);
    out << ')';
}

void PrintVisitor::visit(const AssignExpr& a) {
    a.left->accept(*this);
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
    a.right->accept(*this);
}

void PrintVisitor::visit(const InitListExpr& l) {
    out << "{ ";
    for (size_t i = 0; i < l.values.size(); ++i) {
        if (i) out << ", ";
        l.values[i]->accept(*this);
    }
    out << " }";
}

void PrintVisitor::visit(const MultiAssignExpr& m) {
    for (size_t i = 0; i < m.lefts.size(); ++i) {
        if (i) out << ", ";
        m.lefts[i]->accept(*this);
    }
    out << " = ";
    for (size_t i = 0; i < m.rights.size(); ++i) {
        if (i) out << ", ";
        m.rights[i]->accept(*this);
    }
}

void PrintVisitor::visit(const ConditionalExpr& c) {
    c.cond->accept(*this);
    out << " ? ";
    c.thenExpr->accept(*this);
    out << " : ";
    c.elseExpr->accept(*this);
}

void PrintVisitor::visit(const CallExpr& c) {
    out << c.functionName << "(";
    for (size_t i = 0; i < c.arguments.size(); ++i) {
        if (i) out << ", ";
        c.arguments[i]->accept(*this);
    }
    out << ")";
}

void PrintVisitor::visit(const FieldAccessExpr& f) {
    f.object->accept(*this);
    out << (f.isArrow ? "->" : ".");
    out << f.field;
}

void PrintVisitor::visit(const IndexExpr& i) {
    i.array->accept(*this);
    out << '[';
    i.index->accept(*this);
    out << ']';
}

void PrintVisitor::visit(const CastExpr& c) {
    out << '(';
    c.targetType->accept(*this);
    out << ')';
    c.operand->accept(*this);
}

void PrintVisitor::visit(const SizeofExpr& s) {
    out << "sizeof ";
    if (std::holds_alternative<std::unique_ptr<Type>>(s.operand)) {
        out << '(';
        std::get<std::unique_ptr<Type>>(s.operand)->accept(*this);
        out << ')';
    } else {
        std::get<std::unique_ptr<Expr>>(s.operand)->accept(*this);
    }
}

void PrintVisitor::visit(const PreIncrement& e) {
    out << "++";
    e.operand->accept(*this);
}

void PrintVisitor::visit(const PostIncrement& e) {
    e.operand->accept(*this);
    out << "++";
}

void PrintVisitor::visit(const PreDecrement& e) {
    out << "--";
    e.operand->accept(*this);
}

void PrintVisitor::visit(const PostDecrement& e) {
    e.operand->accept(*this);
    out << "--";
}

// Statements
void PrintVisitor::visit(const ExprStmt& e) {
    printIndent();
    e.expr->accept(*this);
    out << ";\n";
}

void PrintVisitor::visit(const BlockStmt& b) {
    printIndent();
    out << "{\n";
    increaseIndent();
    for (const auto& stmt : b.statements) {
        stmt->accept(*this);
    }
    decreaseIndent();
    printIndent();
    out << "}\n";
}

void PrintVisitor::visit(const IfStmt& i) {
    printIndent();
    out << "if (";
    i.condition->accept(*this);
    out << ")\n";
    if (dynamic_cast<const BlockStmt*>(i.thenStmt.get())) {
        i.thenStmt->accept(*this);
    } else {
        increaseIndent();
        i.thenStmt->accept(*this);
        decreaseIndent();
    }
    if (i.elseStmt) {
        printIndent();
        out << "else\n";
        if (dynamic_cast<const BlockStmt*>((*i.elseStmt).get())) {
            (*i.elseStmt)->accept(*this);
        } else {
            increaseIndent();
            (*i.elseStmt)->accept(*this);
            decreaseIndent();
        }
    }
}

void PrintVisitor::visit(const WhileStmt& w) {
    printIndent();
    out << "while (";
    w.condition->accept(*this);
    out << ")\n";
    if (dynamic_cast<const BlockStmt*>(w.body.get())) {
        w.body->accept(*this);
    } else {
        increaseIndent();
        w.body->accept(*this);
        decreaseIndent();
    }
}

void PrintVisitor::visit(const DoWhileStmt& d) {
    printIndent();
    out << "do\n";
    if (dynamic_cast<const BlockStmt*>(d.body.get())) {
        d.body->accept(*this);
    } else {
        increaseIndent();
        d.body->accept(*this);
        decreaseIndent();
    }
    printIndent();
    out << "while (";
    d.condition->accept(*this);
    out << ");\n";
}

void PrintVisitor::visit(const ForStmt& f) {
    printIndent();
    out << "for (";
    if (f.init) {
        if (auto v = dynamic_cast<const VarDeclStmt*>(f.init->get())) {
            v->type->accept(*this);
            out << ' ' << v->name;
            if (v->initializer) {
                out << " = ";
                (*v->initializer)->accept(*this);
            }
        } else if (auto e = dynamic_cast<const ExprStmt*>(f.init->get())) {
            e->expr->accept(*this);
        }
    }
    out << "; ";
    if (f.condition) (*f.condition)->accept(*this);
    out << "; ";
    if (f.increment) (*f.increment)->accept(*this);
    out << ")\n";
    if (dynamic_cast<const BlockStmt*>(f.body.get())) {
        f.body->accept(*this);
    } else {
        increaseIndent();
        f.body->accept(*this);
        decreaseIndent();
    }
}

void PrintVisitor::visit(const SwitchStmt& s) {
    printIndent();
    out << "switch (";
    s.control->accept(*this);
    out << ")\n";
    if (dynamic_cast<const BlockStmt*>(s.body.get())) {
        s.body->accept(*this);
    } else {
        increaseIndent();
        s.body->accept(*this);
        decreaseIndent();
    }
}

void PrintVisitor::visit(const CaseStmt& c) {
    printIndent();
    if (auto label = std::get_if<CaseLabel>(&c.label)) {
        out << "case ";
        label->value->accept(*this);
        out << ":\n";
    } else {
        out << "default:\n";
    }
    increaseIndent();
    c.statement->accept(*this);
    decreaseIndent();
}

void PrintVisitor::visit(const BreakStmt&) {
    printIndent();
    out << "break;\n";
}

void PrintVisitor::visit(const ContinueStmt&) {
    printIndent();
    out << "continue;\n";
}

void PrintVisitor::visit(const ReturnStmt& r) {
    printIndent();
    out << "return";
    if (!r.values.empty()) {
        out << ' ';
        for (size_t i = 0; i < r.values.size(); ++i) {
            if (i) out << ", ";
            r.values[i]->accept(*this);
        }
    }
    out << ";\n";
}

void PrintVisitor::visit(const VarDeclStmt& v) {
    printIndent();
    v.type->accept(*this);
    out << ' ' << v.name;
    if (v.initializer) {
        out << " = ";
        (*v.initializer)->accept(*this);
    }
    out << ";\n";
}

// Definitions
void PrintVisitor::visit(const FunctionDef& f) {
    printIndent();
    if (f.returnTypes.size() == 1) {
        f.returnTypes[0]->accept(*this);
    } else {
        out << '(';
        for (size_t i = 0; i < f.returnTypes.size(); ++i) {
            if (i) out << ", ";
            f.returnTypes[i]->accept(*this);
        }
        out << ')';
    }
    out << ' ' << f.name << '(';
    for (size_t i = 0; i < f.parameters.size(); ++i) {
        if (i) out << ", ";
        f.parameters[i].type->accept(*this);
        if (!f.parameters[i].name.empty())
            out << ' ' << f.parameters[i].name;
    }
    if (f.variadic) {
        if (!f.parameters.empty()) out << ", ";
        out << "...";
    }
    out << ')';
    if (f.body) {
        out << '\n';
        f.body->accept(*this);
    } else {
        out << ";\n";
    }
}

void PrintVisitor::visit(const GlobalVarDef& g) {
    printIndent();
    g.type->accept(*this);
    out << ' ' << g.name;
    if (g.initializer) {
        out << " = ";
        (*g.initializer)->accept(*this);
    }
    out << ";\n";
}

void PrintVisitor::visit(const StructDef& s) {
    printIndent();
    out << "struct " << s.name << " {\n";
    increaseIndent();
    for (const auto& [name, type] : s.fields) {
        printIndent();
        type->accept(*this);
        out << ' ' << name << ";\n";
    }
    decreaseIndent();
    printIndent();
    out << "};\n";
}

void PrintVisitor::visit(const EnumDef& e) {
    printIndent();
    out << "enum " << e.name << " {\n";
    increaseIndent();
    for (size_t i = 0; i < e.enumerators.size(); ++i) {
        printIndent();
        out << e.enumerators[i].first;
        if (e.enumerators[i].second)
            out << " = " << *e.enumerators[i].second;
        if (i != e.enumerators.size() - 1) out << ',';
        out << '\n';
    }
    decreaseIndent();
    printIndent();
    out << "};\n";
}

} // namespace Parsing