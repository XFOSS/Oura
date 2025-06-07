#ifndef OUROBOROS_AST_H
#define OUROBOROS_AST_H

#include <stddef.h>
#include <stdbool.h>

// Forward declarations
typedef struct Token Token;

// Node types
typedef enum {
    NODE_PROGRAM,
    NODE_EXPRESSION,
    NODE_PACKAGE_DECLARATION,
    NODE_IMPORT_DECLARATION,
    NODE_CLASS_INTERFACE_DECLARATION,
    NODE_FUNCTION_DECLARATION,
    NODE_FIELD_DECLARATION,
    NODE_CONSTRUCTOR_DECLARATION,
    NODE_ENUM_DECLARATION,
    NODE_STATEMENT,
    NODE_TYPE_REFERENCE,
    NODE_IDENTIFIER,
    NODE_PARAMETER
} ASTNodeType;

// Statement types
typedef enum {
    STMT_BLOCK,
    STMT_EXPRESSION
} StatementType;

// Expression types
typedef enum {
    EXPR_LITERAL,
    EXPR_IDENTIFIER,
    EXPR_UNARY,
    EXPR_BINARY,
    EXPR_ASSIGNMENT,
    EXPR_CALL,
    EXPR_MEMBER_ACCESS,
    EXPR_ARRAY_ACCESS,
    EXPR_TERNARY
} ExpressionType;

// Literal types
typedef enum {
    BOOLEAN_LITERAL,
    INTEGER_LITERAL,
    FLOATING_POINT_LITERAL,
    STRING_LITERAL,
    CHARACTER_LITERAL,
    NULL_LITERAL
} LiteralType;

// Base AST node structure
typedef struct ASTNode {
    ASTNodeType base_type;
    StatementType statement_type;  // Only valid for NODE_STATEMENT
} ASTNode;

// Program node
typedef struct ProgramNode {
    ASTNode base;
    ASTNode** declarations;
    size_t declaration_count;
} ProgramNode;

// Expression node
typedef struct ExpressionNode {
    ASTNode base;
    ExpressionType expr_type;
} ExpressionNode;

// Identifier node
typedef struct IdentifierNode {
    ExpressionNode base;
    char* name;
} IdentifierNode;

// Literal node
typedef struct LiteralNode {
    ExpressionNode base;
    LiteralType value_type;
    union {
        bool boolean_val;
        long long integer_val;
        double float_val;
        char* string_val;
        char char_val;
    } value;
} LiteralNode;

// Binary expression node
typedef struct BinaryExpressionNode {
    ExpressionNode base;
    ExpressionNode* left;
    ExpressionNode* right;
    Token* operator;
} BinaryExpressionNode;

// Unary expression node
typedef struct UnaryExpressionNode {
    ExpressionNode base;
    ExpressionNode* operand;
    Token* operator;
} UnaryExpressionNode;

// Assignment expression node
typedef struct AssignmentExpressionNode {
    ExpressionNode base;
    ExpressionNode* target;
    ExpressionNode* value;
    Token* operator;
} AssignmentExpressionNode;

// Call expression node
typedef struct CallExpressionNode {
    ExpressionNode base;
    ExpressionNode* callee;
    ExpressionNode** arguments;
    size_t argument_count;
} CallExpressionNode;

// Member access expression node
typedef struct MemberAccessExpressionNode {
    ExpressionNode base;
    ExpressionNode* object;
    IdentifierNode* member_name;
} MemberAccessExpressionNode;

// Array access expression node
typedef struct ArrayAccessExpressionNode {
    ExpressionNode base;
    ExpressionNode* array;
    ExpressionNode* index;
} ArrayAccessExpressionNode;

// Ternary expression node
typedef struct TernaryExpressionNode {
    ExpressionNode base;
    ExpressionNode* condition;
    ExpressionNode* true_expr;
    ExpressionNode* false_expr;
} TernaryExpressionNode;

// Block statement node
typedef struct BlockStatementNode {
    ASTNode base;
    ASTNode** statements;
    size_t statement_count;
} BlockStatementNode;

// Expression statement node
typedef struct ExpressionStatementNode {
    ASTNode base;
    ExpressionNode* expression;
} ExpressionStatementNode;

// Package declaration node
typedef struct PackageDeclarationNode {
    ASTNode base;
    IdentifierNode* package_name;
} PackageDeclarationNode;

// Import declaration node
typedef struct ImportDeclarationNode {
    ASTNode base;
    IdentifierNode* imported_name;
    bool is_wildcard;
} ImportDeclarationNode;

// Class/Interface declaration node
typedef struct ClassInterfaceDeclarationNode {
    ASTNode base;
    IdentifierNode* name;
    ASTNode** members;
    size_t member_count;
} ClassInterfaceDeclarationNode;

// Function declaration node
typedef struct FunctionDeclarationNode {
    ASTNode base;
    IdentifierNode* name;
    ASTNode* body;
} FunctionDeclarationNode;

// Field declaration node
typedef struct FieldDeclarationNode {
    ASTNode base;
    IdentifierNode* type;
    IdentifierNode* name;
    ExpressionNode* initializer;
} FieldDeclarationNode;

// Constructor declaration node
typedef struct ConstructorDeclarationNode {
    ASTNode base;
    IdentifierNode* class_name;
    ASTNode* body;
} ConstructorDeclarationNode;

// Enum declaration node
typedef struct EnumDeclarationNode {
    ASTNode base;
    IdentifierNode* name;
} EnumDeclarationNode;

// Helper function to convert node type to string
const char* ast_node_type_to_string(ASTNodeType type);

#endif // OUROBOROS_AST_H 