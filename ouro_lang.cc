// ouro_lang.cc
#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <variant>
#include <memory>
#include <stdexcept>
#include <chrono>
#include <thread>
#include <future>
#include <cctype>

// ### Token Definition
enum class TokenType {
    LET, FN, IF, ELSE, RETURN, FOR, IN, ASYNC, AWAIT, GPU,
    INT, FLOAT, STRING, IDENTIFIER, NUMBER, STRING_LITERAL,
    COLON, EQUALS, LPAREN, RPAREN, LBRACE, RBRACE, SEMICOLON, COMMA,
    PLUS, MINUS, MUL, DIV, GT, DOTDOT, ARROW, EOF_TOKEN
};

struct Token {
    TokenType type;
    std::string value;
    int line;
};

// ### Lexer
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
            } else if (c == '"') {
                tokens.push_back(parse_string());
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
        if (value == "for") return {TokenType::FOR, value, line};
        if (value == "in") return {TokenType::IN, value, line};
        if (value == "async") return {TokenType::ASYNC, value, line};
        if (value == "await") return {TokenType::AWAIT, value, line};
        if (value == "gpu") return {TokenType::GPU, value, line};
        if (value == "int") return {TokenType::INT, value, line};
        if (value == "float") return {TokenType::FLOAT, value, line};
        if (value == "string") return {TokenType::STRING, value, line};
        return {TokenType::IDENTIFIER, value, line};
    }

    Token parse_number() {
        std::string value;
        bool has_dot = false;
        while (pos < source.size() && (std::isdigit(static_cast<unsigned char>(source[pos])) || source[pos] == '.')) {
            if (source[pos] == '.') has_dot = true;
            value += source[pos++];
        }
        return {TokenType::NUMBER, value, line};
    }

    Token parse_string() {
        std::string value;
        pos++; // Skip opening quote
        while (pos < source.size() && source[pos] != '"') {
            value += source[pos++];
        }
        pos++; // Skip closing quote
        return {TokenType::STRING_LITERAL, value, line};
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
            case ',': return {TokenType::COMMA, ",", line};
            case '+': return {TokenType::PLUS, "+", line};
            case '-':
                if (pos < source.size() && source[pos] == '>') {
                    pos++;
                    return {TokenType::ARROW, "->", line};
                }
                return {TokenType::MINUS, "-", line};
            case '*': return {TokenType::MUL, "*", line};
            case '/': return {TokenType::DIV, "/", line};
            case '>': return {TokenType::GT, ">", line};
            case '.':
                if (pos < source.size() && source[pos] == '.') {
                    pos++;
                    return {TokenType::DOTDOT, "..", line};
                }
                break;
        }
        throw std::runtime_error("Unknown symbol at line " + std::to_string(line));
    }
};

// ### AST Nodes
struct Expr;
struct Stmt;
using ExprPtr = std::unique_ptr<Expr>;
using StmtPtr = std::unique_ptr<Stmt>;

// #### Expressions
struct NumberExpr { double value; };
struct StringExpr { std::string value; };
struct IdentExpr { std::string name; };
struct BinaryExpr { TokenType op; ExprPtr left; ExprPtr right; };
struct CallExpr { std::string name; std::vector<ExprPtr> args; };
struct AwaitExpr { ExprPtr expr; };

using ExprVariant = std::variant<NumberExpr, StringExpr, IdentExpr, BinaryExpr, CallExpr, AwaitExpr>;
struct Expr { 
    ExprVariant value;
    template <typename T>
    explicit Expr(T&& v) : value(std::forward<T>(v)) {}
};

// #### Statements
struct VarDeclStmt { std::string name; std::string type; ExprPtr value; };
struct FnDeclStmt {
    std::string name;
    std::vector<std::pair<std::string, std::string>> params;
    std::string return_type;
    std::vector<StmtPtr> body;
    bool is_async;
    bool is_gpu;
    bool is_generic;
    std::vector<std::string> generic_params;
};
struct IfStmt { ExprPtr condition; std::vector<StmtPtr> then_branch; std::vector<StmtPtr> else_branch; };
struct ForStmt { std::string var; ExprPtr start; ExprPtr end; std::vector<StmtPtr> body; };
struct ReturnStmt { ExprPtr value; };

using StmtVariant = std::variant<VarDeclStmt, FnDeclStmt, IfStmt, ForStmt, ReturnStmt>;
struct Stmt {
    StmtVariant value;
    template <typename T>
    explicit Stmt(T&& v) : value(std::forward<T>(v)) {}
};

// ### Parser
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
        if (peek().type == TokenType::AWAIT) {
            consume(TokenType::AWAIT, "Expected 'await'");
            auto expr = parse_expr();
            return std::make_unique<Expr>(AwaitExpr{std::move(expr)});
        }
        throw std::runtime_error("Expected expression at line " + std::to_string(peek().line));
    }
};

// ### Type Checker
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

// ### Interpreter
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

// ### REPL
void repl() {
    Interpreter interp;
    std::string line;
    std::cout << "OuroLang REPL (type 'exit' to quit)\n";
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

// ### Main
int main() {
    repl();
    return 0;
}
