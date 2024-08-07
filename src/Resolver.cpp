#include "../include/Resolver.h"

#include <iostream>

#include "../include/Lox.h"

Resolver::Resolver(Interpreter& interpreter) : interpreter{interpreter} {}

void Resolver::resolve(const std::vector<std::shared_ptr<Stmt>>& statements) {
    for (const std::shared_ptr<Stmt>& statement : statements) {
        resolve(statement);
    }
}

void Resolver::visitBlockStmt(std::shared_ptr<BlockStmt> stmt) {
    beginScope();
    resolve(stmt->statements);
    endScope();
}

void Resolver::visitExpressionStmt(std::shared_ptr<ExpressionStmt> stmt) {
    resolve(stmt->expression);
}

void Resolver::visitFunctionStmt(std::shared_ptr<FunctionStmt> stmt) {
    declare(stmt->name);
    define(stmt->name);

    // resolveFunction(stmt);
    resolveFunction(stmt, FunctionType::FUNCTION);
}

void Resolver::visitIfStmt(std::shared_ptr<IfStmt> stmt) {
    resolve(stmt->condition);
    resolve(stmt->thenBranch);
    if (stmt->elseBranch != nullptr) {
        resolve(stmt->elseBranch);
    }
}

void Resolver::visitPrintStmt(std::shared_ptr<PrintStmt> stmt) {
    resolve(stmt->expression);
}

void Resolver::visitReturnStmt(std::shared_ptr<ReturnStmt> stmt) {
    if (currentFunction == FunctionType::NONE) {
        Lox::error(stmt->keyword, "Can't return from top-level code.");
    }

    if (stmt->value != nullptr) {
        resolve(stmt->value);
    }
}

void Resolver::visitVarStmt(std::shared_ptr<VarStmt> stmt) {
    declare(stmt->name);
    if (stmt->initializer != nullptr) {
        resolve(stmt->initializer);
    }
    define(stmt->name);
}

void Resolver::visitWhileStmt(std::shared_ptr<WhileStmt> stmt) {
    resolve(stmt->condition);
    resolve(stmt->body);
}

std::any Resolver::visitAssignExpr(std::shared_ptr<AssignExpr> expr) {
    resolve(expr->value);
    resolveLocal(expr, expr->name);
    return {};
}

std::any Resolver::visitBinaryExpr(std::shared_ptr<BinaryExpr> expr) {
    resolve(expr->left);
    resolve(expr->right);
    return {};
}

std::any Resolver::visitCallExpr(std::shared_ptr<CallExpr> expr) {
    resolve(expr->callee);

    for (const std::shared_ptr<Expr>& argument : expr->arguments) {
        resolve(argument);
    }

    return {};
}

std::any Resolver::visitGroupingExpr(std::shared_ptr<GroupingExpr> expr) {
    resolve(expr->expression);
    return {};
}

std::any Resolver::visitLiteralExpr(std::shared_ptr<LiteralExpr> expr) {
    return {};
}

std::any Resolver::visitLogicalExpr(std::shared_ptr<LogicalExpr> expr) {
    resolve(expr->left);
    resolve(expr->right);
    return {};
}

std::any Resolver::visitUnaryExpr(std::shared_ptr<UnaryExpr> expr) {
    resolve(expr->right);
    return {};
}

std::any Resolver::visitVariableExpr(std::shared_ptr<VariableExpr> expr) {
    if (!scopes.empty()) {
        auto& scope = scopes.back();
        auto elem = scope.find(expr->name.lexeme);
        if (elem != scope.end() && elem->second == false) {
            Lox::error(expr->name, "Can't read local variable in its own initializer.");
        }
    }

    resolveLocal(expr, expr->name);
    return {};
}

void Resolver::resolve(std::shared_ptr<Stmt> stmt) {
    stmt->accept(*this);
}

void Resolver::resolve(std::shared_ptr<Expr> expr) {
    expr->accept(*this);
}

// void resolveFunction(std::shared_ptr<Function> function) {
void Resolver::resolveFunction(std::shared_ptr<FunctionStmt> function, FunctionType type) {
    FunctionType enclosingFunction = currentFunction;
    currentFunction = type;

    beginScope();
    for (const Token& param : function->parameters) {
        declare(param);
        define(param);
    }
    resolve(function->body);
    endScope();
    currentFunction = enclosingFunction;
}

void Resolver::beginScope() {
    scopes.push_back(std::map<std::string, bool>{});
}

void Resolver::endScope() {
    scopes.pop_back();
}

void Resolver::declare(const Token& name) {
    if (scopes.empty()) {
        return;
    }

    std::map<std::string, bool>& scope = scopes.back();
    if (scope.find(name.lexeme) != scope.end()) {
        Lox::error(name, "Already a variable with this name in this scope.");
    }

    scope[name.lexeme] = false;
}

void Resolver::define(const Token& name) {
    if (scopes.empty()) {
        return;
    }
    scopes.back()[name.lexeme] = true;
}

void Resolver::resolveLocal(std::shared_ptr<Expr> expr, const Token& name) {
    for (int i = scopes.size() - 1; i >= 0; --i) {
        if (scopes[i].find(name.lexeme) != scopes[i].end()) {
            interpreter.resolve(expr, scopes.size() - 1 - i);
            return;
        }
    }
}