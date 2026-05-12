#include "../../include/Parsing/PrintAST.hpp"
#include "../../include/Parsing/ASTNodes.hpp"
#include <memory>

namespace Parsing {

namespace {
    std::string indentStr(int indent) {
        return std::string(indent * 2, ' ');
    }
}

// ---------------------------------------------------------------------
// PrintVisitor – конкретный посетитель для печати AST
// ---------------------------------------------------------------------
class PrintVisitor : public ASTVisitor {
    std::ostream& out;
    int indent;

public:
    PrintVisitor(std::ostream& os, int ind) : out(os), indent(ind) {}

    void printIndent() { out << indentStr(indent); }
    void increaseIndent() { indent++; }
    void decreaseIndent() { indent--; }

    // -----------------------------------------------------------------
    // Types
    // -----------------------------------------------------------------
    void visit(const VoidType&) override { out << "void"; }

    void visit(const IntType&) override { out << "int"; }

    void visit(const UnsignedType&) override { out << "unsigned"; }

    void visit(const FloatType&) override { out << "float"; }

    void visit(const BoolType&) override { out << "bool"; }

    void visit(const StringType&) override { out << "string"; }

    void visit(const NamedType& n) override { out << n.name; }

    void visit(const PointerType& p) override {
        p.pointee->accept(*this);
        out << "*";
    }

    void visit(const ArrayType& a) override {
        a.elementType->accept(*this);
        out << "[";
        if (a.size) out << *a.size;
        out << "]";
    }

    void visit(const FunctionType& f) override {
        out << "(";
        for (size_t i = 0; i < f.parameterTypes.size(); ++i) {
            if (i) out << ", ";
            f.parameterTypes[i]->accept(*this);
        }
        if (f.variadic) out << ", ...";
        out << ") -> ";
        if (f.returnTypes.size() == 1) {
            f.returnTypes[0]->accept(*this);
        } else {
            out << "(";
            for (size_t i = 0; i < f.returnTypes.size(); ++i) {
                if (i) out << ", ";
                f.returnTypes[i]->accept(*this);
            }
            out << ")";
        }
    }

    void visit(const TupleType& t) override {
        out << "(";
        for (size_t i = 0; i < t.types.size(); ++i) {
            if (i) out << ", ";
            t.types[i]->accept(*this);
        }
        out << ")";
    }

    // -----------------------------------------------------------------
    // Expressions
    // -----------------------------------------------------------------
    void visit(const IntLiteral& lit) override { out << lit.value; }

    void visit(const FloatLiteral& lit) override { out << lit.value; }

    void visit(const StringLiteral& lit) override { out << "\"" << lit.value << "\""; }

    void visit(const BoolLiteral& lit) override { out << (lit.value ? "true" : "false"); }

    void visit(const VariableExpr& var) override { out << var.name; }

    void visit(const UnaryOp& u) override {
        switch (u.op) {
            case UnaryOp::Op::Not: out << "!"; break;
            case UnaryOp::Op::Minus: out << "-"; break;
            case UnaryOp::Op::Plus: out << "+"; break;
            case UnaryOp::Op::AddressOf: out << "&"; break;
            case UnaryOp::Op::Dereference: out << "*"; break;
            default: out << "?";
        }
        u.operand->accept(*this);
    }

    void visit(const BinaryOp& b) override {
        out << "(";
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
        out << ")";
    }

    void visit(const AssignExpr& a) override {
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

    void visit(const InitListExpr& l) override {
        out << "{ ";
        for (size_t i = 0; i < l.values.size(); ++i) {
            if (i) out << ", ";
            l.values[i]->accept(*this);
        }
        out << " }";
    }

    void visit(const MultiAssignExpr& m) override {
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

    void visit(const ConditionalExpr& c) override {
        c.cond->accept(*this);
        out << " ? ";
        c.thenExpr->accept(*this);
        out << " : ";
        c.elseExpr->accept(*this);
    }

    void visit(const CallExpr& c) override {
        c.callee->accept(*this);
        out << "(";
        for (size_t i = 0; i < c.arguments.size(); ++i) {
            if (i) out << ", ";
            c.arguments[i]->accept(*this);
        }
        out << ")";
    }

    void visit(const FieldAccessExpr& f) override {
        f.object->accept(*this);
        out << (f.isArrow ? "->" : ".") << f.field;
    }

    void visit(const IndexExpr& i) override {
        i.array->accept(*this);
        out << "[";
        i.index->accept(*this);
        out << "]";
    }

    void visit(const CastExpr& c) override {
        out << "(";
        c.targetType->accept(*this);
        out << ")";
        c.operand->accept(*this);
    }

    void visit(const SizeofExpr& s) override {
        out << "sizeof ";
        if (std::holds_alternative<std::unique_ptr<Type>>(s.operand)) {
            out << "(";
            std::get<std::unique_ptr<Type>>(s.operand)->accept(*this);
            out << ")";
        } else {
            std::get<std::unique_ptr<Expr>>(s.operand)->accept(*this);
        }
    }

    // -----------------------------------------------------------------
    // Statements
    // -----------------------------------------------------------------
    void visit(const ExprStmt& e) override {
        printIndent();
        e.expr->accept(*this);
        out << ";\n";
    }

    void visit(const BlockStmt& b) override {
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

    void visit(const IfStmt& i) override {
        printIndent();
        out << "if (";
        i.condition->accept(*this);
        out << ") ";
        if (dynamic_cast<const BlockStmt*>(i.thenStmt.get())) {
            out << "\n";
            i.thenStmt->accept(*this);
        } else {
            out << "\n";
            increaseIndent();
            i.thenStmt->accept(*this);
            decreaseIndent();
        }
        if (i.elseStmt) {
            printIndent();
            out << "else ";
            if (dynamic_cast<const BlockStmt*>((*i.elseStmt).get())) {
                out << "\n";
                (*i.elseStmt)->accept(*this);
            } else {
                out << "\n";
                increaseIndent();
                (*i.elseStmt)->accept(*this);
                decreaseIndent();
            }
        }
    }

    void visit(const WhileStmt& w) override {
        printIndent();
        out << "while (";
        w.condition->accept(*this);
        out << ") ";
        w.body->accept(*this);
    }

    void visit(const DoWhileStmt& d) override {
        printIndent();
        out << "do ";
        d.body->accept(*this);
        printIndent();
        out << "while (";
        d.condition->accept(*this);
        out << ");\n";
    }

    void visit(const ForStmt& f) override {
        printIndent();
        out << "for (";
        if (f.init) {
            if (auto v = dynamic_cast<const VarDeclStmt*>(f.init->get())) {
                v->type->accept(*this);
                out << " " << v->name;
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
        out << ") ";
        f.body->accept(*this);
    }

    void visit(const SwitchStmt& s) override {
        printIndent();
        out << "switch (";
        s.control->accept(*this);
        out << ") ";
        s.body->accept(*this);
    }

    void visit(const CaseStmt& c) override {
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

    void visit(const BreakStmt&) override {
        printIndent();
        out << "break;\n";
    }

    void visit(const ContinueStmt&) override {
        printIndent();
        out << "continue;\n";
    }

    void visit(const ReturnStmt& r) override {
        printIndent();
        out << "return";
        if (!r.values.empty()) {
            out << " ";
            for (size_t i = 0; i < r.values.size(); ++i) {
                if (i) out << ", ";
                r.values[i]->accept(*this);
            }
        }
        out << ";\n";
    }

    void visit(const GotoStmt& g) override {
        printIndent();
        out << "goto " << g.label << ";\n";
    }

    void visit(const LabelStmt& l) override {
        printIndent();
        out << l.name << ":\n";
        increaseIndent();
        l.statement->accept(*this);
        decreaseIndent();
    }

    void visit(const VarDeclStmt& v) override {
        printIndent();
        v.type->accept(*this);
        out << " " << v.name;
        if (v.initializer) {
            out << " = ";
            (*v.initializer)->accept(*this);
        }
        out << ";\n";
    }

    // -----------------------------------------------------------------
    // Definitions
    // -----------------------------------------------------------------
    void visit(const FunctionDef& f) override {
        printIndent();
        if (f.returnTypes.size() == 1) {
            f.returnTypes[0]->accept(*this);
        } else {
            out << "(";
            for (size_t i = 0; i < f.returnTypes.size(); ++i) {
                if (i) out << ", ";
                f.returnTypes[i]->accept(*this);
            }
            out << ")";
        }
        out << " " << f.name << "(";
        for (size_t i = 0; i < f.parameters.size(); ++i) {
            if (i) out << ", ";
            f.parameters[i].type->accept(*this);
            if (!f.parameters[i].name.empty()) out << " " << f.parameters[i].name;
        }
        if (f.variadic) {
            if (!f.parameters.empty()) out << ", ";
            out << "...";
        }
        out << ")";
        if (f.body) {
            out << " ";
            increaseIndent();
            f.body->accept(*this);
            decreaseIndent();
        } else {
            out << ";\n";
        }
    }

    void visit(const GlobalVarDef& g) override {
        printIndent();
        g.type->accept(*this);
        out << " " << g.name;
        if (g.initializer) {
            out << " = ";
            (*g.initializer)->accept(*this);
        }
        out << ";\n";
    }

    void visit(const StructDef& s) override {
        printIndent();
        out << "struct " << s.name << " {\n";
        increaseIndent();
        for (const auto& [name, type] : s.fields) {
            printIndent();
            type->accept(*this);
            out << " " << name << ";\n";
        }
        decreaseIndent();
        printIndent();
        out << "};\n";
    }

    void visit(const EnumDef& e) override {
        printIndent();
        out << "enum " << e.name << " {\n";
        increaseIndent();
        for (size_t i = 0; i < e.enumerators.size(); ++i) {
            printIndent();
            out << e.enumerators[i].first;
            if (e.enumerators[i].second) out << " = " << *e.enumerators[i].second;
            if (i != e.enumerators.size() - 1) out << ",";
            out << "\n";
        }
        decreaseIndent();
        printIndent();
        out << "};\n";
    }
};

// ---------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------
void printAST(const TranslationUnit& tu, std::ostream& out, int indent) {
    PrintVisitor visitor(out, indent);
    for (const auto& def : tu.definitions) {
        def->accept(visitor);
        out << "\n";
    }
}

} // namespace Parsing