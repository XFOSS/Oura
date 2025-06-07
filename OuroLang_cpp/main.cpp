#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <variant>
#include <cctype>
#include <stdexcept>

// Token Types
enum class TokenType {
    LET, FN, IF, ELSE, RETURN, INT, FLOAT, COLON, EQUALS, LPAREN, RPAREN, LBRACE, RBRACE, SEMICOLON,
    PLUS, MINUS, STAR, SLASH, IDENTIFIER, NUMBER, EOF_TOKEN
};

struct Token {
    TokenType type;
    std::string value;
    int line;
};

class Lexer {
    std::string source;
    size_t pos = 0;
    int line = 1;
public:
    Lexer(const std::string& src) : source(src) {}

    std::vector<Token> tokenize() {
        std::vector<Token> tokens;
        while (pos < source.size()) {
            char c = source[pos];
            if (std::isspace(static_cast<unsigned char>(c))) {
                if (c == '\n') line++;
                pos++;
                continue;
            }
            if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
                tokens.push_back(parse_identifier());
            } else if (std::isdigit(static_cast<unsigned char>(c)) || c == '.') {
                tokens.push_back(parse_number());
            } else {
                tokens.push_back(parse_symbol());
            }
        }
        tokens.push_back({TokenType::EOF_TOKEN, "", line});
        return tokens;
    }
private:
    Token parse_identifier() {
        std::string value;
        while (pos < source.size() && (std::isalnum(static_cast<unsigned char>(source[pos])) || source[pos] == '_')) {
            value += source[pos++];
        }
        if (value == "let") return {TokenType::LET, value, line};
        if (value == "fn") return {TokenType::FN, value, line};
        if (value == "if") return {TokenType::IF, value, line};
        if (value == "else") return {TokenType::ELSE, value, line};
        if (value == "return") return {TokenType::RETURN, value, line};
        if (value == "int") return {TokenType::INT, value, line};
        if (value == "float") return {TokenType::FLOAT, value, line};
        return {TokenType::IDENTIFIER, value, line};
    }

    Token parse_number() {
        std::string value;
        while (pos < source.size() && (std::isdigit(static_cast<unsigned char>(source[pos])) || source[pos] == '.')) {
            value += source[pos++];
        }
        return {TokenType::NUMBER, value, line};
    }

    Token parse_symbol() {
        char c = source[pos++];
        switch (c) {
            case ':': return {TokenType::COLON, ":", line};
            case '=': return {TokenType::EQUALS, "=", line};
            case '(': return {TokenType::LPAREN, "(", line};
            case ')': return {TokenType::RPAREN, ")", line};
            case '{': return {TokenType::LBRACE, "{", line};
            case '}': return {TokenType::RBRACE, "}", line};
            case ';': return {TokenType::SEMICOLON, ";", line};
            case '+': return {TokenType::PLUS, "+", line};
            case '-': return {TokenType::MINUS, "-", line};
            case '*': return {TokenType::STAR, "*", line};
            case '/': return {TokenType::SLASH, "/", line};
            default: throw std::runtime_error("Unknown symbol at line " + std::to_string(line));
        }
    }
};

struct Expr;
struct Stmt;
using ExprPtr = std::unique_ptr<Expr>;
using StmtPtr = std::unique_ptr<Stmt>;

struct NumberExpr { double value; };
struct IdentifierExpr { std::string name; };
struct BinaryExpr { TokenType op; ExprPtr left; ExprPtr right; };

struct VarDeclStmt { std::string name; std::string type; ExprPtr value; };
struct IfStmt { ExprPtr condition; std::vector<StmtPtr> then_branch; std::vector<StmtPtr> else_branch; };
struct ReturnStmt { ExprPtr value; };

using ExprVariant = std::variant<NumberExpr, IdentifierExpr, BinaryExpr>;
struct Expr { ExprVariant value; };

using StmtVariant = std::variant<VarDeclStmt, IfStmt, ReturnStmt>;
struct Stmt { StmtVariant value; };

class Parser {
    std::vector<Token> tokens;
    size_t pos = 0;
public:
    Parser(const std::vector<Token>& t) : tokens(t) {}

    std::vector<StmtPtr> parse() {
        std::vector<StmtPtr> stmts;
        while (tokens[pos].type != TokenType::EOF_TOKEN) {
            stmts.push_back(parse_stmt());
        }
        return stmts;
    }
private:
    Token peek() { return tokens[pos]; }
    Token consume(TokenType type, const std::string& msg) {
        if (peek().type == type) return tokens[pos++];
        throw std::runtime_error(msg + " at line " + std::to_string(peek().line));
    }

    StmtPtr parse_stmt() {
        if (peek().type == TokenType::LET) return parse_var_decl();
        if (peek().type == TokenType::IF) return parse_if_stmt();
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
        return std::make_unique<Stmt>(Stmt{VarDeclStmt{name, type, std::move(value)}});
    }

    StmtPtr parse_if_stmt() {
        consume(TokenType::IF, "Expected 'if'");
        consume(TokenType::LPAREN, "Expected '('");
        auto condition = parse_expr();
        consume(TokenType::RPAREN, "Expected ')'");
        consume(TokenType::LBRACE, "Expected '{'");
        std::vector<StmtPtr> then_branch;
        while (peek().type != TokenType::RBRACE) {
            then_branch.push_back(parse_stmt());
        }
        consume(TokenType::RBRACE, "Expected '}'");
        std::vector<StmtPtr> else_branch;
        if (peek().type == TokenType::ELSE) {
            consume(TokenType::ELSE, "Expected 'else'");
            consume(TokenType::LBRACE, "Expected '{'");
            while (peek().type != TokenType::RBRACE) {
                else_branch.push_back(parse_stmt());
            }
            consume(TokenType::RBRACE, "Expected '}'");
        }
        return std::make_unique<Stmt>(Stmt{IfStmt{std::move(condition), std::move(then_branch), std::move(else_branch)}});
    }

    StmtPtr parse_return_stmt() {
        consume(TokenType::RETURN, "Expected 'return'");
        auto value = parse_expr();
        consume(TokenType::SEMICOLON, "Expected ';'");
        return std::make_unique<Stmt>(Stmt{ReturnStmt{std::move(value)}});
    }

    ExprPtr parse_expr() { return parse_additive(); }

    ExprPtr parse_additive() {
        auto left = parse_multiplicative();
        while (peek().type == TokenType::PLUS || peek().type == TokenType::MINUS) {
            TokenType op = consume(peek().type, "Expected '+' or '-'").type;
            auto right = parse_multiplicative();
            left = std::make_unique<Expr>(Expr{BinaryExpr{op, std::move(left), std::move(right)}});
        }
        return left;
    }

    ExprPtr parse_multiplicative() {
        auto left = parse_primary();
        while (peek().type == TokenType::STAR || peek().type == TokenType::SLASH) {
            TokenType op = consume(peek().type, "Expected '*' or '/'").type;
            auto right = parse_primary();
            left = std::make_unique<Expr>(Expr{BinaryExpr{op, std::move(left), std::move(right)}});
        }
        return left;
    }

    ExprPtr parse_primary() {
        if (peek().type == TokenType::NUMBER) {
            double val = std::stod(consume(TokenType::NUMBER, "Expected number").value);
            return std::make_unique<Expr>(Expr{NumberExpr{val}});
        } else if (peek().type == TokenType::IDENTIFIER) {
            std::string name = consume(TokenType::IDENTIFIER, "Expected identifier").value;
            return std::make_unique<Expr>(Expr{IdentifierExpr{name}});
        }
        throw std::runtime_error("Expected expression");
    }
};

class TypeChecker {
    std::map<std::string, std::string> env;
public:
    void check(const std::vector<StmtPtr>& stmts) {
        for (const auto& stmt : stmts) {
            check_stmt(*stmt);
        }
    }
private:
    void check_stmt(const Stmt& stmt) {
        if (auto* var = std::get_if<VarDeclStmt>(&stmt.value)) {
            auto inferred_type = infer_type(var->value.get());
            if (!var->type.empty() && var->type != inferred_type) {
                throw std::runtime_error("Type mismatch for " + var->name);
            }
            env[var->name] = var->type.empty() ? inferred_type : var->type;
        } else if (auto* if_stmt = std::get_if<IfStmt>(&stmt.value)) {
            for (const auto& then_stmt : if_stmt->then_branch) {
                check_stmt(*then_stmt);
            }
            for (const auto& else_stmt : if_stmt->else_branch) {
                check_stmt(*else_stmt);
            }
        } else if (auto* ret = std::get_if<ReturnStmt>(&stmt.value)) {
            (void)ret; // ignored
        }
    }

    std::string infer_type(const Expr* expr) {
        if (std::holds_alternative<NumberExpr>(expr->value)) {
            return "float";
        } else if (std::holds_alternative<IdentifierExpr>(expr->value)) {
            auto name = std::get<IdentifierExpr>(expr->value).name;
            if (env.find(name) != env.end()) return env[name];
            throw std::runtime_error("Undefined variable: " + name);
        }
        return "unknown";
    }
};

using Value = std::variant<double, std::string>;

class Interpreter {
    std::map<std::string, Value> env;
public:
    void run(const std::string& source) {
        Lexer lexer(source);
        auto tokens = lexer.tokenize();
        Parser parser(tokens);
        auto ast = parser.parse();
        TypeChecker checker;
        checker.check(ast);
        for (const auto& stmt : ast) {
            execute_stmt(*stmt);
        }
    }
private:
    void execute_stmt(const Stmt& stmt) {
        if (auto* var = std::get_if<VarDeclStmt>(&stmt.value)) {
            env[var->name] = evaluate_expr(*var->value);
        } else if (auto* if_stmt = std::get_if<IfStmt>(&stmt.value)) {
            auto condition_value = evaluate_expr(*if_stmt->condition);
            if (std::holds_alternative<double>(condition_value) && std::get<double>(condition_value) != 0) {
                for (const auto& then_stmt : if_stmt->then_branch) {
                    execute_stmt(*then_stmt);
                }
            } else {
                for (const auto& else_stmt : if_stmt->else_branch) {
                    execute_stmt(*else_stmt);
                }
            }
        } else if (auto* ret = std::get_if<ReturnStmt>(&stmt.value)) {
            (void)ret; // ignore
        }
    }

    Value evaluate_expr(const Expr& expr) {
        if (auto* num = std::get_if<NumberExpr>(&expr.value)) {
            return num->value;
        } else if (auto* id = std::get_if<IdentifierExpr>(&expr.value)) {
            if (env.find(id->name) != env.end()) return env[id->name];
            if (id->name == "print") return std::string("print");
            throw std::runtime_error("Undefined variable: " + id->name);
        } else if (auto* bin = std::get_if<BinaryExpr>(&expr.value)) {
            auto left = evaluate_expr(*bin->left);
            auto right = evaluate_expr(*bin->right);
            if (std::holds_alternative<double>(left) && std::holds_alternative<double>(right)) {
                double l = std::get<double>(left);
                double r = std::get<double>(right);
                switch (bin->op) {
                    case TokenType::PLUS: return l + r;
                    case TokenType::MINUS: return l - r;
                    case TokenType::STAR: return l * r;
                    case TokenType::SLASH: return l / r;
                    default: throw std::runtime_error("Unknown operator");
                }
            }
            throw std::runtime_error("Type mismatch in binary operation");
        }
        return Value{};
    }
};

void print(const Value& value) {
    if (std::holds_alternative<double>(value)) {
        std::cout << std::get<double>(value) << std::endl;
    } else if (std::holds_alternative<std::string>(value)) {
        std::cout << std::get<std::string>(value) << std::endl;
    }
}

void repl() {
    Interpreter interp;
    std::string line;
    while (true) {
        std::cout << "> ";
        std::getline(std::cin, line);
        if (line == "exit") break;
        try {
            interp.run(line);
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }
}

int main() {
    repl();
    return 0;
}

