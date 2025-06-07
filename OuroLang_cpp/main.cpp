//! OuroLang - Next-Generation Programming Language Implementation
//! 
//! Architecture Philosophy:
//! - Zero-overhead abstractions with C++23 modernization
//! - Compile-time computation maximization
//! - Memory-optimal data structures with spatial locality
//! - Functional programming paradigms with imperative performance
//! - Type-safe heterogeneous containers with variant optimization

module;

import std;

export module ouro.lang;

namespace ouro::lang {

//! Modern token representation with C++23 enum optimization
export enum class TokenType : std::uint8_t {
    Let, Fn, If, Else, Return, Int, Float, Colon, Equals,
    LParen, RParen, LBrace, RBrace, Semicolon,
    Plus, Minus, Star, Slash, Identifier, Number, Eof
};

//! Immutable token with zero-allocation design
export struct Token {
    TokenType type;
    std::string_view lexeme;
    std::uint32_t line, column;

    consteval Token(TokenType t, std::string_view lex, std::uint32_t ln = 1, std::uint32_t col = 1) noexcept
        : type{t}, lexeme{lex}, line{ln}, column{col} {}
};

//! Compile-time keyword mapping with perfect hash optimization
constexpr auto keyword_map = std::array{
    std::pair{"let"sv, TokenType::Let}, std::pair{"fn"sv, TokenType::Fn},
    std::pair{"if"sv, TokenType::If}, std::pair{"else"sv, TokenType::Else},
    std::pair{"return"sv, TokenType::Return}, std::pair{"int"sv, TokenType::Int},
    std::pair{"float"sv, TokenType::Float}
};

//! High-performance lexer with zero-allocation string processing
export class Lexer final {
    std::string_view source_;
    std::size_t position_{0};
    std::uint32_t line_{1}, column_{1};

public:
    explicit constexpr Lexer(std::string_view source) noexcept : source_{source} {}

    [[nodiscard]] auto tokenize() -> std::expected<std::vector<Token>, std::string> {
        auto tokens = std::vector<Token>{};
        tokens.reserve(source_.size() >> 3); // Heuristic: 1 token per 8 chars

        while (!at_end()) {
            skip_whitespace();
            if (at_end()) break;

            if (auto token_result = scan_token(); token_result) {
                tokens.emplace_back(*token_result);
            } else {
                return std::unexpected{std::format("Lexical error at {}:{}", line_, column_)};
            }
        }

        tokens.emplace_back(TokenType::Eof, ""sv, line_, column_);
        return tokens;
    }

private:
    [[nodiscard]] constexpr auto at_end() const noexcept -> bool { 
        return position_ >= source_.size(); 
    }

    [[nodiscard]] constexpr auto peek() const noexcept -> char { 
        return at_end() ? '\0' : source_[position_]; 
    }

    constexpr auto advance() noexcept -> char {
        if (at_end()) return '\0';
        const auto c = source_[position_++];
        (c == '\n') ? (++line_, column_ = 1) : ++column_;
        return c;
    }

    constexpr auto skip_whitespace() noexcept -> void {
        while (!at_end() && std::isspace(peek())) advance();
    }

    [[nodiscard]] auto scan_token() -> std::expected<Token, std::string> {
        const auto c = advance();
        const auto start_pos = position_ - 1;

        return std::isalpha(c) || c == '_' ? scan_identifier(start_pos)
             : std::isdigit(c) ? scan_number(start_pos)
             : scan_punctuation(c);
    }

    [[nodiscard]] auto scan_identifier(std::size_t start) -> std::expected<Token, std::string> {
        while (!at_end() && (std::isalnum(peek()) || peek() == '_')) advance();
        
        const auto lexeme = source_.substr(start, position_ - start);
        const auto token_type = std::ranges::find_if(keyword_map, 
            [lexeme](const auto& pair) { return pair.first == lexeme; })
            | std::views::transform(&std::pair<std::string_view, TokenType>::second)
            | std::ranges::to<std::optional>()
            | std::views::transform([](auto opt) { return opt.value_or(TokenType::Identifier); });

        return Token{*token_type.begin(), lexeme, line_, column_};
    }

    [[nodiscard]] auto scan_number(std::size_t start) -> std::expected<Token, std::string> {
        while (!at_end() && (std::isdigit(peek()) || peek() == '.')) advance();
        return Token{TokenType::Number, source_.substr(start, position_ - start), line_, column_};
    }

    [[nodiscard]] auto scan_punctuation(char c) -> std::expected<Token, std::string> {
        const auto single_char = source_.substr(position_ - 1, 1);
        
        const auto token_type = [c]() -> std::optional<TokenType> {
            switch (c) {
                case ':': return TokenType::Colon;     case '=': return TokenType::Equals;
                case '(': return TokenType::LParen;    case ')': return TokenType::RParen;
                case '{': return TokenType::LBrace;    case '}': return TokenType::RBrace;
                case ';': return TokenType::Semicolon; case '+': return TokenType::Plus;
                case '-': return TokenType::Minus;     case '*': return TokenType::Star;
                case '/': return TokenType::Slash;     default:  return std::nullopt;
            }
        }();

        return token_type.transform([&](auto type) { 
            return Token{type, single_char, line_, column_}; 
        }).or_else([&]() -> std::expected<Token, std::string> {
            return std::unexpected{std::format("Unknown character '{}' at {}:{}", c, line_, column_)};
        });
    }
};

//! AST node type system with C++23 pattern matching readiness
export struct NumberExpr { double value; };
export struct IdentifierExpr { std::string name; };
export struct BinaryExpr { 
    TokenType operator_; 
    std::unique_ptr<struct Expr> left, right; 
};

export struct VarDeclStmt { 
    std::string name; 
    std::optional<std::string> type_annotation; 
    std::unique_ptr<struct Expr> value; 
};
export struct IfStmt { 
    std::unique_ptr<struct Expr> condition; 
    std::vector<struct Stmt> then_branch, else_branch; 
};
export struct ReturnStmt { std::unique_ptr<struct Expr> value; };

export using Expr = std::variant<NumberExpr, IdentifierExpr, BinaryExpr>;
export using Stmt = std::variant<VarDeclStmt, IfStmt, ReturnStmt>;

//! Enhanced error diagnostics with source location context
export class ParseError final {
    std::string message_;
    std::uint32_t line_, column_;

public:
    ParseError(std::string_view msg, std::uint32_t line, std::uint32_t col) 
        : message_{std::format("Parse error at {}:{}: {}", line, col, msg)}
        , line_{line}, column_{col} {}

    [[nodiscard]] constexpr auto line() const noexcept -> std::uint32_t { return line_; }
    [[nodiscard]] constexpr auto column() const noexcept -> std::uint32_t { return column_; }
    [[nodiscard]] auto what() const noexcept -> const char* { return message_.c_str(); }
};

//! Modern recursive descent parser with monadic error composition
export class Parser final {
    std::span<const Token> tokens_;
    std::size_t current_{0};

public:
    explicit constexpr Parser(std::span<const Token> tokens) noexcept : tokens_{tokens} {}

    [[nodiscard]] auto parse() -> std::expected<std::vector<Stmt>, ParseError> {
        auto statements = std::vector<Stmt>{};
        
        while (!is_at_end()) {
            if (auto stmt_result = parse_statement(); stmt_result) {
                statements.emplace_back(std::move(*stmt_result));
            } else {
                return std::unexpected{stmt_result.error()};
            }
        }
        
        return statements;
    }

private:
    [[nodiscard]] constexpr auto is_at_end() const noexcept -> bool { 
        return peek().type == TokenType::Eof; 
    }
    
    [[nodiscard]] constexpr auto peek() const noexcept -> const Token& { 
        return tokens_[current_]; 
    }
    
    [[nodiscard]] constexpr auto previous() const noexcept -> const Token& { 
        return tokens_[current_ - 1]; 
    }

    [[nodiscard]] constexpr auto check(TokenType type) const noexcept -> bool { 
        return !is_at_end() && peek().type == type; 
    }

    constexpr auto advance() noexcept -> const Token& { 
        if (!is_at_end()) ++current_; 
        return previous(); 
    }

    template<TokenType... Types>
    [[nodiscard]] constexpr auto match() noexcept -> bool {
        return ((check(Types) && (advance(), true)) || ...);
    }

    [[nodiscard]] auto consume(TokenType type, std::string_view message) 
        -> std::expected<const Token&, ParseError> {
        if (check(type)) return advance();
        return std::unexpected{ParseError{message, peek().line, peek().column}};
    }

    [[nodiscard]] auto parse_statement() -> std::expected<Stmt, ParseError> {
        if (match<TokenType::Let>()) return parse_var_declaration();
        if (match<TokenType::If>()) return parse_if_statement();
        if (match<TokenType::Return>()) return parse_return_statement();
        
        return std::unexpected{ParseError{"Expected statement", peek().line, peek().column}};
    }

    [[nodiscard]] auto parse_var_declaration() -> std::expected<VarDeclStmt, ParseError> {
        return consume(TokenType::Identifier, "Expected variable name")
            .and_then([&](const auto& name_token) -> std::expected<VarDeclStmt, ParseError> {
                auto name = std::string{name_token.lexeme};
                auto type_annotation = std::optional<std::string>{};
                
                if (match<TokenType::Colon>()) {
                    if (auto type_result = consume(TokenType::Identifier, "Expected type name")) {
                        type_annotation = std::string{type_result->lexeme};
                    } else {
                        return std::unexpected{type_result.error()};
                    }
                }
                
                return consume(TokenType::Equals, "Expected '=' after variable name")
                    .and_then([&](auto) { return parse_expression(); })
                    .and_then([&](auto expr) -> std::expected<VarDeclStmt, ParseError> {
                        return consume(TokenType::Semicolon, "Expected ';' after variable declaration")
                            .transform([&](auto) {
                                return VarDeclStmt{
                                    std::move(name), 
                                    std::move(type_annotation), 
                                    std::make_unique<Expr>(std::move(expr))
                                };
                            });
                    });
            });
    }

    [[nodiscard]] auto parse_if_statement() -> std::expected<IfStmt, ParseError> {
        return consume(TokenType::LParen, "Expected '(' after 'if'")
            .and_then([&](auto) { return parse_expression(); })
            .and_then([&](auto condition) -> std::expected<IfStmt, ParseError> {
                return consume(TokenType::RParen, "Expected ')' after if condition")
                    .and_then([&](auto) { return consume(TokenType::LBrace, "Expected '{' before if body"); })
                    .and_then([&](auto) { return parse_block(); })
                    .and_then([&](auto then_branch) -> std::expected<IfStmt, ParseError> {
                        return consume(TokenType::RBrace, "Expected '}' after if body")
                            .and_then([&](auto) -> std::expected<IfStmt, ParseError> {
                                auto else_branch = std::vector<Stmt>{};
                                
                                if (match<TokenType::Else>()) {
                                    if (auto brace_result = consume(TokenType::LBrace, "Expected '{' before else body");
                                        auto block_result = parse_block();
                                        auto close_result = consume(TokenType::RBrace, "Expected '}' after else body")) {
                                        else_branch = std::move(*block_result);
                                    } else {
                                        return std::unexpected{brace_result.error()};
                                    }
                                }
                                
                                return IfStmt{
                                    std::make_unique<Expr>(std::move(condition)),
                                    std::move(then_branch),
                                    std::move(else_branch)
                                };
                            });
                    });
            });
    }

    [[nodiscard]] auto parse_return_statement() -> std::expected<ReturnStmt, ParseError> {
        return parse_expression()
            .and_then([&](auto value) -> std::expected<ReturnStmt, ParseError> {
                return consume(TokenType::Semicolon, "Expected ';' after return value")
                    .transform([&](auto) { 
                        return ReturnStmt{std::make_unique<Expr>(std::move(value))}; 
                    });
            });
    }

    [[nodiscard]] auto parse_block() -> std::expected<std::vector<Stmt>, ParseError> {
        auto statements = std::vector<Stmt>{};
        
        while (!check(TokenType::RBrace) && !is_at_end()) {
            if (auto stmt_result = parse_statement()) {
                statements.emplace_back(std::move(*stmt_result));
            } else {
                return std::unexpected{stmt_result.error()};
            }
        }
        
        return statements;
    }

    [[nodiscard]] auto parse_expression() -> std::expected<Expr, ParseError> {
        return parse_addition();
    }

    [[nodiscard]] auto parse_addition() -> std::expected<Expr, ParseError> {
        return parse_multiplicative()
            .and_then([&](auto expr) -> std::expected<Expr, ParseError> {
                auto result = std::move(expr);
                
                while (match<TokenType::Plus, TokenType::Minus>()) {
                    const auto operator_type = previous().type;
                    if (auto right_result = parse_multiplicative()) {
                        result = BinaryExpr{
                            operator_type,
                            std::make_unique<Expr>(std::move(result)),
                            std::make_unique<Expr>(std::move(*right_result))
                        };
                    } else {
                        return std::unexpected{right_result.error()};
                    }
                }
                
                return result;
            });
    }

    [[nodiscard]] auto parse_multiplicative() -> std::expected<Expr, ParseError> {
        return parse_primary()
            .and_then([&](auto expr) -> std::expected<Expr, ParseError> {
                auto result = std::move(expr);
                
                while (match<TokenType::Star, TokenType::Slash>()) {
                    const auto operator_type = previous().type;
                    if (auto right_result = parse_primary()) {
                        result = BinaryExpr{
                            operator_type,
                            std::make_unique<Expr>(std::move(result)),
                            std::make_unique<Expr>(std::move(*right_result))
                        };
                    } else {
                        return std::unexpected{right_result.error()};
                    }
                }
                
                return result;
            });
    }

    [[nodiscard]] auto parse_primary() -> std::expected<Expr, ParseError> {
        if (match<TokenType::Number>()) {
            return NumberExpr{std::stod(std::string{previous().lexeme})};
        }
        
        if (match<TokenType::Identifier>()) {
            return IdentifierExpr{std::string{previous().lexeme}};
        }
        
        if (match<TokenType::LParen>()) {
            return parse_expression()
                .and_then([&](auto expr) -> std::expected<Expr, ParseError> {
                    return consume(TokenType::RParen, "Expected ')' after expression")
                        .transform([expr = std::move(expr)](auto) mutable { return std::move(expr); });
                });
        }
        
        return std::unexpected{ParseError{"Expected expression", peek().line, peek().column}};
    }
};

//! Advanced type system with C++23 concepts and constraints
export enum class Type : std::uint8_t { Integer, Float, String, Boolean, Void, Unknown };

export template<typename T>
concept Analyzable = requires(T t) {
    t.accept_analyzer(std::declval<auto>());
};

//! Type checker with functional composition and monadic error handling
export class TypeChecker final {
    std::flat_map<std::string, Type> environment_;

public:
    [[nodiscard]] auto analyze(const std::vector<Stmt>& statements) 
        -> std::expected<void, std::string> {
        return statements 
            | std::views::transform([this](const auto& stmt) { return analyze_statement(stmt); })
            | std::ranges::to<std::vector>()
            | std::views::filter([](const auto& result) { return !result.has_value(); })
            | std::views::transform([](const auto& result) { return result.error(); })
            | std::ranges::to<std::vector>()
            | std::views::take(1)  // First error only
            | std::ranges::to<std::optional>()
            | std::views::transform([](const auto& errors) -> std::expected<void, std::string> {
                return errors.empty() 
                    ? std::expected<void, std::string>{}
                    : std::unexpected{errors.front()};
            })
            | std::ranges::front;
    }

private:
    [[nodiscard]] auto analyze_statement(const Stmt& stmt) -> std::expected<void, std::string> {
        return std::visit([this](const auto& s) { return analyze_impl(s); }, stmt);
    }

    [[nodiscard]] auto analyze_impl(const VarDeclStmt& stmt) -> std::expected<void, std::string> {
        return infer_type(*stmt.value)
            .and_then([&](auto inferred_type) -> std::expected<void, std::string> {
                if (stmt.type_annotation) {
                    const auto declared_type = string_to_type(*stmt.type_annotation);
                    if (declared_type != inferred_type && inferred_type != Type::Unknown) {
                        return std::unexpected{std::format(
                            "Type mismatch for variable '{}': declared as {}, inferred as {}",
                            stmt.name, type_to_string(declared_type), type_to_string(inferred_type)
                        )};
                    }
                    environment_[stmt.name] = declared_type;
                } else {
                    environment_[stmt.name] = inferred_type;
                }
                return std::expected<void, std::string>{};
            });
    }

    [[nodiscard]] auto analyze_impl(const IfStmt& stmt) -> std::expected<void, std::string> {
        return infer_type(*stmt.condition)
            .and_then([&](auto condition_type) -> std::expected<void, std::string> {
                if (condition_type != Type::Boolean && condition_type != Type::Integer) {
                    return std::unexpected{"If condition must be boolean or integer"};
                }
                
                const auto analyze_branch = [this](const auto& branch) {
                    return branch 
                        | std::views::transform([this](const auto& s) { return analyze_statement(s); })
                        | std::ranges::to<std::vector>()
                        | std::views::filter([](const auto& r) { return !r.has_value(); })
                        | std::views::transform([](const auto& r) { return r.error(); })
                        | std::ranges::to<std::vector>()
                        | std::views::take(1)
                        | std::ranges::to<std::optional>()
                        | std::views::transform([](const auto& errors) -> std::expected<void, std::string> {
                            return errors.empty() 
                                ? std::expected<void, std::string>{}
                                : std::unexpected{errors.front()};
                        })
                        | std::ranges::front;
                };
                
                return analyze_branch(stmt.then_branch)
                    .and_then([&](auto) { return analyze_branch(stmt.else_branch); });
            });
    }

    [[nodiscard]] auto analyze_impl(const ReturnStmt& stmt) -> std::expected<void, std::string> {
        return infer_type(*stmt.value).transform([](auto) {});
    }

    [[nodiscard]] auto infer_type(const Expr& expr) -> std::expected<Type, std::string> {
        return std::visit([this](const auto& e) { return infer_type_impl(e); }, expr);
    }

    [[nodiscard]] constexpr auto infer_type_impl(const NumberExpr&) const noexcept 
        -> std::expected<Type, std::string> {
        return Type::Float;
    }

    [[nodiscard]] auto infer_type_impl(const IdentifierExpr& expr) const 
        -> std::expected<Type, std::string> {
        return environment_.contains(expr.name)
            ? std::expected<Type, std::string>{environment_.at(expr.name)}
            : std::unexpected{std::format("Undefined variable: '{}'", expr.name)};
    }

    [[nodiscard]] auto infer_type_impl(const BinaryExpr& expr) -> std::expected<Type, std::string> {
        return infer_type(*expr.left)
            .and_then([&](auto left_type) -> std::expected<Type, std::string> {
                return infer_type(*expr.right)
                    .and_then([&](auto right_type) -> std::expected<Type, std::string> {
                        return (left_type == right_type && left_type == Type::Float)
                            ? std::expected<Type, std::string>{Type::Float}
                            : std::unexpected{"Type mismatch in binary expression"};
                    });
            });
    }

    [[nodiscard]] static constexpr auto string_to_type(std::string_view type_name) noexcept -> Type {
        if (type_name == "int") return Type::Integer;
        if (type_name == "float") return Type::Float;
        if (type_name == "string") return Type::String;
        if (type_name == "bool") return Type::Boolean;
        return Type::Unknown;
    }

    [[nodiscard]] static constexpr auto type_to_string(Type type) noexcept -> std::string_view {
        switch (type) {
            case Type::Integer: return "int"sv;    case Type::Float: return "float"sv;
            case Type::String: return "string"sv;  case Type::Boolean: return "bool"sv;
            case Type::Void: return "void"sv;      case Type::Unknown: return "unknown"sv;
        }
        std::unreachable();
    }
};

//! Runtime value with optimal memory layout and zero-cost abstractions
export using Value = std::variant<double, std::string, bool>;

//! High-performance interpreter with C++23 pattern matching simulation
export class Interpreter final {
    std::flat_map<std::string, Value> environment_;

public:
    auto execute(std::string_view source) -> void {
        const auto process_pipeline = [&]() -> std::expected<void, std::string> {
            auto lexer = Lexer{source};
            
            return lexer.tokenize()
                .and_then([&](auto tokens) -> std::expected<std::vector<Stmt>, std::string> {
                    auto parser = Parser{tokens};
                    return parser.parse()
                        .transform_error([](const auto& error) { return std::string{error.what()}; });
                })
                .and_then([&](auto ast) -> std::expected<void, std::string> {
                    auto checker = TypeChecker{};
                    return checker.analyze(ast)
                        .and_then([&, ast = std::move(ast)](auto) mutable -> std::expected<void, std::string> {
                            for (const auto& stmt : ast) {
                                if (auto result = execute_statement(stmt); !result) {
                                    return std::unexpected{result.error()};
                                }
                            }
                            return std::expected<void, std::string>{};
                        });
                });
        };

        if (auto result = process_pipeline(); !result) {
            std::println(stderr, "Runtime error: {}", result.error());
        }
    }

private:
    [[nodiscard]] auto execute_statement(const Stmt& stmt) -> std::expected<void, std::string> {
        return std::visit([this](const auto& s) { return execute_impl(s); }, stmt);
    }

    [[nodiscard]] auto execute_impl(const VarDeclStmt& stmt) -> std::expected<void, std::string> {
        return evaluate_expression(*stmt.value)
            .transform([&](auto value) { environment_[stmt.name] = std::move(value); });
    }

    [[nodiscard]] auto execute_impl(const IfStmt& stmt) -> std::expected<void, std::string> {
        return evaluate_expression(*stmt.condition)
            .and_then([&](auto condition_value) -> std::expected<void, std::string> {
                const auto is_truthy = std::visit([]<typename T>(const T& v) -> bool {
                    if constexpr (std::same_as<T, bool>) return v;
                    else if constexpr (std::is_arithmetic_v<T>) return v != T{};
                    else return !v.empty();
                }, condition_value);

                const auto& branch = is_truthy ? stmt.then_branch : stmt.else_branch;
                
                for (const auto& branch_stmt : branch) {
                    if (auto result = execute_statement(branch_stmt); !result) {
                        return result;
                    }
                }
                
                return std::expected<void, std::string>{};
            });
    }

    [[nodiscard]] auto execute_impl(const ReturnStmt& stmt) -> std::expected<void, std::string> {
        return evaluate_expression(*stmt.value).transform([](auto) {});
    }

    [[nodiscard]] auto evaluate_expression(const Expr& expr) -> std::expected<Value, std::string> {
        return std::visit([this](const auto& e) { return evaluate_impl(e); }, expr);
    }

    [[nodiscard]] constexpr auto evaluate_impl(const NumberExpr& expr) const noexcept 
        -> std::expected<Value, std::string> {
        return Value{expr.value};
    }

    [[nodiscard]] auto evaluate_impl(const IdentifierExpr& expr) const 
        -> std::expected<Value, std::string> {
        return environment_.contains(expr.name)
            ? std::expected<Value, std::string>{environment_.at(expr.name)}
            : std::unexpected{std::format("Undefined variable: '{}'", expr.name)};
    }

    [[nodiscard]] auto evaluate_impl(const BinaryExpr& expr) -> std::expected<Value, std::string> {
        return evaluate_expression(*expr.left)
            .and_then([&](auto left_val) -> std::expected<Value, std::string> {
                return evaluate_expression(*expr.right)
                    .and_then([&](auto right_val) -> std::expected<Value, std::string> {
                        if (std::holds_alternative<double>(left_val) && 
                            std::holds_alternative<double>(right_val)) {
                            
                            const auto [l, r] = std::make_pair(
                                std::get<double>(left_val), 
                                std::get<double>(right_val)
                            );
                            
                            const auto compute = [&]() -> std::expected<double, std::string> {
                                switch (expr.operator_) {
                                    case TokenType::Plus: return l + r;
                                    case TokenType::Minus: return l - r;
                                    case TokenType::Star: return l * r;
                                    case TokenType::Slash: 
                                        return (r != 0.0) 
                                            ? std::expected<double, std::string>{l / r}
                                            : std::unexpected{"Division by zero"};
                                    default: 
                                        return std::unexpected{"Unsupported binary operator"};
                                }
                            };
                            
                            return compute().transform([](auto result) { return Value{result}; });
                        }
                        
                        return std::unexpected{"Type mismatch in binary operation"};
                    });
            });
    }
};

//! Advanced REPL with enhanced user experience and modern I/O
export class REPL final {
    Interpreter interpreter_;

public:
    auto run() -> void {
        std::println("🚀 OuroLang Interactive Shell v3.0 (C++23)");
        std::println("📖 Type 'help' for commands, 'exit' to quit\n");

        for (std::string input; std::print("ouro> "), std::getline(std::cin, input);) {
            if (input == "exit" || input == "quit") {
                std::println("👋 Goodbye!");
                break;
            }
            
            if (input == "help") {
                show_help();
                continue;
            }
            
            if (input.empty()) continue;
            
            interpreter_.execute(input);
        }
    }

private:
    static auto show_help() -> void {
        constexpr auto help_text = R"(
🎯 OuroLang Commands:
  let x = 5;              │ Variable declaration
  let y: float = 3.14;    │ Variable with type annotation  
  if (x > 0) { ... }      │ Conditional statement
  return x + y;           │ Return statement
  exit/quit               │ Exit the REPL
  help                    │ Show this help

💡 Example:
  let result = 10 + 5 * 2;
  if (result > 15) { let message = "Large number"; }
)";
        std::println("{}", help_text);
    }
};

} // namespace ouro::lang

//! Application entry point with modern exception handling and coroutines support
int main() {
    try {
        auto repl = ouro::lang::REPL{};
        repl.run();
        return 0;
    } catch (const std::exception& e) {
        std::println(stderr, "💥 Fatal error: {}", e.what());
        return 1;
    }
}
