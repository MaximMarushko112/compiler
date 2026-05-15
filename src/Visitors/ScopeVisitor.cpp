#include "../../include/Visitors/ScopeVisitor.hpp"
#include <iostream>

namespace Parsing {

ScopeVisitor::ScopeVisitor() : currentScope(&globalScope) {}

void ScopeVisitor::build(const TranslationUnit& tu) {
    // Добавляем встроенную функцию print
    auto printType = std::make_unique<FunctionType>();
    printType->parameterTypes.push_back(std::make_unique<IntType>());
    printType->returnTypes.push_back(std::make_unique<VoidType>());
    globalScope.symbols["print"] = Symbol{Symbol::Kind::Function, "print", std::move(printType)};
    
    for (const auto& def : tu.definitions) {
        def->accept(*this);
    }
}

void ScopeVisitor::printErrors() const {
    for (const auto& e : errors) {
        std::cerr << "Scope error: " << e << std::endl;
    }
}

void ScopeVisitor::enterScope() {
    auto newScope = std::make_unique<Scope>();
    newScope->parent = currentScope;
    currentScope->children.push_back(std::move(newScope));
    currentScope = currentScope->children.back().get();
}

void ScopeVisitor::exitScope() {
    if (currentScope->parent)
        currentScope = currentScope->parent;
}

void ScopeVisitor::addSymbol(const std::string& name, Symbol::Kind kind, std::unique_ptr<Type> type) {
    auto it = currentScope->symbols.find(name);
    if (it != currentScope->symbols.end()) {
        error("Redeclaration of '" + name + "' in same scope");
        return;
    }
    Symbol sym{kind, name, std::move(type)};
    currentScope->symbols[name] = std::move(sym);
}

Symbol* ScopeVisitor::lookupSymbol(const std::string& name, bool currentOnly) {
    Scope* sc = currentScope;
    while (sc) {
        auto it = sc->symbols.find(name);
        if (it != sc->symbols.end())
            return &it->second;
        if (currentOnly) break;
        sc = sc->parent;
    }
    return nullptr;
}

void ScopeVisitor::error(const std::string& msg) {
    errors.push_back(msg);
}

// ---------------------------------------------------------------------
// Expressions – рекурсивный обход
// ---------------------------------------------------------------------
void ScopeVisitor::visit(const IntLiteral&) {}
void ScopeVisitor::visit(const FloatLiteral&) {}
void ScopeVisitor::visit(const StringLiteral&) {}
void ScopeVisitor::visit(const BoolLiteral&) {}

void ScopeVisitor::visit(const VariableExpr& var) {
    if (!lookupSymbol(var.name)) {
        error("Use of undeclared variable '" + var.name + "'");
    }
}

void ScopeVisitor::visit(const UnaryOp& u) {
    u.operand->accept(*this);
}

void ScopeVisitor::visit(const BinaryOp& b) {
    b.left->accept(*this);
    b.right->accept(*this);
}

void ScopeVisitor::visit(const AssignExpr& a) {
    a.left->accept(*this);
    a.right->accept(*this);
}

void ScopeVisitor::visit(const InitListExpr& l) {
    for (const auto& val : l.values) {
        val->accept(*this);
    }
}

void ScopeVisitor::visit(const MultiAssignExpr& m) {
    for (const auto& left : m.lefts) left->accept(*this);
    for (const auto& right : m.rights) right->accept(*this);
}

void ScopeVisitor::visit(const ConditionalExpr& c) {
    c.cond->accept(*this);
    c.thenExpr->accept(*this);
    c.elseExpr->accept(*this);
}

void ScopeVisitor::visit(const CallExpr& call) {
    call.callee->accept(*this);
    for (const auto& arg : call.arguments) {
        arg->accept(*this);
    }
    // Дополнительная проверка, что callee – функция
    if (auto var = dynamic_cast<const VariableExpr*>(call.callee.get())) {
        Symbol* sym = lookupSymbol(var->name);
        if (!sym) {
            error("Call to undeclared function '" + var->name + "'");
        } else if (sym->kind != Symbol::Kind::Function) {
            error("'" + var->name + "' is not a function");
        }
    } else {
        error("Call to non-identifier expression");
    }
}

void ScopeVisitor::visit(const FieldAccessExpr& f) {
    f.object->accept(*this);
    // поле не проверяется, так как его тип уже известен из структуры
}

void ScopeVisitor::visit(const IndexExpr& i) {
    i.array->accept(*this);
    i.index->accept(*this);
}

void ScopeVisitor::visit(const CastExpr& c) {
    c.targetType->accept(*this);
    c.operand->accept(*this);
}

void ScopeVisitor::visit(const SizeofExpr& s) {
    if (std::holds_alternative<std::unique_ptr<Type>>(s.operand)) {
        std::get<std::unique_ptr<Type>>(s.operand)->accept(*this);
    } else {
        std::get<std::unique_ptr<Expr>>(s.operand)->accept(*this);
    }
}

// ---------------------------------------------------------------------
// Statements
// ---------------------------------------------------------------------
void ScopeVisitor::visit(const ExprStmt& e) {
    e.expr->accept(*this);
}

void ScopeVisitor::visit(const BlockStmt& block) {
    enterScope();
    for (const auto& stmt : block.statements) {
        stmt->accept(*this);
    }
    exitScope();
}

void ScopeVisitor::visit(const IfStmt& ifs) {
    ifs.condition->accept(*this);
    ifs.thenStmt->accept(*this);
    if (ifs.elseStmt) (*ifs.elseStmt)->accept(*this);
}

void ScopeVisitor::visit(const WhileStmt& w) {
    w.condition->accept(*this);
    w.body->accept(*this);
}

void ScopeVisitor::visit(const DoWhileStmt& d) {
    d.body->accept(*this);
    d.condition->accept(*this);
}

void ScopeVisitor::visit(const ForStmt& f) {
    enterScope();
    if (f.init) (*f.init)->accept(*this);
    if (f.condition) (*f.condition)->accept(*this);
    if (f.increment) (*f.increment)->accept(*this);
    f.body->accept(*this);
    exitScope();
}

void ScopeVisitor::visit(const SwitchStmt& s) {
    s.control->accept(*this);
    s.body->accept(*this);
}

void ScopeVisitor::visit(const CaseStmt& c) {
    if (auto label = std::get_if<CaseLabel>(&c.label)) {
        label->value->accept(*this);
    }
    c.statement->accept(*this);
}

void ScopeVisitor::visit(const BreakStmt&) {}
void ScopeVisitor::visit(const ContinueStmt&) {}
void ScopeVisitor::visit(const ReturnStmt& r) {
    for (const auto& val : r.values) {
        val->accept(*this);
    }
}
void ScopeVisitor::visit(const GotoStmt&) {}
void ScopeVisitor::visit(const LabelStmt& l) {
    l.statement->accept(*this);
}

void ScopeVisitor::visit(const VarDeclStmt& v) {
    if (lookupSymbol(v.name, true)) {
        error("Variable '" + v.name + "' already declared in this scope");
    } else {
        addSymbol(v.name, Symbol::Kind::Variable, std::unique_ptr<Type>(v.type->clone()));
    }
    if (v.initializer) (*v.initializer)->accept(*this);
}

// ---------------------------------------------------------------------
// Definitions
// ---------------------------------------------------------------------
void ScopeVisitor::visit(const FunctionDef& f) {
    if (lookupSymbol(f.name, true)) {
        error("Function '" + f.name + "' already declared");
    } else {
        auto funcType = std::make_unique<FunctionType>();
        for (const auto& p : f.parameters) {
            funcType->parameterTypes.push_back(std::unique_ptr<Type>(p.type->clone()));
        }
        for (const auto& rt : f.returnTypes) {
            funcType->returnTypes.push_back(std::unique_ptr<Type>(rt->clone()));
        }
        funcType->variadic = f.variadic;
        addSymbol(f.name, Symbol::Kind::Function, std::move(funcType));
    }
    enterScope();
    for (const auto& p : f.parameters) {
        addSymbol(p.name, Symbol::Kind::Variable, std::unique_ptr<Type>(p.type->clone()));
    }
    if (f.body) f.body->accept(*this);
    exitScope();
}

void ScopeVisitor::visit(const GlobalVarDef& g) {
    if (lookupSymbol(g.name, true)) {
        error("Global variable '" + g.name + "' already declared");
    } else {
        addSymbol(g.name, Symbol::Kind::Variable, std::unique_ptr<Type>(g.type->clone()));
        if (g.initializer) (*g.initializer)->accept(*this);
    }
}

void ScopeVisitor::visit(const StructDef& s) {
    if (lookupSymbol(s.name, true)) {
        error("Struct '" + s.name + "' already declared");
    } else {
        auto structType = std::make_unique<NamedType>(s.name);
        addSymbol(s.name, Symbol::Kind::Type, std::move(structType));
        enterScope();
        for (const auto& [fname, ftype] : s.fields) {
            addSymbol(fname, Symbol::Kind::Variable, std::unique_ptr<Type>(ftype->clone()));
        }
        exitScope();
    }
}

void ScopeVisitor::visit(const EnumDef& e) {
    if (lookupSymbol(e.name, true)) {
        error("Enum '" + e.name + "' already declared");
    } else {
        addSymbol(e.name, Symbol::Kind::Type, std::make_unique<NamedType>(e.name));
        for (const auto& [ename, optVal] : e.enumerators) {
            if (lookupSymbol(ename, true)) {
                error("Enumerator '" + ename + "' already declared in this enum");
            } else {
                addSymbol(ename, Symbol::Kind::Variable, std::make_unique<IntType>());
            }
        }
    }
}

} // namespace Parsing