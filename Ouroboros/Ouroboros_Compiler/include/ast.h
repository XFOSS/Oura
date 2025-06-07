// ast.h
// Defines the structures for the Abstract Syntax Tree (AST) nodes for Ouroboros.

#ifndef OUROBOROS_AST_H
#define OUROBOROS_AST_H

#include "ouroboros/token.h" // Include Token definitions for literals and lexemes
#include <stdlib.h> // For size_t
#include <stdbool.h> // For bool type

// Forward declarations for recursive types
typedef struct ASTNode ASTNode;
typedef struct DeclarationNode DeclarationNode;
typedef struct StatementNode StatementNode;
typedef struct ExpressionNode ExpressionNode;

// --- Enums for AST Node Types ---

// Base type for all AST nodes
typedef enum {
    NODE_PROGRAM,
    NODE_DECLARATION,
    NODE_STATEMENT,
    NODE_EXPRESSION,
    NODE_LITERAL,
    NODE_IDENTIFIER,
    // Add more specific types as needed, e.g., NODE_TYPE_REFERENCE
} ASTNodeType;

// Specific types of declarations
typedef enum {
    DECL_VARIABLE,
    DECL_FUNCTION,
    DECL_CLASS,
    DECL_INTERFACE,
    DECL_ENUM,
    DECL_PACKAGE, // For package declarations
    DECL_IMPORT   // For import statements
} DeclarationType;

// Specific types of statements
typedef enum {
    STMT_BLOCK,
    STMT_EXPRESSION, // An expression used as a statement (e.g., function call)
    STMT_VAR_DECL,   // Variable declaration as a statement
    STMT_IF,
    STMT_WHILE,
    STMT_FOR,
    STMT_DO_WHILE,
    STMT_FOREACH,
    STMT_BREAK,
    STMT_CONTINUE,
    STMT_RETURN,
    STMT_THROW,      // Throw exception
    STMT_TRY_CATCH_FINALLY, // Try-catch-finally block
    // Add more as needed (e.g., STMT_SWITCH)
} StatementType;

// Specific types of expressions
typedef enum {
    EXPR_LITERAL,
    EXPR_IDENTIFIER,
    EXPR_BINARY,     // a + b, a == b, etc.
    EXPR_UNARY,      // !a, -a, ++a, a--
    EXPR_ASSIGNMENT, // a = b, a += b
    EXPR_CALL,       // function(args)
    EXPR_MEMBER_ACCESS, // obj.field or obj.method()
    EXPR_ARRAY_ACCESS,  // arr[index]
    EXPR_NEW_OBJECT, // new Class()
    EXPR_NEW_ARRAY, // new int[10]
    EXPR_CAST,       // (Type)expression
    EXPR_TERNARY,    // condition ? val1 : val2
    EXPR_REF_PARAM,  // For `ref` or `out` parameters
    // Add more as needed
} ExpressionType;

// --- AST Node Structures ---

// Base AST Node structure (all nodes embed this)
struct ASTNode {
    ASTNodeType base_type; // General category (DECLARATION, STATEMENT, EXPRESSION)
    Token* token;          // The token associated with this node (for line/column info)
    // Add common fields like source location, type info, etc.
};

// Node for a literal value (e.g., integer, string, boolean)
typedef struct {
    ASTNode base;
    TokenType literal_type; // INTEGER_LITERAL, STRING_LITERAL, etc.
    LiteralValue value;     // The actual literal value
} LiteralNode;

// Node for an identifier (e.g., variable name, function name)
typedef struct {
    ASTNode base;
    char* name; // The identifier name (dynamically allocated)
} IdentifierNode;

// Represents a type reference (e.g., "int", "String", "MyClass")
typedef struct {
    ASTNode base;
    char* name; // The type name (e.g., "int", "String", "MyClass")
    // Potentially add a list for generic type arguments: List<T>
} TypeReferenceNode;


// --- Expression Nodes ---

// Binary expression (e.g., a + b, x == y)
typedef struct {
    ASTNode base;
    ExpressionType expr_type; // EXPR_BINARY
    ASTNode* left;
    Token* operator; // Token representing the operator (+, -, ==, etc.)
    ASTNode* right;
} BinaryExpressionNode;

// Unary expression (e.g., !a, -b, ++c)
typedef struct {
    ASTNode base;
    ExpressionType expr_type; // EXPR_UNARY
    Token* operator; // Token representing the operator (!, -, ++, --)
    ASTNode* operand;
} UnaryExpressionNode;

// Assignment expression (e.g., a = b, x += y)
typedef struct {
    ASTNode base;
    ExpressionType expr_type; // EXPR_ASSIGNMENT
    ASTNode* target; // Left-hand side (e.g., IdentifierNode, MemberAccessNode)
    Token* operator; // Token representing the assignment operator (=, +=, etc.)
    ASTNode* value;  // Right-hand side
} AssignmentExpressionNode;

// Function/Method Call expression
typedef struct {
    ASTNode base;
    ExpressionType expr_type; // EXPR_CALL
    ASTNode* callee; // IdentifierNode or MemberAccessNode for the function being called
    ASTNode** arguments; // Array of ASTNode* for arguments
    size_t argument_count;
} CallExpressionNode;

// Member access (e.g., obj.field, obj.method)
typedef struct {
    ASTNode base;
    ExpressionType expr_type; // EXPR_MEMBER_ACCESS
    ASTNode* object; // The object on which the member is accessed
    IdentifierNode* member; // The name of the member (field or method)
} MemberAccessExpressionNode;

// Array access (e.g., arr[index])
typedef struct {
    ASTNode base;
    ExpressionType expr_type; // EXPR_ARRAY_ACCESS
    ASTNode* array; // The array expression
    ASTNode* index; // The index expression
} ArrayAccessExpressionNode;

// New object instantiation (e.g., new MyClass(args))
typedef struct {
    ASTNode base;
    ExpressionType expr_type; // EXPR_NEW_OBJECT
    TypeReferenceNode* class_type; // The type of the class being instantiated
    ASTNode** arguments; // Array of ASTNode* for constructor arguments
    size_t argument_count;
} NewObjectExpressionNode;

// New array instantiation (e.g., new int[10], new String[5][2])
typedef struct {
    ASTNode base;
    ExpressionType expr_type; // EXPR_NEW_ARRAY
    TypeReferenceNode* element_type; // Base type of array elements
    // List of dimension expressions, e.g., for `new int[dim1][dim2]`
    ASTNode** dimensions;
    size_t dimension_count;
} NewArrayExpressionNode;

// Cast expression (e.g., (int)myFloat)
typedef struct {
    ASTNode base;
    ExpressionType expr_type; // EXPR_CAST
    TypeReferenceNode* target_type; // The type to cast to
    ASTNode* operand; // The expression being casted
} CastExpressionNode;

// Ternary (conditional) expression (e.g., cond ? val_if_true : val_if_false)
typedef struct {
    ASTNode base;
    ExpressionType expr_type; // EXPR_TERNARY
    ASTNode* condition;
    ASTNode* true_expr;
    ASTNode* false_expr;
} TernaryExpressionNode;

// For `ref` or `out` parameters, marking an expression as passed by reference
typedef struct {
    ASTNode base;
    ExpressionType expr_type; // EXPR_REF_PARAM
    Token* modifier; // REF or OUT token
    ASTNode* operand; // The expression (e.g., identifier) being passed by reference
} RefOutExpressionNode;


// --- Statement Nodes ---

// Block statement (e.g., { ... statements ... })
typedef struct {
    ASTNode base;
    StatementType stmt_type; // STMT_BLOCK
    StatementNode** statements; // Array of StatementNode*
    size_t statement_count;
} BlockStatementNode;

// Expression statement (e.g., myFunc(); x = 5;)
typedef struct {
    ASTNode base;
    StatementType stmt_type; // STMT_EXPRESSION
    ExpressionNode* expression;
} ExpressionStatementNode;

// Variable declaration statement (e.g., int x = 10; var name = "Bob";)
typedef struct {
    ASTNode base;
    StatementType stmt_type; // STMT_VAR_DECL
    Token* modifier; // VAR, LET, CONST, FINAL (can be NULL)
    TypeReferenceNode* type; // Type of the variable (can be NULL for type inference)
    IdentifierNode* name;    // Name of the variable
    ExpressionNode* initializer; // Optional: Initial value expression (can be NULL)
} VariableDeclarationStatementNode;

// If-Else-If-Else statement
typedef struct {
    ASTNode base;
    StatementType stmt_type; // STMT_IF
    ExpressionNode* condition;
    StatementNode* then_branch; // The 'if' block
    StatementNode* else_branch; // Optional: The 'else if' or 'else' block (can be another IfStatementNode for else if)
} IfStatementNode;

// While loop statement
typedef struct {
    ASTNode base;
    StatementType stmt_type; // STMT_WHILE
    ExpressionNode* condition;
    StatementNode* body;
} WhileStatementNode;

// For loop statement (C-style)
typedef struct {
    ASTNode base;
    StatementType stmt_type; // STMT_FOR
    StatementNode* initializer; // VariableDeclarationStatementNode or ExpressionStatementNode (can be NULL)
    ExpressionNode* condition;  // Can be NULL for infinite loop
    ExpressionNode* incrementer; // Can be NULL
    StatementNode* body;
} ForStatementNode;

// Do-While loop statement
typedef struct {
    ASTNode base;
    StatementType stmt_type; // STMT_DO_WHILE
    StatementNode* body;
    ExpressionNode* condition;
} DoWhileStatementNode;

// For-each loop statement
typedef struct {
    ASTNode base;
    StatementType stmt_type; // STMT_FOREACH
    TypeReferenceNode* item_type; // Type of the item (can be NULL for var/let)
    IdentifierNode* item_name; // Name of the item variable
    ExpressionNode* collection; // Expression for the collection to iterate over
    StatementNode* body;
} ForEachStatementNode;

// Break and Continue statements
typedef struct {
    ASTNode base;
    StatementType stmt_type; // STMT_BREAK or STMT_CONTINUE
    IdentifierNode* label; // Optional label for labeled break/continue (can be NULL)
} JumpStatementNode;

// Return statement
typedef struct {
    ASTNode base;
    StatementType stmt_type; // STMT_RETURN
    ExpressionNode* value; // Optional: Expression to return (can be NULL for void functions)
} ReturnStatementNode;

// Throw statement
typedef struct {
    ASTNode base;
    StatementType stmt_type; // STMT_THROW
    ExpressionNode* exception; // The exception object to throw
} ThrowStatementNode;

// Try-Catch-Finally statement
typedef struct {
    ASTNode base;
    StatementType stmt_type; // STMT_TRY_CATCH_FINALLY
    BlockStatementNode* try_block;
    // Array of CatchClauseNode for multiple catch blocks
    struct CatchClauseNode {
        TypeReferenceNode* exception_type; // Type of exception caught
        IdentifierNode* variable_name; // Name of the exception variable
        BlockStatementNode* catch_block;
    }** catch_clauses;
    size_t catch_clause_count;
    BlockStatementNode* finally_block; // Optional: can be NULL
} TryCatchFinallyStatementNode;


// --- Declaration Nodes ---

// Function/Method parameter
typedef struct {
    TypeReferenceNode* type;
    IdentifierNode* name;
    Token* modifier; // REF or OUT (can be NULL)
    ExpressionNode* default_value; // Optional default value (can be NULL)
} ParameterNode;

// Base for function/method declaration
typedef struct {
    ASTNode base;
    DeclarationType decl_type; // DECL_FUNCTION
    Token* access_modifier; // PUBLIC, PRIVATE, PROTECTED, INTERNAL (can be NULL)
    Token* static_modifier; // STATIC (can be NULL)
    TypeReferenceNode* return_type; // VOID or actual type
    IdentifierNode* name;
    ParameterNode** parameters; // Array of ParameterNode*
    size_t parameter_count;
    // List of TypeReferenceNode for 'throws' clause
    TypeReferenceNode** thrown_exceptions;
    size_t thrown_exception_count;
    BlockStatementNode* body; // Function body
} FunctionDeclarationNode;

// Field (member variable) declaration within a class/interface
typedef struct {
    ASTNode base;
    DeclarationType decl_type; // DECL_VARIABLE (used within class context)
    Token* access_modifier; // PUBLIC, PRIVATE, PROTECTED, INTERNAL (can be NULL)
    Token* static_modifier; // STATIC (can be NULL)
    Token* immutability_modifier; // CONST, FINAL (can be NULL)
    TypeReferenceNode* type;
    IdentifierNode* name;
    ExpressionNode* initializer; // Optional initial value
} FieldDeclarationNode;

// Base for class/interface declaration
typedef struct {
    ASTNode base;
    DeclarationType decl_type; // DECL_CLASS or DECL_INTERFACE
    Token* access_modifier; // PUBLIC, PRIVATE, PROTECTED, INTERNAL (can be NULL)
    IdentifierNode* name;
    TypeReferenceNode** super_classes; // For 'extends' (single inheritance for classes, multiple for interfaces)
    size_t super_class_count;
    TypeReferenceNode** implemented_interfaces; // For 'implements'
    size_t implemented_interface_count;
    // Array of ASTNode* for members (FieldDeclarationNode, FunctionDeclarationNode, ConstructorNode)
    ASTNode** members;
    size_t member_count;
} ClassInterfaceDeclarationNode;

// Constructor declaration
typedef struct {
    ASTNode base;
    DeclarationType decl_type; // Special type for constructor
    // No explicit return type for constructors
    IdentifierNode* name; // Name of the class/constructor
    ParameterNode** parameters;
    size_t parameter_count;
    BlockStatementNode* body;
    // Potential for constructor chaining (this() or super())
} ConstructorDeclarationNode;

// Enum Declaration Node (placeholder for now)
typedef struct {
    ASTNode base;
    DeclarationType decl_type; // DECL_ENUM
    IdentifierNode* name;
    // List of enum values (identifiers or identifiers with initializers)
    // For simplicity, just identifiers for now
    IdentifierNode** enum_values;
    size_t enum_value_count;
} EnumDeclarationNode;


// Package declaration node
typedef struct {
    ASTNode base;
    DeclarationType decl_type; // DECL_PACKAGE
    IdentifierNode* package_name; // Dotted identifier (e.g., "com.ourolang.lang")
} PackageDeclarationNode;

// Import declaration node
typedef struct {
    ASTNode base;
    DeclarationType decl_type; // DECL_IMPORT
    IdentifierNode* imported_name; // Dotted identifier (e.g., "java.util.List" or "java.util.*")
    bool is_wildcard_import; // True if it's a `.*` import
} ImportDeclarationNode;

// Top-level program node
typedef struct {
    ASTNode base;
    ASTNode** declarations; // Array of DeclarationNode* (package, import, classes, functions)
    size_t declaration_count;
} ProgramNode;


// --- Helper Functions for AST Node Creation (to be implemented in ast.c) ---
// These will help the parser allocate and initialize AST nodes correctly.

// Base node creation
ASTNode* ast_create_node(ASTNodeType type, Token* token);

// Literal node creation
LiteralNode* ast_create_literal_node(Token* token, TokenType literal_type, LiteralValue value);

// Identifier node creation
IdentifierNode* ast_create_identifier_node(Token* token, const char* name);

// Type reference node creation
TypeReferenceNode* ast_create_type_reference_node(Token* token, const char* name);

// Expression node creation examples
BinaryExpressionNode* ast_create_binary_expr_node(ASTNode* left, Token* op, ASTNode* right, Token* token);
UnaryExpressionNode* ast_create_unary_expr_node(Token* op, ASTNode* operand, Token* token);
AssignmentExpressionNode* ast_create_assignment_expr_node(ASTNode* target, Token* op, ASTNode* value, Token* token);
CallExpressionNode* ast_create_call_expr_node(ASTNode* callee, ASTNode** args, size_t arg_count, Token* token);
MemberAccessExpressionNode* ast_create_member_access_expr_node(ASTNode* object, IdentifierNode* member, Token* token);
ArrayAccessExpressionNode* ast_create_array_access_expr_node(ASTNode* array, ASTNode* index, Token* token);
NewObjectExpressionNode* ast_create_new_object_expr_node(TypeReferenceNode* class_type, ASTNode** args, size_t arg_count, Token* token);
NewArrayExpressionNode* ast_create_new_array_expr_node(TypeReferenceNode* element_type, ASTNode** dimensions, size_t dim_count, Token* token);
CastExpressionNode* ast_create_cast_expr_node(TypeReferenceNode* target_type, ASTNode* operand, Token* token);
TernaryExpressionNode* ast_create_ternary_expr_node(ASTNode* condition, ASTNode* true_expr, ASTNode* false_expr, Token* token);
RefOutExpressionNode* ast_create_ref_out_expr_node(Token* modifier, ASTNode* operand, Token* token);

// Statement node creation examples
BlockStatementNode* ast_create_block_stmt_node(StatementNode** statements, size_t count, Token* token);
ExpressionStatementNode* ast_create_expr_stmt_node(ExpressionNode* expr, Token* token);
VariableDeclarationStatementNode* ast_create_var_decl_stmt_node(Token* modifier, TypeReferenceNode* type, IdentifierNode* name, ExpressionNode* initializer, Token* token);
IfStatementNode* ast_create_if_stmt_node(ExpressionNode* condition, StatementNode* then_b, StatementNode* else_b, Token* token);
WhileStatementNode* ast_create_while_stmt_node(ExpressionNode* condition, StatementNode* body, Token* token);
ForStatementNode* ast_create_for_stmt_node(StatementNode* init, ExpressionNode* cond, ExpressionNode* incr, StatementNode* body, Token* token);
DoWhileStatementNode* ast_create_do_while_stmt_node(StatementNode* body, ExpressionNode* cond, Token* token);
ForEachStatementNode* ast_create_foreach_stmt_node(TypeReferenceNode* item_type, IdentifierNode* item_name, ExpressionNode* collection, StatementNode* body, Token* token);
JumpStatementNode* ast_create_jump_stmt_node(TokenType type, IdentifierNode* label, Token* token); // For break/continue
ReturnStatementNode* ast_create_return_stmt_node(ExpressionNode* value, Token* token);
ThrowStatementNode* ast_create_throw_stmt_node(ExpressionNode* exception, Token* token);
TryCatchFinallyStatementNode* ast_create_try_catch_finally_stmt_node(BlockStatementNode* try_b, struct CatchClauseNode** catch_c, size_t cc_count, BlockStatementNode* finally_b, Token* token);
// Helper for creating catch clauses
struct CatchClauseNode* ast_create_catch_clause(TypeReferenceNode* exception_type, IdentifierNode* variable_name, BlockStatementNode* catch_block);

// Declaration node creation examples
ParameterNode* ast_create_parameter_node(TypeReferenceNode* type, IdentifierNode* name, Token* modifier, ExpressionNode* default_value);
FunctionDeclarationNode* ast_create_function_decl_node(Token* access_mod, Token* static_mod, TypeReferenceNode* return_type, IdentifierNode* name, ParameterNode** params, size_t param_count, TypeReferenceNode** thrown_exc, size_t thrown_exc_count, BlockStatementNode* body, Token* token);
FieldDeclarationNode* ast_create_field_decl_node(Token* access_mod, Token* static_mod, Token* immut_mod, TypeReferenceNode* type, IdentifierNode* name, ExpressionNode* initializer, Token* token);
ClassInterfaceDeclarationNode* ast_create_class_interface_decl_node(DeclarationType decl_type, Token* access_mod, IdentifierNode* name, TypeReferenceNode** super_cls, size_t super_cls_count, TypeReferenceNode** impl_interfaces, size_t impl_int_count, ASTNode** members, size_t member_count, Token* token);
ConstructorDeclarationNode* ast_create_constructor_decl_node(IdentifierNode* name, ParameterNode** params, size_t param_count, BlockStatementNode* body, Token* token);
EnumDeclarationNode* ast_create_enum_decl_node(IdentifierNode* name, IdentifierNode** enum_values, size_t enum_value_count, Token* token);
PackageDeclarationNode* ast_create_package_decl_node(IdentifierNode* package_name, Token* token);
ImportDeclarationNode* ast_create_import_decl_node(IdentifierNode* imported_name, bool is_wildcard, Token* token);
ProgramNode* ast_create_program_node(ASTNode** declarations, size_t decl_count, Token* token);


// --- Functions for AST Memory Management (to be implemented in ast.c) ---
void ast_free_node(ASTNode* node); // Recursively frees an AST node and its children
void ast_free_program(ProgramNode* program_node); // Specific for freeing the whole AST

#endif // OUROBOROS_AST_H
