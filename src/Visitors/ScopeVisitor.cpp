#include "../../include/Visitors/ScopeVisitor.hpp"
#include <iostream>

namespace Parsing {

// ---------------------------------------------------------------------
// Вспомогательная функция для сравнения типов
// ---------------------------------------------------------------------
static bool isTypeCompatible(const Type* a, const Type* b) {
    if (!a || !b) return false;
    if (dynamic_cast<const IntType*>(a) && dynamic_cast<const IntType*>(b)) return true;
    if (dynamic_cast<const FloatType*>(a) && dynamic_cast<const FloatType*>(b)) return true;
    if (dynamic_cast<const BoolType*>(a) && dynamic_cast<const BoolType*>(b)) return true;
    if (dynamic_cast<const StringType*>(a) && dynamic_cast<const StringType*>(b)) return true;
    if (dynamic_cast<const VoidType*>(a) && dynamic_cast<const VoidType*>(b)) return true;
    if (auto na = dynamic_cast<const NamedType*>(a)) {
        if (auto nb = dynamic_cast<const NamedType*>(b)) {
            return na->name == nb->name;
        }
    }
    return false;
}

// ---------------------------------------------------------------------
// Реализация методов Scope
// ---------------------------------------------------------------------
void Scope::addSymbol(const std::string& name, Symbol sym) {
    auto it = symbols.find(name);
    if (it != symbols.end()) {
        return;
    }
    symbols[name] = std::move(sym);
}

Symbol* Scope::lookupSymbol(const std::string& name, bool currentOnly) {
    Scope* sc = this;
    while (sc) {
        auto it = sc->symbols.find(name);
        if (it != sc->symbols.end())
            return &it->second;
        if (currentOnly) break;
        sc = sc->parent;
    }
    return nullptr;
}

// ---------------------------------------------------------------------
// ScopeVisitor
// ---------------------------------------------------------------------
ScopeVisitor::ScopeVisitor() : currentScope(&globalScope), currentFuncReturnTypes(nullptr) {}

void ScopeVisitor::build(const TranslationUnit& tu) {
    FunctionInfo printInfo;
    printInfo.paramTypes.push_back(std::make_unique<IntType>());
    printInfo.returnTypes.push_back(std::make_unique<VoidType>());
    globalScope.addSymbol("print", std::move(printInfo));
    
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

void ScopeVisitor::addSymbol(const std::string& name, Symbol sym) {
    if (currentScope->lookupSymbol(name, true)) {
        error("Redeclaration of '" + name + "' in same scope");
        return;
    }
    currentScope->addSymbol(name, std::move(sym));
}

Symbol* ScopeVisitor::lookupSymbol(const std::string& name, bool currentOnly) {
    return currentScope->lookupSymbol(name, currentOnly);
}

void ScopeVisitor::error(const std::string& msg) {
    errors.push_back(msg);
}

// ---------------------------------------------------------------------
// Получение типа выражения
// ---------------------------------------------------------------------
std::unique_ptr<Type> ScopeVisitor::getExprType(const Expr* expr) {
    if (!expr) return nullptr;

    // Пробуем получить тип через виртуальный метод getType()
    auto directType = expr->getType();
    if (directType) {
        return directType;
    }

    // Для выражений, которые не могут определить тип самостоятельно (переменные, вызовы функций),
    // используем таблицу символов.
    if (auto var = dynamic_cast<const VariableExpr*>(expr)) {
        Symbol* sym = lookupSymbol(var->name);
        if (sym && std::holds_alternative<VariableInfo>(*sym)) {
            const VariableInfo& info = std::get<VariableInfo>(*sym);
            return info.type->clone();
        }
    }
    if (auto call = dynamic_cast<const CallExpr*>(expr)) {
        Symbol* sym = lookupSymbol(call->functionName);
        if (sym && std::holds_alternative<FunctionInfo>(*sym)) {
            const FunctionInfo& info = std::get<FunctionInfo>(*sym);
            if (!info.returnTypes.empty()) {
                return info.returnTypes[0]->clone();
            }
        }
    }
    return nullptr;
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
    auto leftType = getExprType(a.left.get());
    auto rightType = getExprType(a.right.get());
    if (leftType && rightType && !isTypeCompatible(leftType.get(), rightType.get())) {
        error("Type mismatch in assignment");
    }
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
    Symbol* sym = lookupSymbol(call.functionName);
    if (!sym) {
        error("Call to undeclared function '" + call.functionName + "'");
        return;
    }
    if (!std::holds_alternative<FunctionInfo>(*sym)) {
        error("'" + call.functionName + "' is not a function");
        return;
    }
    const FunctionInfo& funcInfo = std::get<FunctionInfo>(*sym);
    size_t expected = funcInfo.paramTypes.size();
    size_t given = call.arguments.size();
    if (funcInfo.variadic) {
        if (given < expected) {
            error("Too few arguments for function '" + call.functionName + "'");
        }
    } else {
        if (given != expected) {
            error("Wrong number of arguments for function '" + call.functionName +
                  " (expected " + std::to_string(expected) + ", got " + std::to_string(given) + ")");
        }
    }
    for (size_t i = 0; i < std::min(expected, given); ++i) {
        auto argType = getExprType(call.arguments[i].get());
        if (argType && !isTypeCompatible(argType.get(), funcInfo.paramTypes[i].get())) {
            error("Type mismatch in argument " + std::to_string(i+1) + " of function '" + call.functionName + "'");
        }
    }
    for (const auto& arg : call.arguments) {
        arg->accept(*this);
    }
}

void ScopeVisitor::visit(const FieldAccessExpr& f) {
    f.object->accept(*this);
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

void ScopeVisitor::visit(const PreIncrement& e) { e.operand->accept(*this); }
void ScopeVisitor::visit(const PostIncrement& e) { e.operand->accept(*this); }
void ScopeVisitor::visit(const PreDecrement& e) { e.operand->accept(*this); }
void ScopeVisitor::visit(const PostDecrement& e) { e.operand->accept(*this); }

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
    if (currentFuncReturnTypes == nullptr) {
        error("Return statement outside function");
        return;
    }
    if (r.values.size() != currentFuncReturnTypes->size()) {
        error("Wrong number of return values (expected " + std::to_string(currentFuncReturnTypes->size()) +
              ", got " + std::to_string(r.values.size()) + ")");
    }
    for (size_t i = 0; i < r.values.size() && i < currentFuncReturnTypes->size(); ++i) {
        auto valType = getExprType(r.values[i].get());
        if (valType && !isTypeCompatible(valType.get(), (*currentFuncReturnTypes)[i].get())) {
            error("Type mismatch in return value " + std::to_string(i+1));
        }
    }
    for (const auto& val : r.values) {
        val->accept(*this);
    }
}

void ScopeVisitor::visit(const VarDeclStmt& v) {
    if (lookupSymbol(v.name, true)) {
        error("Variable '" + v.name + "' already declared in this scope");
    } else {
        VariableInfo varInfo;
        varInfo.type = v.type->clone();
        addSymbol(v.name, std::move(varInfo));
        if (v.initializer) {
            auto initType = getExprType(v.initializer->get());
            if (initType && !isTypeCompatible(initType.get(), v.type.get())) {
                error("Initializer type mismatch for variable '" + v.name + "'");
            }
            (*v.initializer)->accept(*this);
        }
    }
}

// ---------------------------------------------------------------------
// Definitions
// ---------------------------------------------------------------------
void ScopeVisitor::visit(const FunctionDef& f) {
    if (lookupSymbol(f.name, true)) {
        error("Function '" + f.name + "' already declared");
    } else {
        FunctionInfo funcInfo;
        for (const auto& p : f.parameters) {
            funcInfo.paramTypes.push_back(p.type->clone());
        }
        for (const auto& rt : f.returnTypes) {
            funcInfo.returnTypes.push_back(rt->clone());
        }
        funcInfo.variadic = f.variadic;
        addSymbol(f.name, std::move(funcInfo));
    }
    enterScope();
    for (const auto& p : f.parameters) {
        VariableInfo varInfo;
        varInfo.type = p.type->clone();
        addSymbol(p.name, std::move(varInfo));
    }
    auto oldReturnTypes = currentFuncReturnTypes;
    currentFuncReturnTypes = &f.returnTypes;
    if (f.body) f.body->accept(*this);
    currentFuncReturnTypes = oldReturnTypes;
    exitScope();
}

void ScopeVisitor::visit(const GlobalVarDef& g) {
    if (lookupSymbol(g.name, true)) {
        error("Global variable '" + g.name + "' already declared");
    } else {
        VariableInfo varInfo;
        varInfo.type = g.type->clone();
        addSymbol(g.name, std::move(varInfo));
        if (g.initializer) {
            auto initType = getExprType(g.initializer->get());
            if (initType && !isTypeCompatible(initType.get(), g.type.get())) {
                error("Initializer type mismatch for global variable '" + g.name + "'");
            }
            (*g.initializer)->accept(*this);
        }
    }
}

void ScopeVisitor::visit(const StructDef& s) {
    if (lookupSymbol(s.name, true)) {
        error("Struct '" + s.name + "' already declared");
    } else {
        TypeInfo typeInfo;
        typeInfo.type = std::make_unique<NamedType>(s.name);
        addSymbol(s.name, std::move(typeInfo));
        enterScope();
        for (const auto& [fname, ftype] : s.fields) {
            VariableInfo varInfo;
            varInfo.type = ftype->clone();
            addSymbol(fname, std::move(varInfo));
        }
        exitScope();
    }
}

void ScopeVisitor::visit(const EnumDef& e) {
    if (lookupSymbol(e.name, true)) {
        error("Enum '" + e.name + "' already declared");
    } else {
        TypeInfo typeInfo;
        typeInfo.type = std::make_unique<NamedType>(e.name);
        addSymbol(e.name, std::move(typeInfo));
        for (const auto& [ename, optVal] : e.enumerators) {
            if (lookupSymbol(ename, true)) {
                error("Enumerator '" + ename + "' already declared in this enum");
            } else {
                VariableInfo varInfo;
                varInfo.type = std::make_unique<IntType>();
                addSymbol(ename, std::move(varInfo));
            }
        }
    }
}

} // namespace Parsing