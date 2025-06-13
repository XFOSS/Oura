#pragma once
#include "lexer.h"
#include "ast.h"
#include <vector>

namespace ouro {

class Parser {
    std::vector<Token> tokens;
    size_t pos = 0;

public:
    explicit Parser(const std::vector<Token>& t) : tokens(t) {}

    std::vector<StmtPtr> parse() {
        std::vector<StmtPtr> stmts;
        while (tokens[pos].type != TokenType::EOF_TOKEN) {
            stmts.push_back(parse_stmt());
        }
        return stmts;
    }

private:
    Token peek() { return tokens[pos]; }
    Token advance() { return tokens[pos++]; }
    Token consume(TokenType type, const std::string& msg) {
        if (peek().type == type) return advance();
        throw std::runtime_error(msg + " at line " + std::to_string(peek().line));
    }

    StmtPtr parse_stmt() {
        if (peek().type == TokenType::LET) return parse_var_decl();
        if (peek().type == TokenType::FN || peek().type == TokenType::ASYNC || peek().type == TokenType::GPU) {
            return parse_fn_decl();
        }
        if (peek().type == TokenType::IF) return parse_if_stmt();
        if (peek().type == TokenType::FOR) return parse_for_stmt();
        if (peek().type == TokenType::RETURN) return parse_return_stmt();
        throw std::runtime_error("Unexpected token at line " + std::to_string(peek().line));
    }

    StmtPtr parse_var_decl() {
        consume(TokenType::LET, "Expected 'let'");
        auto name = consume(TokenType::IDENTIFIER, "Expected identifier").value;
        std::string type;
        if (peek().type == TokenType::COLON) {
            consume(TokenType::COLON, "Expected ':'");
            type = consume(TokenType::IDENTIFIER, "Expected type").value;
        }
        consume(TokenType::EQUALS, "Expected '='");
        auto value = parse_expr();
        consume(TokenType::SEMICOLON, "Expected ';'");
        return std::make_unique<Stmt>(VarDeclStmt{name, type, std::move(value)});
    }

    StmtPtr parse_fn_decl() {
        bool is_async = false, is_gpu = false;
        if (peek().type == TokenType::ASYNC) { consume(TokenType::ASYNC, ""); is_async = true; }
        else if (peek().type == TokenType::GPU) { consume(TokenType::GPU, ""); is_gpu = true; }
        consume(TokenType::FN, "Expected 'fn'");
        auto name = consume(TokenType::IDENTIFIER, "Expected identifier").value;
        consume(TokenType::LPAREN, "Expected '('");
        std::vector<std::pair<std::string, std::string>> params;
        if (peek().type != TokenType::RPAREN) {
            do {
                auto param_name = consume(TokenType::IDENTIFIER, "Expected param name").value;
                consume(TokenType::COLON, "Expected ':'");
                auto param_type = consume(TokenType::IDENTIFIER, "Expected param type").value;
                params.push_back({param_name, param_type});
                if (peek().type == TokenType::COMMA) consume(TokenType::COMMA, "");
            } while (peek().type != TokenType::RPAREN);
        }
        consume(TokenType::RPAREN, "Expected ')'");
        std::string return_type;
        if (peek().type == TokenType::ARROW) {
            consume(TokenType::ARROW, "Expected '->'");
            return_type = consume(TokenType::IDENTIFIER, "Expected return type").value;
        }
        consume(TokenType::LBRACE, "Expected '{'");
        std::vector<StmtPtr> body;
        while (peek().type != TokenType::RBRACE) {
            body.push_back(parse_stmt());
        }
        consume(TokenType::RBRACE, "Expected '}'");
        auto fn = FnDeclStmt{name, params, return_type, std::move(body), is_async, is_gpu, false, {}};
        return std::make_unique<Stmt>(std::move(fn));
    }

    StmtPtr parse_if_stmt() {
        consume(TokenType::IF, "Expected 'if'");
        auto condition = parse_expr();
        consume(TokenType::LBRACE, "Expected '{'");
        std::vector<StmtPtr> then_branch;
        while (peek().type != TokenType::RBRACE && peek().type != TokenType::ELSE) {
            then_branch.push_back(parse_stmt());
        }
        consume(TokenType::RBRACE, "Expected '}'");
        std::vector<StmtPtr> else_branch;
        if (peek().type == TokenType::ELSE) {
            consume(TokenType::ELSE, "");
            consume(TokenType::LBRACE, "Expected '{'");
            while (peek().type != TokenType::RBRACE) {
                else_branch.push_back(parse_stmt());
            }
            consume(TokenType::RBRACE, "Expected '}'");
        }
        return std::make_unique<Stmt>(IfStmt{std::move(condition), std::move(then_branch), std::move(else_branch)});
    }

    StmtPtr parse_for_stmt() {
        consume(TokenType::FOR, "Expected 'for'");
        auto var = consume(TokenType::IDENTIFIER, "Expected loop variable").value;
        consume(TokenType::IN, "Expected 'in'");
        auto start = parse_expr();
        consume(TokenType::DOTDOT, "Expected '..'");
        auto end = parse_expr();
        consume(TokenType::LBRACE, "Expected '{'");
        std::vector<StmtPtr> body;
        while (peek().type != TokenType::RBRACE) {
            body.push_back(parse_stmt());
        }
        consume(TokenType::RBRACE, "Expected '}'");
        return std::make_unique<Stmt>(ForStmt{var, std::move(start), std::move(end), std::move(body)});
    }

    StmtPtr parse_return_stmt() {
        consume(TokenType::RETURN, "Expected 'return'");
        ExprPtr value;
        if (peek().type != TokenType::SEMICOLON) {
            value = parse_expr();
        }
        consume(TokenType::SEMICOLON, "Expected ';'");
        return std::make_unique<Stmt>(ReturnStmt{std::move(value)});
    }

    ExprPtr parse_expr() { return parse_binary_expr(0); }

    int precedence(TokenType op) {
        switch (op) {
            case TokenType::MUL:
            case TokenType::DIV: return 2;
            case TokenType::PLUS:
            case TokenType::MINUS: return 1;
            case TokenType::GT: return 0;
            default: return -1;
        }
    }

    ExprPtr parse_binary_expr(int prec) {
        auto left = parse_primary_expr();
        while (true) {
            TokenType op = peek().type;
            int op_prec = precedence(op);
            if (op_prec <= prec) break;
            advance();
            auto right = parse_binary_expr(op_prec);
            left = std::make_unique<Expr>(BinaryExpr{op, std::move(left), std::move(right)});
        }
        return left;
    }

    ExprPtr parse_primary_expr() {
        if (peek().type == TokenType::NUMBER) {
            double val = std::stod(consume(TokenType::NUMBER, "Expected number").value);
            return std::make_unique<Expr>(NumberExpr{val});
        }
        if (peek().type == TokenType::STRING_LITERAL) {
            auto val = consume(TokenType::STRING_LITERAL, "Expected string").value;
            return std::make_unique<Expr>(StringExpr{val});
        }
        if (peek().type == TokenType::IDENTIFIER) {
            auto name = consume(TokenType::IDENTIFIER, "Expected identifier").value;
            if (peek().type == TokenType::LPAREN) {
                consume(TokenType::LPAREN, "Expected '('");
                std::vector<ExprPtr> args;
                if (peek().type != TokenType::RPAREN) {
                    do {
                        args.push_back(parse_expr());
                        if (peek().type == TokenType::COMMA) consume(TokenType::COMMA, "");
                    } while (peek().type != TokenType::RPAREN);
                }
                consume(TokenType::RPAREN, "Expected ')'");
                return std::make_unique<Expr>(CallExpr{name, std::move(args)});
            }
            return std::make_unique<Expr>(IdentExpr{name});
        }
        throw std::runtime_error("Unexpected token in expression at line " + std::to_string(peek().line));
    }
};

} // namespace ouro
