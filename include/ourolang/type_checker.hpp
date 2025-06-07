#pragma once
#include "ast.hpp"
#include <map>
#include <string>

namespace ouro {

class TypeChecker {
    std::map<std::string, std::string> env;
    std::map<std::string, FnDeclStmt*> functions;

public:
    void check(const std::vector<StmtPtr>& stmts) {
        for (const auto& s : stmts) check_stmt(*s);
    }

private:
    void check_stmt(const Stmt& stmt) {
        if (std::holds_alternative<VarDeclStmt>(stmt.value)) {
            const auto& var = std::get<VarDeclStmt>(stmt.value);
            auto inferred = infer_type(var.value.get());
            if (!var.type.empty() && var.type != inferred) {
                throw std::runtime_error("Type mismatch for " + var.name);
            }
            env[var.name] = var.type.empty() ? inferred : var.type;
        } else if (std::holds_alternative<FnDeclStmt>(stmt.value)) {
            const auto& fn = std::get<FnDeclStmt>(stmt.value);
            functions[fn.name] = const_cast<FnDeclStmt*>(&fn);
            auto saved = env;
            for (const auto& p : fn.params) env[p.first] = p.second;
            for (const auto& b : fn.body) check_stmt(*b);
            env = saved;
        } else if (std::holds_alternative<IfStmt>(stmt.value)) {
            const auto& i = std::get<IfStmt>(stmt.value);
            if (infer_type(i.condition.get()) != "int") {
                throw std::runtime_error("If condition must be int");
            }
            for (const auto& s : i.then_branch) check_stmt(*s);
            for (const auto& s : i.else_branch) check_stmt(*s);
        } else if (std::holds_alternative<ForStmt>(stmt.value)) {
            const auto& f = std::get<ForStmt>(stmt.value);
            if (infer_type(f.start.get()) != "int" || infer_type(f.end.get()) != "int") {
                throw std::runtime_error("For loop bounds must be int");
            }
            env[f.var] = "int";
            for (const auto& s : f.body) check_stmt(*s);
        } else if (std::holds_alternative<ReturnStmt>(stmt.value)) {
            const auto& r = std::get<ReturnStmt>(stmt.value);
            if (r.value) {
                infer_type(r.value.get());
            }
        }
    }

    std::string infer_type(const Expr* expr) {
        if (std::holds_alternative<NumberExpr>(expr->value)) return "float";
        if (std::holds_alternative<StringExpr>(expr->value)) return "string";
        if (std::holds_alternative<IdentExpr>(expr->value)) {
            const auto& id = std::get<IdentExpr>(expr->value);
            if (env.find(id.name) != env.end()) return env[id.name];
            throw std::runtime_error("Undefined variable: " + id.name);
        }
        if (std::holds_alternative<BinaryExpr>(expr->value)) {
            const auto& b = std::get<BinaryExpr>(expr->value);
            auto lt = infer_type(b.left.get());
            auto rt = infer_type(b.right.get());
            if (lt != rt) throw std::runtime_error("Type mismatch in binary op");
            if (b.op == TokenType::GT) return "int";
            return lt;
        }
        if (std::holds_alternative<CallExpr>(expr->value)) {
            const auto& c = std::get<CallExpr>(expr->value);
            if (functions.find(c.name) != functions.end()) {
                return functions[c.name]->return_type;
            }
            throw std::runtime_error("Undefined function: " + c.name);
        }
        if (std::holds_alternative<AwaitExpr>(expr->value)) {
            const auto& a = std::get<AwaitExpr>(expr->value);
            return infer_type(a.expr.get());
        }
        return "unknown";
    }
};

} // namespace ouro
