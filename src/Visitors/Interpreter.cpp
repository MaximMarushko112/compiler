#include "../../include/Visitors/Interpreter.hpp"
#include <iostream>

// #define DEBUG_INTERPRETER

#ifdef DEBUG_INTERPRETER
#define DEBUG_LOG(msg) std::cerr << "[DEBUG] " << msg << std::endl
#else
#define DEBUG_LOG(msg)
#endif

namespace Parsing {

Interpreter::Interpreter() : result(0), breakFlag(false), continueFlag(false), returnFlag(false), returnValue(0) {}

int Interpreter::getVariableValue(const std::string& name) {
    auto it = variables.find(name);
    if (it != variables.end()) return it->second;
    auto eit = enumConstants.find(name);
    if (eit != enumConstants.end()) return eit->second;
    throw std::runtime_error("Variable not declared: " + name);
}

void Interpreter::setVariableValue(const std::string& name, int value) {
    // Если переменная не объявлена, но это разрешено (например, для глобальных enum), просто создаём
    variables[name] = value;
}

void Interpreter::processEnum(const EnumDef& e) {
    int nextValue = 0;
    for (const auto& [name, optVal] : e.enumerators) {
        int val = optVal.value_or(nextValue);
        enumConstants[name] = val;
        nextValue = val + 1;
    }
}

int Interpreter::run(const TranslationUnit& tu) {
    DEBUG_LOG("=== Starting interpreter ===");
    // Сначала собираем все функции и глобальные enum-константы
    functions.clear();
    enumConstants.clear();
    for (const auto& def : tu.definitions) {
        if (auto f = dynamic_cast<const FunctionDef*>(def.get())) {
            functions[f->name] = f;
        } else if (auto e = dynamic_cast<const EnumDef*>(def.get())) {
            processEnum(*e);
        }
    }
    DEBUG_LOG("Found main function");
    // Находим main
    auto it = functions.find("main");
    if (it == functions.end()) {
        std::cerr << "No 'main' function found\n";
        return 0;
    }
    const FunctionDef* mainFunc = it->second;
    DEBUG_LOG("Executing body of main");
    // Выполняем тело main
    if (mainFunc->body) {
        mainFunc->body->accept(*this);
    }
    DEBUG_LOG("Interpreter result: " << returnValue);
    return returnValue;
}

// ---------------------------------------------------------------------
// Expressions
// ---------------------------------------------------------------------
void Interpreter::visit(const IntLiteral& lit) { result = lit.value; }
void Interpreter::visit(const FloatLiteral& lit) { result = static_cast<int>(lit.value); }
void Interpreter::visit(const StringLiteral&) { result = 0; }
void Interpreter::visit(const BoolLiteral& lit) { result = lit.value ? 1 : 0; }
void Interpreter::visit(const VariableExpr& var) { 
    result = getVariableValue(var.name);
    DEBUG_LOG("Read variable " << var.name << " = " << result);
}

void Interpreter::visit(const UnaryOp& u) {
    // Префиксный инкремент ++var (два унарных плюса)
    if (u.op == UnaryOp::Op::Plus) {
        if (auto inner = dynamic_cast<const UnaryOp*>(u.operand.get())) {
            if (inner->op == UnaryOp::Op::Plus) {
                if (auto var = dynamic_cast<const VariableExpr*>(inner->operand.get())) {
                    int v = getVariableValue(var->name);
                    DEBUG_LOG("Increment " << var->name << " from " << v << " to " << v + 1);
                    setVariableValue(var->name, v + 1);
                    result = v + 1;
                    return;
                }
            }
        }
        // Обычный унарный плюс
        u.operand->accept(*this);
        DEBUG_LOG("Unary plus on value " << result);
        return;
    }
    // Остальные унарные операции
    u.operand->accept(*this);
    switch (u.op) {
        case UnaryOp::Op::Not:   result = !result; break;
        case UnaryOp::Op::Minus: result = -result; break;
        default: throw std::runtime_error("Unsupported unary op");
    }
    DEBUG_LOG("Unary op result: " << result);
}

void Interpreter::visit(const BinaryOp& b) {
    int left, right;
    b.left->accept(*this);
    left = result;
    b.right->accept(*this);
    right = result;
    switch (b.op) {
        case BinaryOp::Op::Add: result = left + right; break;
        case BinaryOp::Op::Sub: result = left - right; break;
        case BinaryOp::Op::Mul: result = left * right; break;
        case BinaryOp::Op::Div: result = left / right; break;
        case BinaryOp::Op::Rem: result = left % right; break;
        case BinaryOp::Op::Less:      result = left < right; break;
        case BinaryOp::Op::LessEqual: 
          DEBUG_LOG("Comparing " << left << " <= " << right);
          result = left <= right;
          DEBUG_LOG("Result: " << result);
          break;
        case BinaryOp::Op::Greater:   result = left > right; break;
        case BinaryOp::Op::GreaterEqual: result = left >= right; break;
        case BinaryOp::Op::Equal:      result = left == right; break;
        case BinaryOp::Op::NotEqual:   result = left != right; break;
        case BinaryOp::Op::LogicalAnd: result = left && right; break;
        case BinaryOp::Op::LogicalOr:  result = left || right; break;
        default: throw std::runtime_error("Unsupported binary op");
    }
}

void Interpreter::visit(const AssignExpr& a) {
    a.right->accept(*this);
    int val = result;
    if (auto var = dynamic_cast<const VariableExpr*>(a.left.get())) {
        DEBUG_LOG("Assign " << var->name << " = " << val);
        setVariableValue(var->name, val);
        result = val;
    } else {
        throw std::runtime_error("Assignment to non-variable");
    }
}

void Interpreter::visit(const InitListExpr&) { result = 0; }

void Interpreter::visit(const MultiAssignExpr& m) {
    DEBUG_LOG("MultiAssignExpr: lefts size=" << m.lefts.size() << ", rights size=" << m.rights.size());
    std::vector<int> rightValues;
    for (const auto& e : m.rights) {
        e->accept(*this);
        rightValues.push_back(result);
        DEBUG_LOG("  right value = " << result);
    }
    for (size_t i = 0; i < m.lefts.size() && i < rightValues.size(); ++i) {
        if (auto var = dynamic_cast<const VariableExpr*>(m.lefts[i].get())) {
            DEBUG_LOG("  assigning " << var->name << " = " << rightValues[i]);
            setVariableValue(var->name, rightValues[i]);
        } else {
            throw std::runtime_error("MultiAssign: left side is not a variable");
        }
    }
    result = rightValues.empty() ? 0 : rightValues.back();
    DEBUG_LOG("MultiAssignExpr result = " << result);
}

void Interpreter::visit(const ConditionalExpr& c) {
    c.cond->accept(*this);
    if (result) c.thenExpr->accept(*this);
    else c.elseExpr->accept(*this);
}

void Interpreter::visit(const CallExpr& c) {
    if (auto var = dynamic_cast<const VariableExpr*>(c.callee.get())) {
        const std::string& funcName = var->name;
        if (funcName == "print") {
            if (c.arguments.size() == 1) {
                c.arguments[0]->accept(*this);
                std::cout << result << std::endl;
                result = 0;
                return;
            }
            throw std::runtime_error("print expects 1 argument");
        }
        auto fit = functions.find(funcName);
        if (fit == functions.end())
            throw std::runtime_error("Unknown function: " + funcName);
        const FunctionDef* func = fit->second;
        if (c.arguments.size() != func->parameters.size())
            throw std::runtime_error("Argument count mismatch for " + funcName);
        auto oldVars = std::move(variables);
        variables.clear();
        for (size_t i = 0; i < c.arguments.size(); ++i) {
            c.arguments[i]->accept(*this);
            variables[func->parameters[i].name] = result;
        }
        // Сохраняем текущее состояние флагов
        bool oldReturnFlag = returnFlag;
        int oldReturnValue = returnValue;
        // Сбрасываем перед вызовом функции
        returnFlag = false;
        returnValue = 0;
        if (func->body) {
            func->body->accept(*this);
        }
        // Результат функции – из returnValue
        result = returnValue;
        // Восстанавливаем флаги
        returnFlag = oldReturnFlag;
        returnValue = oldReturnValue;
        variables = std::move(oldVars);
        return;
    }
    throw std::runtime_error("Call to non-identifier expression");
}

void Interpreter::visit(const CastExpr& c) {
    c.operand->accept(*this);
}

void Interpreter::visit(const SizeofExpr&) {
    result = 4; // упрощённо
}

// ---------------------------------------------------------------------
// Statements
// ---------------------------------------------------------------------
void Interpreter::visit(const ExprStmt& e) { e.expr->accept(*this); }
void Interpreter::visit(const BlockStmt& b) { executeBlock(b.statements); }

void Interpreter::visit(const IfStmt& i) {
    i.condition->accept(*this);
    if (result) i.thenStmt->accept(*this);
    else if (i.elseStmt) (*i.elseStmt)->accept(*this);
}

void Interpreter::visit(const WhileStmt& w) {
    while (true) {
        w.condition->accept(*this);
        if (!result) break;
        w.body->accept(*this);
        if (breakFlag) { breakFlag = false; break; }
        if (returnFlag) break;
        if (continueFlag) { continueFlag = false; continue; }
    }
    continueFlag = false;
}

void Interpreter::visit(const DoWhileStmt& d) {
    do {
        d.body->accept(*this);
        if (breakFlag) { breakFlag = false; break; }
        if (returnFlag) break;
        if (continueFlag) { continueFlag = false; continue; }
        d.condition->accept(*this);
    } while (result);
    continueFlag = false;
}

void Interpreter::visit(const ForStmt& f) {
    DEBUG_LOG("=== Entering for loop ===");
    if (f.init) {
        DEBUG_LOG("Processing init");
        (*f.init)->accept(*this);
    }
    while (true) {
        if (f.condition) {
            DEBUG_LOG("Evaluating condition");
            (*f.condition)->accept(*this);
            DEBUG_LOG("Condition result: " << result);
            if (!result) break;
        }
        DEBUG_LOG("Executing body");
        f.body->accept(*this);
        if (breakFlag) { breakFlag = false; break; }
        if (returnFlag) break;
        if (continueFlag) { continueFlag = false; continue; }
        if (f.increment) {
            DEBUG_LOG("Processing increment");
            if (auto unary = dynamic_cast<const UnaryOp*>(f.increment->get())) {
                if (unary->op == UnaryOp::Op::Plus) {
                    if (auto inner = dynamic_cast<const UnaryOp*>(unary->operand.get())) {
                        if (inner->op == UnaryOp::Op::Plus) {
                            if (auto var = dynamic_cast<const VariableExpr*>(inner->operand.get())) {
                                int v = getVariableValue(var->name);
                                DEBUG_LOG("Increment " << var->name << " from " << v << " to " << v + 1);
                                setVariableValue(var->name, v + 1);
                            }
                        }
                    }
                } else {
                    (*f.increment)->accept(*this);
                }
            } else {
                (*f.increment)->accept(*this);
            }
        }
        DEBUG_LOG("--- End of iteration ---");
    }
    DEBUG_LOG("=== Exiting for loop ===");
}

void Interpreter::visit(const BreakStmt&) { breakFlag = true; }
void Interpreter::visit(const ContinueStmt&) { continueFlag = true; }

void Interpreter::visit(const ReturnStmt& r) {
    DEBUG_LOG("ReturnStmt: values count=" << r.values.size());
    if (r.values.empty()) {
        returnValue = 0;
    } else {
        r.values[0]->accept(*this);
        returnValue = result;
        DEBUG_LOG("  return value = " << returnValue);
    }
    returnFlag = true;
}

void Interpreter::visit(const VarDeclStmt& v) {
    int initVal = 0;
    if (v.initializer) {
        if (dynamic_cast<const InitListExpr*>(v.initializer->get())) {
            initVal = 0;
        } else {
            (*v.initializer)->accept(*this);
            initVal = result;
        }
    }
    setVariableValue(v.name, initVal);
}

// ---------------------------------------------------------------------
// Definitions
// ---------------------------------------------------------------------
void Interpreter::visit(const FunctionDef&) {} // уже обработаны в run
void Interpreter::visit(const GlobalVarDef& g) {
    int initVal = 0;
    if (g.initializer) {
        (*g.initializer)->accept(*this);
        initVal = result;
    }
    setVariableValue(g.name, initVal);
}
void Interpreter::visit(const EnumDef& e) { processEnum(e); }

void Interpreter::evaluate(Expr* e) { e->accept(*this); }
void Interpreter::execute(Stmt* s) { s->accept(*this); }
void Interpreter::executeBlock(const std::vector<std::unique_ptr<Stmt>>& stmts) {
    for (const auto& stmt : stmts) {
        stmt->accept(*this);
        if (breakFlag || continueFlag || returnFlag) break;
    }
}

} // namespace Parsing