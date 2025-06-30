#include "ourolang/lexer.h"
#include "ourolang/parser.h"
#include "ourolang/ast.h"
#include <cassert>
#include <sstream>

// Simple code generator that emits literals and identifiers
static void emit_expr(const ouro::Expr& e, std::ostringstream& out) {
    using namespace ouro;
    if (std::holds_alternative<NumberExpr>(e.value)) {
        out << std::get<NumberExpr>(e.value).value;
    } else if (std::holds_alternative<StringExpr>(e.value)) {
        out << '"' << std::get<StringExpr>(e.value).value << '"';
    } else if (std::holds_alternative<IdentExpr>(e.value)) {
        out << std::get<IdentExpr>(e.value).name;
    } else if (std::holds_alternative<BinaryExpr>(e.value)) {
        const auto& b = std::get<BinaryExpr>(e.value);
        out << '(';
        emit_expr(*b.left, out);
        switch (b.op) {
            case TokenType::PLUS: out << " + "; break;
            case TokenType::MINUS: out << " - "; break;
            case TokenType::MUL: out << " * "; break;
            case TokenType::DIV: out << " / "; break;
            default: out << ' '; break;
        }
        emit_expr(*b.right, out);
        out << ')';
    }
}

int main() {
    const std::string src = "let x = 2 + 3;";
    ouro::Lexer lex(src);
    auto tokens = lex.tokenize();
    ouro::Parser parser(tokens);
    auto ast = parser.parse();
    assert(!ast.empty());
    std::ostringstream out;
    const auto& stmt = *ast.front();
    const auto& var = std::get<ouro::VarDeclStmt>(stmt.value);
    emit_expr(*var.value, out);
    assert(out.str() == "(2 + 3)");
    return 0;
}
