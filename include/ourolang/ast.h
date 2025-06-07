#pragma once
#include "token.h"
#include <memory>
#include <variant>
#include <vector>
#include <string>

namespace ouro {

struct Expr;
struct Stmt;
using ExprPtr = std::unique_ptr<Expr>;
using StmtPtr = std::unique_ptr<Stmt>;

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

} // namespace ouro
