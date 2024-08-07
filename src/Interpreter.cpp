#include <iostream>

#include "../include/Environment.h"
#include "../include/Interpreter.h"
#include "../include/Lox.h"
#include "../include/LoxCallable.h"
#include "../include/LoxFunction.h"
#include "../include/LoxReturn.h"
#include "../include/RuntimeError.h"

Interpreter::Interpreter() {
    globals->define("clock", std::shared_ptr<Clock>{});
}

void Interpreter::interpret(const std::vector<std::shared_ptr<Stmt>>& statements) {
    try {
        for (const std::shared_ptr<Stmt>& statement : statements) {
            execute(statement);
        }
    } catch (RuntimeError error) {
        Lox::runtimeError(error);
    }
}

std::any Interpreter::evaluate(std::shared_ptr<Expr> expr) {
    return expr->accept(*this);
}

void Interpreter::execute(std::shared_ptr<Stmt> stmt) {
    stmt->accept(*this);
}

void Interpreter::resolve(std::shared_ptr<Expr> expr, int depth) {
    locals[expr] = depth;
}

void Interpreter::executeBlock(const std::vector<std::shared_ptr<Stmt>>& statements, std::shared_ptr<Environment> environment) {
    std::shared_ptr<Environment> previous = this->environment;
    try {
        this->environment = environment;
        for (const std::shared_ptr<Stmt>& statement : statements) {
            execute(statement);
        }
    } catch (...) {
        this->environment = previous;
        throw;
    }

    this->environment = previous;
}

void Interpreter::visitBlockStmt(std::shared_ptr<BlockStmt> stmt) {
    executeBlock(stmt->statements, std::make_shared<Environment>(environment));
}

void Interpreter::visitExpressionStmt(std::shared_ptr<ExpressionStmt> stmt) {
    evaluate(stmt->expression);
}

void Interpreter::visitFunctionStmt(std::shared_ptr<FunctionStmt> stmt) {
    auto function = std::make_shared<LoxFunction>(stmt, environment);
    environment->define(stmt->name.lexeme, function);
}

void Interpreter::visitIfStmt(std::shared_ptr<IfStmt> stmt) {
    if (isTruthy(evaluate(stmt->condition))) {
        execute(stmt->thenBranch);
    } else if (stmt->elseBranch != nullptr) {
        execute(stmt->elseBranch);
    }
}

void Interpreter::visitPrintStmt(std::shared_ptr<PrintStmt> stmt) {
    std::any value = evaluate(stmt->expression);
    std::cout << stringify(value) << "\n";
}

void Interpreter::visitReturnStmt(std::shared_ptr<ReturnStmt> stmt) {
    std::any value = nullptr;
    if (stmt->value != nullptr) {
        value = evaluate(stmt->value);
    }

    throw LoxReturn{value};
}

void Interpreter::visitVarStmt(std::shared_ptr<VarStmt> stmt) {
    std::any value = nullptr;
    if (stmt->initializer != nullptr) {
        value = evaluate(stmt->initializer);
    }

    environment->define(stmt->name.lexeme, std::move(value));
}

void Interpreter::visitWhileStmt(std::shared_ptr<WhileStmt> stmt) {
    while (isTruthy(evaluate(stmt->condition))) {
        execute(stmt->body);
    }
}

std::any Interpreter::visitAssignExpr(std::shared_ptr<AssignExpr> expr) {
    std::any value = evaluate(expr->value);

    auto element = locals.find(expr);
    if (element != locals.end()) {
        int distance = element->second;
        environment->assignAt(distance, expr->name, value);
    } else {
        globals->assign(expr->name, value);
    }

    return value;
}

std::any Interpreter::visitBinaryExpr(std::shared_ptr<BinaryExpr> expr) {
    std::any left = evaluate(expr->left);
    std::any right = evaluate(expr->right);

    switch (expr->op.type) {
        case BANG_EQUAL:
            return !isEqual(left, right);
        case EQUAL_EQUAL:
            return isEqual(left, right);
        case GREATER:
            checkNumberOperands(expr->op, left, right);
            return std::any_cast<double>(left) > std::any_cast<double>(right);
        case GREATER_EQUAL:
            checkNumberOperands(expr->op, left, right);
            return std::any_cast<double>(left) >= std::any_cast<double>(right);
        case LESS:
            checkNumberOperands(expr->op, left, right);
            return std::any_cast<double>(left) < std::any_cast<double>(right);
        case LESS_EQUAL:
            checkNumberOperands(expr->op, left, right);
            return std::any_cast<double>(left) <= std::any_cast<double>(right);
        case MINUS:
            checkNumberOperands(expr->op, left, right);
            return std::any_cast<double>(left) - std::any_cast<double>(right);
        case PLUS:
            if (left.type() == typeid(double) && right.type() == typeid(double)) {
                return std::any_cast<double>(left) + std::any_cast<double>(right);
            }

            if (left.type() == typeid(std::string) && right.type() == typeid(std::string)) {
                return std::any_cast<std::string>(left) + std::any_cast<std::string>(right);
            }

            throw RuntimeError{expr->op, "Operands must be two numbers or two strings."};
        case SLASH:
            checkNumberOperands(expr->op, left, right);
            return std::any_cast<double>(left) / std::any_cast<double>(right);
        case STAR:
            checkNumberOperands(expr->op, left, right);
            return std::any_cast<double>(left) * std::any_cast<double>(right);
        default:
            return {};
    }

    // Unreachable.
    return {};
}

std::any Interpreter::visitCallExpr(std::shared_ptr<CallExpr> expr) {
    std::any callee = evaluate(expr->callee);

    std::vector<std::any> arguments;
    for (const std::shared_ptr<Expr>& argument : expr->arguments) {
        arguments.push_back(evaluate(argument));
    }

    std::shared_ptr<LoxCallable> function;

    if (callee.type() == typeid(std::shared_ptr<LoxFunction>)) {
        function = std::any_cast<std::shared_ptr<LoxFunction>>(callee);
    } else {
        throw RuntimeError{expr->paren, "Can only call functions and classes."};
    }

    if (arguments.size() != function->arity()) {
        throw RuntimeError{expr->paren, "Expected " + std::to_string(function->arity()) + " arguments but got " +
                                            std::to_string(arguments.size()) + "."};
    }

    return function->call(*this, std::move(arguments));
}

std::any Interpreter::visitGroupingExpr(std::shared_ptr<GroupingExpr> expr) {
    return evaluate(expr->expression);
}

std::any Interpreter::visitLiteralExpr(std::shared_ptr<LiteralExpr> expr) {
    return expr->value;
}

std::any Interpreter::visitLogicalExpr(std::shared_ptr<LogicalExpr> expr) {
    std::any left = evaluate(expr->left);

    if (expr->op.type == OR) {
        if (isTruthy(left)) {
            return left;
        }
    } else {
        if (!isTruthy(left)) {
            return left;
        }
    }

    return evaluate(expr->right);
}

std::any Interpreter::visitUnaryExpr(std::shared_ptr<UnaryExpr> expr) {
    std::any right = evaluate(expr->right);

    switch (expr->op.type) {
        case BANG:
            return !isTruthy(right);
        case MINUS:
            checkNumberOperand(expr->op, right);
            return -std::any_cast<double>(right);
        default:
            return std::any{};
    }

    // Unreachable.
    return {};
}

std::any Interpreter::visitVariableExpr(std::shared_ptr<VariableExpr> expr) {
    return lookUpVariable(expr->name, expr);
}

std::any Interpreter::lookUpVariable(const Token& name, std::shared_ptr<Expr> expr) {
    auto elem = locals.find(expr);
    if (elem != locals.end()) {
        int distance = elem->second;
        return environment->getAt(distance, name.lexeme);
    } else {
        return globals->get(name);
    }
}

void Interpreter::checkNumberOperand(const Token& op, const std::any& operand) {
    if (operand.type() == typeid(double)) {
        return;
    }
    throw RuntimeError{op, "Operand must be a number."};
}

void Interpreter::checkNumberOperands(const Token& op, const std::any& left, const std::any& right) {
    if (left.type() == typeid(double) && right.type() == typeid(double)) {
        return;
    }

    throw RuntimeError{op, "Operands must be numbers."};
}

bool Interpreter::isTruthy(const std::any& object) {
    if (object.type() == typeid(nullptr)) {
        return false;
    }
    if (object.type() == typeid(bool)) {
        return std::any_cast<bool>(object);
    }
    return true;
}

bool Interpreter::isEqual(const std::any& a, const std::any& b) {
    if (a.type() == typeid(nullptr) && b.type() == typeid(nullptr)) {
        return true;
    }
    if (a.type() == typeid(nullptr)) {
        return false;
    }

    if (a.type() == typeid(std::string) && b.type() == typeid(std::string)) {
        return std::any_cast<std::string>(a) == std::any_cast<std::string>(b);
    }
    if (a.type() == typeid(double) && b.type() == typeid(double)) {
        return std::any_cast<double>(a) == std::any_cast<double>(b);
    }
    if (a.type() == typeid(bool) && b.type() == typeid(bool)) {
        return std::any_cast<bool>(a) == std::any_cast<bool>(b);
    }

    return false;
}

std::string Interpreter::stringify(const std::any& object) {
    if (object.type() == typeid(nullptr)) {
        return "nil";
    }

    if (object.type() == typeid(double)) {
        std::string text = std::to_string(std::any_cast<double>(object));
        if (text[text.length() - 2] == '.' && text[text.length() - 1] == '0') {
            text = text.substr(0, text.length() - 2);
        }
        return text;
    }

    if (object.type() == typeid(std::string)) {
        return std::any_cast<std::string>(object);
    }
    if (object.type() == typeid(bool)) {
        return std::any_cast<bool>(object) ? "true" : "false";
    }
    if (object.type() == typeid(std::shared_ptr<LoxFunction>)) {
        return std::any_cast<std::shared_ptr<LoxFunction>>(object)->toString();
    }

    return "Error in stringify: object type not recognized.";
}