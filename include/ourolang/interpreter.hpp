#pragma once
#include "parser.hpp"
#include "type_checker.hpp"
#include <functional>
#include <variant>
#include <map>
#include <thread>
#include <chrono>
#include <iostream>

namespace ouro {

class Interpreter {
    using Value = std::variant<double, std::string, std::monostate>;
    using Function = std::function<Value(const std::vector<Value>&, Interpreter&)>;
    std::map<std::string, Value> env;
    std::map<std::string, Function> functions;
    Value return_value;
    std::vector<StmtPtr> program;

public:
    Interpreter() {
        functions["print"] = [](const std::vector<Value>& args, Interpreter&) -> Value {
            for (const auto& a : args) {
                if (std::holds_alternative<double>(a)) std::cout << std::get<double>(a);
                else if (std::holds_alternative<std::string>(a)) std::cout << std::get<std::string>(a);
            }
            std::cout << std::endl;
            return {};
        };
        functions["sleep"] = [](const std::vector<Value>& args, Interpreter&) -> Value {
            if (!args.empty() && std::holds_alternative<double>(args[0])) {
                std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(std::get<double>(args[0]))));
            }
            return {};
        };
    }

    void run(const std::string& source) {
        Lexer lex(source);
        auto tokens = lex.tokenize();
        Parser parser(tokens);
        auto ast = parser.parse();
        TypeChecker checker;
        checker.check(ast);
        for (auto& s : ast) {
            program.push_back(std::move(s));
            execute_stmt(*program.back());
        }
    }

private:
    void execute_stmt(const Stmt& stmt, std::map<std::string, Value>* local_env = nullptr) {
        auto& e = local_env ? *local_env : env;
        if (std::holds_alternative<VarDeclStmt>(stmt.value)) {
            const auto& v = std::get<VarDeclStmt>(stmt.value);
            e[v.name] = evaluate_expr(*v.value, e);
        } else if (std::holds_alternative<FnDeclStmt>(stmt.value)) {
            auto& fn = std::get<FnDeclStmt>(stmt.value);
            functions[fn.name] = [this, &fn](const std::vector<Value>& args, Interpreter& interp) -> Value {
                std::map<std::string, Value> fn_env = env;
                for (size_t i = 0; i < args.size() && i < fn.params.size(); ++i) {
                    fn_env[fn.params[i].first] = args[i];
                }
                for (const auto& b : fn.body) {
                    interp.execute_stmt(*b, &fn_env);
                }
                return interp.return_value;
            };
        } else if (std::holds_alternative<IfStmt>(stmt.value)) {
            const auto& i = std::get<IfStmt>(stmt.value);
            auto cond = evaluate_expr(*i.condition, e);
            bool truth = false;
            if (std::holds_alternative<double>(cond)) truth = std::get<double>(cond) != 0;
            if (truth) {
                for (const auto& s : i.then_branch) execute_stmt(*s, &e);
            } else {
                for (const auto& s : i.else_branch) execute_stmt(*s, &e);
            }
        } else if (std::holds_alternative<ForStmt>(stmt.value)) {
            const auto& f = std::get<ForStmt>(stmt.value);
            auto start = evaluate_expr(*f.start, e);
            auto end = evaluate_expr(*f.end, e);
            int s = static_cast<int>(std::get<double>(start));
            int en = static_cast<int>(std::get<double>(end));
            for (int i = s; i < en; ++i) {
                e[f.var] = static_cast<double>(i);
                for (const auto& st : f.body) execute_stmt(*st, &e);
            }
        } else if (std::holds_alternative<ReturnStmt>(stmt.value)) {
            const auto& r = std::get<ReturnStmt>(stmt.value);
            if (r.value) return_value = evaluate_expr(*r.value, e);
            else return_value = {};
        }
    }

    Value evaluate_expr(const Expr& expr, std::map<std::string, Value>& e) {
        if (std::holds_alternative<NumberExpr>(expr.value)) {
            return std::get<NumberExpr>(expr.value).value;
        } else if (std::holds_alternative<StringExpr>(expr.value)) {
            return std::get<StringExpr>(expr.value).value;
        } else if (std::holds_alternative<IdentExpr>(expr.value)) {
            const auto& id = std::get<IdentExpr>(expr.value);
            if (e.find(id.name) != e.end()) return e[id.name];
            throw std::runtime_error("Undefined variable: " + id.name);
        } else if (std::holds_alternative<BinaryExpr>(expr.value)) {
            const auto& b = std::get<BinaryExpr>(expr.value);
            auto left = evaluate_expr(*b.left, e);
            auto right = evaluate_expr(*b.right, e);
            double l = std::get<double>(left);
            double r = std::get<double>(right);
            switch (b.op) {
                case TokenType::PLUS: return l + r;
                case TokenType::MINUS: return l - r;
                case TokenType::MUL: return l * r;
                case TokenType::DIV: return l / r;
                case TokenType::GT: return l > r ? 1.0 : 0.0;
                default: throw std::runtime_error("Invalid operator");
            }
        } else if (std::holds_alternative<CallExpr>(expr.value)) {
            const auto& c = std::get<CallExpr>(expr.value);
            if (functions.find(c.name) == functions.end()) {
                throw std::runtime_error("Undefined function: " + c.name);
            }
            std::vector<Value> args;
            for (const auto& a : c.args) args.push_back(evaluate_expr(*a, e));
            return functions[c.name](args, *this);
        } else if (std::holds_alternative<AwaitExpr>(expr.value)) {
            const auto& a = std::get<AwaitExpr>(expr.value);
            return evaluate_expr(*a.expr, e);
        }
        throw std::runtime_error("Invalid expression");
    }
};

} // namespace ouro
