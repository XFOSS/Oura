#include "ourolang/ast.h"
#include "ourolang/token.h"
#include <cassert>
#include <string>
#include <memory>

using namespace ouro;

static std::string emit_expr(const Expr& expr) {
    if (std::holds_alternative<NumberExpr>(expr.value)) {
        return std::to_string(std::get<NumberExpr>(expr.value).value);
    }
    if (std::holds_alternative<IdentExpr>(expr.value)) {
        return std::get<IdentExpr>(expr.value).name;
    }
    if (std::holds_alternative<BinaryExpr>(expr.value)) {
        const auto& bin = std::get<BinaryExpr>(expr.value);
        std::string op;
        switch (bin.op) {
            case TokenType::PLUS: op = "+"; break;
            case TokenType::MINUS: op = "-"; break;
            case TokenType::MUL: op = "*"; break;
            case TokenType::DIV: op = "/"; break;
            default: op = "?"; break;
        }
        return "(" + emit_expr(*bin.left) + " " + op + " " + emit_expr(*bin.right) + ")";
    }
    return {};
}

static std::string emit_stmt(const Stmt& stmt) {
    if (std::holds_alternative<ReturnStmt>(stmt.value)) {
        const auto& r = std::get<ReturnStmt>(stmt.value);
        return "return " + emit_expr(*r.value) + ";";
    }
    if (std::holds_alternative<FnDeclStmt>(stmt.value)) {
        const auto& fn = std::get<FnDeclStmt>(stmt.value);
        std::string out = "fn " + fn.name + "(";
        for (size_t i = 0; i < fn.params.size(); ++i) {
            out += fn.params[i].first + ":" + fn.params[i].second;
            if (i + 1 < fn.params.size()) out += ", ";
        }
        out += ")";
        if (!fn.return_type.empty()) out += " -> " + fn.return_type;
        out += " { ";
        for (size_t i = 0; i < fn.body.size(); ++i) {
            out += emit_stmt(*fn.body[i]);
            if (i + 1 < fn.body.size()) out += " ";
        }
        out += " }";
        return out;
    }
    return {};
}

int main() {
    // Build AST for: fn add(a:int, b:int) -> int { return a + b; }
    auto left = std::make_unique<Expr>(IdentExpr{"a"});
    auto right = std::make_unique<Expr>(IdentExpr{"b"});
    auto sum = std::make_unique<Expr>(BinaryExpr{TokenType::PLUS, std::move(left), std::move(right)});
    auto ret_stmt = std::make_unique<Stmt>(ReturnStmt{std::move(sum)});

    FnDeclStmt fn{"add", {{"a","int"}, {"b","int"}}, "int", {}, false, false, false, {}};
    fn.body.push_back(std::move(ret_stmt));

    Stmt fn_stmt(std::move(fn));
    std::string generated = emit_stmt(fn_stmt);
    std::string expected = "fn add(a:int, b:int) -> int { return (a + b); }";
    assert(generated == expected);
    return 0;
}
