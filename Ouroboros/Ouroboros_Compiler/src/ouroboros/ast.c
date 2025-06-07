// ast.c
// Implements functions for creating and managing AST nodes.

#include "ouroboros/ast.h"
#include "ouroboros/token.h" // For free_token()
#include <stdio.h>   // For fprintf
#include <stdlib.h>  // For malloc, free, exit
#include <string.h>  // For strdup, strlen

// --- Internal Helper for Memory Management ---

/**
 * Safely duplicates a string. Exits on memory allocation failure.
 */
static char* ast_safe_strdup(const char* s) {
    if (s == NULL) return NULL;
    char* new_s = strdup(s); // strdup allocates and copies
    if (new_s == NULL) {
        fprintf(stderr, "Fatal Error: Memory allocation failed for string duplication in AST.\n");
        exit(EXIT_FAILURE);
    }
    return new_s;
}

/**
 * Safely allocates memory for an array of pointers. Exits on failure.
 */
static void* ast_safe_malloc_array(size_t count, size_t element_size) {
    if (count == 0) return NULL; // No allocation needed for empty arrays
    void* ptr = calloc(count, element_size); // calloc initializes to zero
    if (ptr == NULL) {
        fprintf(stderr, "Fatal Error: Memory allocation failed for AST array.\n");
        exit(EXIT_FAILURE);
    }
    return ptr;
}


// --- AST Node Creation Functions ---

/**
 * Base node creation function. All specific creation functions will call this.
 */
ASTNode* ast_create_node(ASTNodeType type, Token* token) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    if (node == NULL) {
        fprintf(stderr, "Fatal Error: Memory allocation failed for ASTNode.\n");
        exit(EXIT_FAILURE);
    }
    node->base_type = type;
    node->token = token; // The AST owns the token, no duplication needed here
    return node;
}

LiteralNode* ast_create_literal_node(Token* token, TokenType literal_type, LiteralValue value) {
    LiteralNode* node = (LiteralNode*)malloc(sizeof(LiteralNode));
    if (node == NULL) { /* handle error */ }
    node->base.base_type = NODE_LITERAL;
    node->base.token = token;
    node->literal_type = literal_type;
    node->value = value; // Copy union value directly

    // If it's a string literal, the string_val within the union also needs duplication
    if (literal_type == STRING_LITERAL && value.string_val != NULL) {
        node->value.string_val = ast_safe_strdup(value.string_val);
    }
    return node;
}

IdentifierNode* ast_create_identifier_node(Token* token, const char* name) {
    IdentifierNode* node = (IdentifierNode*)malloc(sizeof(IdentifierNode));
    if (node == NULL) { /* handle error */ }
    node->base.base_type = NODE_IDENTIFIER;
    node->base.token = token;
    node->name = ast_safe_strdup(name);
    return node;
}

TypeReferenceNode* ast_create_type_reference_node(Token* token, const char* name) {
    TypeReferenceNode* node = (TypeReferenceNode*)malloc(sizeof(TypeReferenceNode));
    if (node == NULL) { /* handle error */ }
    node->base.base_type = NODE_EXPRESSION; // Or a specific NODE_TYPE_REFERENCE
    node->base.token = token;
    node->name = ast_safe_strdup(name);
    // Initialize generic type arguments if any were added
    return node;
}


// --- Expression Node Creation ---

BinaryExpressionNode* ast_create_binary_expr_node(ASTNode* left, Token* op, ASTNode* right, Token* token) {
    BinaryExpressionNode* node = (BinaryExpressionNode*)malloc(sizeof(BinaryExpressionNode));
    if (node == NULL) { /* handle error */ }
    node->base.base_type = NODE_EXPRESSION;
    node->base.token = token; // The token for the overall expression (e.g., the operator token)
    node->expr_type = EXPR_BINARY;
    node->left = left;
    node->operator = op;
    node->right = right;
    return node;
}

UnaryExpressionNode* ast_create_unary_expr_node(Token* op, ASTNode* operand, Token* token) {
    UnaryExpressionNode* node = (UnaryExpressionNode*)malloc(sizeof(UnaryExpressionNode));
    if (node == NULL) { /* handle error */ }
    node->base.base_type = NODE_EXPRESSION;
    node->base.token = token;
    node->expr_type = EXPR_UNARY;
    node->operator = op;
    node->operand = operand;
    return node;
}

AssignmentExpressionNode* ast_create_assignment_expr_node(ASTNode* target, Token* op, ASTNode* value, Token* token) {
    AssignmentExpressionNode* node = (AssignmentExpressionNode*)malloc(sizeof(AssignmentExpressionNode));
    if (node == NULL) { /* handle error */ }
    node->base.base_type = NODE_EXPRESSION;
    node->base.token = token;
    node->expr_type = EXPR_ASSIGNMENT;
    node->target = target;
    node->operator = op;
    node->value = value;
    return node;
}

CallExpressionNode* ast_create_call_expr_node(ASTNode* callee, ASTNode** args, size_t arg_count, Token* token) {
    CallExpressionNode* node = (CallExpressionNode*)malloc(sizeof(CallExpressionNode));
    if (node == NULL) { /* handle error */ }
    node->base.base_type = NODE_EXPRESSION;
    node->base.token = token;
    node->expr_type = EXPR_CALL;
    node->callee = callee;
    node->argument_count = arg_count;
    node->arguments = (ASTNode**)ast_safe_malloc_array(arg_count, sizeof(ASTNode*));
    for (size_t i = 0; i < arg_count; ++i) {
        node->arguments[i] = args[i];
    }
    return node;
}

MemberAccessExpressionNode* ast_create_member_access_expr_node(ASTNode* object, IdentifierNode* member, Token* token) {
    MemberAccessExpressionNode* node = (MemberAccessExpressionNode*)malloc(sizeof(MemberAccessExpressionNode));
    if (node == NULL) { /* handle error */ }
    node->base.base_type = NODE_EXPRESSION;
    node->base.token = token;
    node->expr_type = EXPR_MEMBER_ACCESS;
    node->object = object;
    node->member = member;
    return node;
}

ArrayAccessExpressionNode* ast_create_array_access_expr_node(ASTNode* array, ASTNode* index, Token* token) {
    ArrayAccessExpressionNode* node = (ArrayAccessExpressionNode*)malloc(sizeof(ArrayAccessExpressionNode));
    if (node == NULL) { /* handle error */ }
    node->base.base_type = NODE_EXPRESSION;
    node->base.token = token;
    node->expr_type = EXPR_ARRAY_ACCESS;
    node->array = array;
    node->index = index;
    return node;
}

NewObjectExpressionNode* ast_create_new_object_expr_node(TypeReferenceNode* class_type, ASTNode** args, size_t arg_count, Token* token) {
    NewObjectExpressionNode* node = (NewObjectExpressionNode*)malloc(sizeof(NewObjectExpressionNode));
    if (node == NULL) { /* handle error */ }
    node->base.base_type = NODE_EXPRESSION;
    node->base.token = token;
    node->expr_type = EXPR_NEW_OBJECT;
    node->class_type = class_type;
    node->argument_count = arg_count;
    node->arguments = (ASTNode**)ast_safe_malloc_array(arg_count, sizeof(ASTNode*));
    for (size_t i = 0; i < arg_count; ++i) {
        node->arguments[i] = args[i];
    }
    return node;
}

NewArrayExpressionNode* ast_create_new_array_expr_node(TypeReferenceNode* element_type, ASTNode** dimensions, size_t dim_count, Token* token) {
    NewArrayExpressionNode* node = (NewArrayExpressionNode*)malloc(sizeof(NewArrayExpressionNode));
    if (node == NULL) { /* handle error */ }
    node->base.base_type = NODE_EXPRESSION;
    node->base.token = token;
    node->expr_type = EXPR_NEW_ARRAY;
    node->element_type = element_type;
    node->dimension_count = dim_count;
    node->dimensions = (ASTNode**)ast_safe_malloc_array(dim_count, sizeof(ASTNode*));
    for (size_t i = 0; i < dim_count; ++i) {
        node->dimensions[i] = dimensions[i];
    }
    return node;
}

CastExpressionNode* ast_create_cast_expr_node(TypeReferenceNode* target_type, ASTNode* operand, Token* token) {
    CastExpressionNode* node = (CastExpressionNode*)malloc(sizeof(CastExpressionNode));
    if (node == NULL) { /* handle error */ }
    node->base.base_type = NODE_EXPRESSION;
    node->base.token = token;
    node->expr_type = EXPR_CAST;
    node->target_type = target_type;
    node->operand = operand;
    return node;
}

TernaryExpressionNode* ast_create_ternary_expr_node(ASTNode* condition, ASTNode* true_expr, ASTNode* false_expr, Token* token) {
    TernaryExpressionNode* node = (TernaryExpressionNode*)malloc(sizeof(TernaryExpressionNode));
    if (node == NULL) { /* handle error */ }
    node->base.base_type = NODE_EXPRESSION;
    node->base.token = token;
    node->expr_type = EXPR_TERNARY;
    node->condition = condition;
    node->true_expr = true_expr;
    node->false_expr = false_expr;
    return node;
}

RefOutExpressionNode* ast_create_ref_out_expr_node(Token* modifier, ASTNode* operand, Token* token) {
    RefOutExpressionNode* node = (RefOutExpressionNode*)malloc(sizeof(RefOutExpressionNode));
    if (node == NULL) { /* handle error */ }
    node->base.base_type = NODE_EXPRESSION;
    node->base.token = token;
    node->expr_type = EXPR_REF_PARAM;
    node->modifier = modifier;
    node->operand = operand;
    return node;
}


// --- Statement Node Creation ---

BlockStatementNode* ast_create_block_stmt_node(StatementNode** statements, size_t count, Token* token) {
    BlockStatementNode* node = (BlockStatementNode*)malloc(sizeof(BlockStatementNode));
    if (node == NULL) { /* handle error */ }
    node->base.base_type = NODE_STATEMENT;
    node->base.token = token;
    node->stmt_type = STMT_BLOCK;
    node->statement_count = count;
    node->statements = (StatementNode**)ast_safe_malloc_array(count, sizeof(StatementNode*));
    for (size_t i = 0; i < count; ++i) {
        node->statements[i] = statements[i];
    }
    return node;
}

ExpressionStatementNode* ast_create_expr_stmt_node(ExpressionNode* expr, Token* token) {
    ExpressionStatementNode* node = (ExpressionStatementNode*)malloc(sizeof(ExpressionStatementNode));
    if (node == NULL) { /* handle error */ }
    node->base.base_type = NODE_STATEMENT;
    node->base.token = token;
    node->stmt_type = STMT_EXPRESSION;
    node->expression = expr;
    return node;
}

VariableDeclarationStatementNode* ast_create_var_decl_stmt_node(Token* modifier, TypeReferenceNode* type, IdentifierNode* name, ExpressionNode* initializer, Token* token) {
    VariableDeclarationStatementNode* node = (VariableDeclarationStatementNode*)malloc(sizeof(VariableDeclarationStatementNode));
    if (node == NULL) { /* handle error */ }
    node->base.base_type = NODE_STATEMENT;
    node->base.token = token;
    node->stmt_type = STMT_VAR_DECL;
    node->modifier = modifier;
    node->type = type;
    node->name = name;
    node->initializer = initializer;
    return node;
}

IfStatementNode* ast_create_if_stmt_node(ExpressionNode* condition, StatementNode* then_b, StatementNode* else_b, Token* token) {
    IfStatementNode* node = (IfStatementNode*)malloc(sizeof(IfStatementNode));
    if (node == NULL) { /* handle error */ }
    node->base.base_type = NODE_STATEMENT;
    node->base.token = token;
    node->stmt_type = STMT_IF;
    node->condition = condition;
    node->then_branch = then_b;
    node->else_branch = else_b;
    return node;
}

WhileStatementNode* ast_create_while_stmt_node(ExpressionNode* condition, StatementNode* body, Token* token) {
    WhileStatementNode* node = (WhileStatementNode*)malloc(sizeof(WhileStatementNode));
    if (node == NULL) { /* handle error */ }
    node->base.base_type = NODE_STATEMENT;
    node->base.token = token;
    node->stmt_type = STMT_WHILE;
    node->condition = condition;
    node->body = body;
    return node;
}

ForStatementNode* ast_create_for_stmt_node(StatementNode* init, ExpressionNode* cond, ExpressionNode* incr, StatementNode* body, Token* token) {
    ForStatementNode* node = (ForStatementNode*)malloc(sizeof(ForStatementNode));
    if (node == NULL) { /* handle error */ }
    node->base.base_type = NODE_STATEMENT;
    node->base.token = token;
    node->stmt_type = STMT_FOR;
    node->initializer = init;
    node->condition = cond;
    node->incrementer = incr;
    node->body = body;
    return node;
}

DoWhileStatementNode* ast_create_do_while_stmt_node(StatementNode* body, ExpressionNode* cond, Token* token) {
    DoWhileStatementNode* node = (DoWhileStatementNode*)malloc(sizeof(DoWhileStatementNode));
    if (node == NULL) { /* handle error */ }
    node->base.base_type = NODE_STATEMENT;
    node->base.token = token;
    node->stmt_type = STMT_DO_WHILE;
    node->body = body;
    node->condition = cond;
    return node;
}

ForEachStatementNode* ast_create_foreach_stmt_node(TypeReferenceNode* item_type, IdentifierNode* item_name, ExpressionNode* collection, StatementNode* body, Token* token) {
    ForEachStatementNode* node = (ForEachStatementNode*)malloc(sizeof(ForEachStatementNode));
    if (node == NULL) { /* handle error */ }
    node->base.base_type = NODE_STATEMENT;
    node->base.token = token;
    node->stmt_type = STMT_FOREACH;
    node->item_type = item_type;
    node->item_name = item_name;
    node->collection = collection;
    node->body = body;
    return node;
}

JumpStatementNode* ast_create_jump_stmt_node(TokenType type, IdentifierNode* label, Token* token) {
    JumpStatementNode* node = (JumpStatementNode*)malloc(sizeof(JumpStatementNode));
    if (node == NULL) { /* handle error */ }
    node->base.base_type = NODE_STATEMENT;
    node->base.token = token;
    node->stmt_type = (type == BREAK) ? STMT_BREAK : STMT_CONTINUE;
    node->label = label;
    return node;
}

ReturnStatementNode* ast_create_return_stmt_node(ExpressionNode* value, Token* token) {
    ReturnStatementNode* node = (ReturnStatementNode*)malloc(sizeof(ReturnStatementNode));
    if (node == NULL) { /* handle error */ }
    node->base.base_type = NODE_STATEMENT;
    node->base.token = token;
    node->stmt_type = STMT_RETURN;
    node->value = value;
    return node;
}

ThrowStatementNode* ast_create_throw_stmt_node(ExpressionNode* exception, Token* token) {
    ThrowStatementNode* node = (ThrowStatementNode*)malloc(sizeof(ThrowStatementNode));
    if (node == NULL) { /* handle error */ }
    node->base.base_type = NODE_STATEMENT;
    node->base.token = token;
    node->stmt_type = STMT_THROW;
    node->exception = exception;
    return node;
}

struct CatchClauseNode* ast_create_catch_clause(TypeReferenceNode* exception_type, IdentifierNode* variable_name, BlockStatementNode* catch_block) {
    struct CatchClauseNode* clause = (struct CatchClauseNode*)malloc(sizeof(struct CatchClauseNode));
    if (clause == NULL) { /* handle error */ }
    clause->exception_type = exception_type;
    clause->variable_name = variable_name;
    clause->catch_block = catch_block;
    return clause;
}

TryCatchFinallyStatementNode* ast_create_try_catch_finally_stmt_node(BlockStatementNode* try_b, struct CatchClauseNode** catch_c, size_t cc_count, BlockStatementNode* finally_b, Token* token) {
    TryCatchFinallyStatementNode* node = (TryCatchFinallyStatementNode*)malloc(sizeof(TryCatchFinallyStatementNode));
    if (node == NULL) { /* handle error */ }
    node->base.base_type = NODE_STATEMENT;
    node->base.token = token;
    node->stmt_type = STMT_TRY_CATCH_FINALLY;
    node->try_block = try_b;
    node->catch_clause_count = cc_count;
    node->catch_clauses = (struct CatchClauseNode**)ast_safe_malloc_array(cc_count, sizeof(struct CatchClauseNode*));
    for (size_t i = 0; i < cc_count; ++i) {
        node->catch_clauses[i] = catch_c[i];
    }
    node->finally_block = finally_b;
    return node;
}


// --- Declaration Node Creation ---

ParameterNode* ast_create_parameter_node(TypeReferenceNode* type, IdentifierNode* name, Token* modifier, ExpressionNode* default_value) {
    ParameterNode* node = (ParameterNode*)malloc(sizeof(ParameterNode));
    if (node == NULL) { /* handle error */ }
    node->type = type;
    node->name = name;
    node->modifier = modifier;
    node->default_value = default_value;
    return node;
}

FunctionDeclarationNode* ast_create_function_decl_node(Token* access_mod, Token* static_mod, TypeReferenceNode* return_type, IdentifierNode* name, ParameterNode** params, size_t param_count, TypeReferenceNode** thrown_exc, size_t thrown_exc_count, BlockStatementNode* body, Token* token) {
    FunctionDeclarationNode* node = (FunctionDeclarationNode*)malloc(sizeof(FunctionDeclarationNode));
    if (node == NULL) { /* handle error */ }
    node->base.base_type = NODE_DECLARATION;
    node->base.token = token;
    node->decl_type = DECL_FUNCTION;
    node->access_modifier = access_mod;
    node->static_modifier = static_mod;
    node->return_type = return_type;
    node->name = name;
    node->parameter_count = param_count;
    node->parameters = (ParameterNode**)ast_safe_malloc_array(param_count, sizeof(ParameterNode*));
    for (size_t i = 0; i < param_count; ++i) {
        node->parameters[i] = params[i];
    }
    node->thrown_exception_count = thrown_exc_count;
    node->thrown_exceptions = (TypeReferenceNode**)ast_safe_malloc_array(thrown_exc_count, sizeof(TypeReferenceNode*));
    for (size_t i = 0; i < thrown_exc_count; ++i) {
        node->thrown_exceptions[i] = thrown_exc[i];
    }
    node->body = body;
    return node;
}

FieldDeclarationNode* ast_create_field_decl_node(Token* access_mod, Token* static_mod, Token* immut_mod, TypeReferenceNode* type, IdentifierNode* name, ExpressionNode* initializer, Token* token) {
    FieldDeclarationNode* node = (FieldDeclarationNode*)malloc(sizeof(FieldDeclarationNode));
    if (node == NULL) { /* handle error */ }
    node->base.base_type = NODE_DECLARATION;
    node->base.token = token;
    node->decl_type = DECL_VARIABLE; // Represents a field in this context
    node->access_modifier = access_mod;
    node->static_modifier = static_mod;
    node->immutability_modifier = immut_mod;
    node->type = type;
    node->name = name;
    node->initializer = initializer;
    return node;
}

ClassInterfaceDeclarationNode* ast_create_class_interface_decl_node(DeclarationType decl_type, Token* access_mod, IdentifierNode* name, TypeReferenceNode** super_cls, size_t super_cls_count, TypeReferenceNode** impl_interfaces, size_t impl_int_count, ASTNode** members, size_t member_count, Token* token) {
    ClassInterfaceDeclarationNode* node = (ClassInterfaceDeclarationNode*)malloc(sizeof(ClassInterfaceDeclarationNode));
    if (node == NULL) { /* handle error */ }
    node->base.base_type = NODE_DECLARATION;
    node->base.token = token;
    node->decl_type = decl_type;
    node->access_modifier = access_mod;
    node->name = name;
    node->super_class_count = super_cls_count;
    node->super_classes = (TypeReferenceNode**)ast_safe_malloc_array(super_cls_count, sizeof(TypeReferenceNode*));
    for (size_t i = 0; i < super_cls_count; ++i) {
        node->super_classes[i] = super_cls[i];
    }
    node->implemented_interface_count = impl_int_count;
    node->implemented_interfaces = (TypeReferenceNode**)ast_safe_malloc_array(impl_int_count, sizeof(TypeReferenceNode*));
    for (size_t i = 0; i < impl_int_count; ++i) {
        node->implemented_interfaces[i] = impl_interfaces[i];
    }
    node->member_count = member_count;
    node->members = (ASTNode**)ast_safe_malloc_array(member_count, sizeof(ASTNode*));
    for (size_t i = 0; i < member_count; ++i) {
        node->members[i] = members[i];
    }
    return node;
}

ConstructorDeclarationNode* ast_create_constructor_decl_node(IdentifierNode* name, ParameterNode** params, size_t param_count, BlockStatementNode* body, Token* token) {
    ConstructorDeclarationNode* node = (ConstructorDeclarationNode*)malloc(sizeof(ConstructorDeclarationNode));
    if (node == NULL) { /* handle error */ }
    node->base.base_type = NODE_DECLARATION; // Or a specific type like NODE_CONSTRUCTOR
    node->base.token = token;
    node->decl_type = DECL_FUNCTION; // Treat as a special function for now
    node->name = name;
    node->parameter_count = param_count;
    node->parameters = (ParameterNode**)ast_safe_malloc_array(param_count, sizeof(ParameterNode*));
    for (size_t i = 0; i < param_count; ++i) {
        node->parameters[i] = params[i];
    }
    node->body = body;
    return node;
}

EnumDeclarationNode* ast_create_enum_decl_node(IdentifierNode* name, IdentifierNode** enum_values, size_t enum_value_count, Token* token) {
    EnumDeclarationNode* node = (EnumDeclarationNode*)malloc(sizeof(EnumDeclarationNode));
    if (node == NULL) { /* handle error */ }
    node->base.base_type = NODE_DECLARATION;
    node->base.token = token;
    node->decl_type = DECL_ENUM;
    node->name = name;
    node->enum_value_count = enum_value_count;
    node->enum_values = (IdentifierNode**)ast_safe_malloc_array(enum_value_count, sizeof(IdentifierNode*));
    for (size_t i = 0; i < enum_value_count; ++i) {
        node->enum_values[i] = enum_values[i];
    }
    return node;
}

PackageDeclarationNode* ast_create_package_decl_node(IdentifierNode* package_name, Token* token) {
    PackageDeclarationNode* node = (PackageDeclarationNode*)malloc(sizeof(PackageDeclarationNode));
    if (node == NULL) { /* handle error */ }
    node->base.base_type = NODE_DECLARATION;
    node->base.token = token;
    node->decl_type = DECL_PACKAGE;
    node->package_name = package_name;
    return node;
}

ImportDeclarationNode* ast_create_import_decl_node(IdentifierNode* imported_name, bool is_wildcard, Token* token) {
    ImportDeclarationNode* node = (ImportDeclarationNode*)malloc(sizeof(ImportDeclarationNode));
    if (node == NULL) { /* handle error */ }
    node->base.base_type = NODE_DECLARATION;
    node->base.token = token;
    node->decl_type = DECL_IMPORT;
    node->imported_name = imported_name;
    node->is_wildcard_import = is_wildcard;
    return node;
}

ProgramNode* ast_create_program_node(ASTNode** declarations, size_t decl_count, Token* token) {
    ProgramNode* node = (ProgramNode*)malloc(sizeof(ProgramNode));
    if (node == NULL) { /* handle error */ }
    node->base.base_type = NODE_PROGRAM;
    node->base.token = token; // Can be NULL or the first token of the program
    node->declaration_count = decl_count;
    node->declarations = (ASTNode**)ast_safe_malloc_array(decl_count, sizeof(ASTNode*));
    for (size_t i = 0; i < decl_count; ++i) {
        node->declarations[i] = declarations[i];
    }
    return node;
}


// --- AST Node Freeing Functions (Recursive) ---

/**
 * Recursively frees an AST node and its children.
 * Uses a switch-case based on the base_type to determine the specific node structure.
 */
void ast_free_node(ASTNode* node) {
    if (node == NULL) return;

    // Free the token associated with this node (as parser takes ownership)
    if (node->token != NULL) {
        free_token(node->token);
    }

    // Use a switch to free specific node types and their children
    switch (node->base_type) {
        case NODE_LITERAL: {
            LiteralNode* literal_node = (LiteralNode*)node;
            if (literal_node->literal_type == STRING_LITERAL && literal_node->value.string_val != NULL) {
                free(literal_node->value.string_val);
            }
            break;
        }
        case NODE_IDENTIFIER: {
            IdentifierNode* identifier_node = (IdentifierNode*)node;
            if (identifier_node->name != NULL) {
                free(identifier_node->name);
            }
            break;
        }
        case NODE_EXPRESSION: {
            ExpressionNode* expr_node = (ExpressionNode*)node; // Cast to base ExpressionNode for type checking
            switch (expr_node->expr_type) { // Now check the specific expression type
                case EXPR_BINARY: {
                    BinaryExpressionNode* bin_node = (BinaryExpressionNode*)node;
                    ast_free_node(bin_node->left);
                    ast_free_node(bin_node->right);
                    // bin_node->operator is just a Token*, freed by base node's token
                    break;
                }
                case EXPR_UNARY: {
                    UnaryExpressionNode* un_node = (UnaryExpressionNode*)node;
                    ast_free_node(un_node->operand);
                    // un_node->operator is just a Token*
                    break;
                }
                case EXPR_ASSIGNMENT: {
                    AssignmentExpressionNode* assign_node = (AssignmentExpressionNode*)node;
                    ast_free_node(assign_node->target);
                    ast_free_node(assign_node->value);
                    // assign_node->operator is just a Token*
                    break;
                }
                case EXPR_CALL: {
                    CallExpressionNode* call_node = (CallExpressionNode*)node;
                    ast_free_node(call_node->callee);
                    for (size_t i = 0; i < call_node->argument_count; ++i) {
                        ast_free_node(call_node->arguments[i]);
                    }
                    free(call_node->arguments);
                    break;
                }
                case EXPR_MEMBER_ACCESS: {
                    MemberAccessExpressionNode* member_node = (MemberAccessExpressionNode*)node;
                    ast_free_node(member_node->object);
                    ast_free_node((ASTNode*)member_node->member); // Cast IdentifierNode to ASTNode
                    break;
                }
                case EXPR_ARRAY_ACCESS: {
                    ArrayAccessExpressionNode* array_node = (ArrayAccessExpressionNode*)node;
                    ast_free_node(array_node->array);
                    ast_free_node(array_node->index);
                    break;
                }
                case EXPR_NEW_OBJECT: {
                    NewObjectExpressionNode* new_obj_node = (NewObjectExpressionNode*)node;
                    ast_free_node((ASTNode*)new_obj_node->class_type);
                    for (size_t i = 0; i < new_obj_node->argument_count; ++i) {
                        ast_free_node(new_obj_node->arguments[i]);
                    }
                    free(new_obj_node->arguments);
                    break;
                }
                case EXPR_NEW_ARRAY: {
                    NewArrayExpressionNode* new_arr_node = (NewArrayExpressionNode*)node;
                    ast_free_node((ASTNode*)new_arr_node->element_type);
                    for (size_t i = 0; i < new_arr_node->dimension_count; ++i) {
                        ast_free_node(new_arr_node->dimensions[i]);
                    }
                    free(new_arr_node->dimensions);
                    break;
                }
                case EXPR_CAST: {
                    CastExpressionNode* cast_node = (CastExpressionNode*)node;
                    ast_free_node((ASTNode*)cast_node->target_type);
                    ast_free_node(cast_node->operand);
                    break;
                }
                case EXPR_TERNARY: {
                    TernaryExpressionNode* ternary_node = (TernaryExpressionNode*)node;
                    ast_free_node(ternary_node->condition);
                    ast_free_node(ternary_node->true_expr);
                    ast_free_node(ternary_node->false_expr);
                    break;
                }
                case EXPR_REF_PARAM: {
                    RefOutExpressionNode* ref_out_node = (RefOutExpressionNode*)node;
                    // ref_out_node->modifier is a Token*
                    ast_free_node(ref_out_node->operand);
                    break;
                }
                case EXPR_LITERAL: // Covered by NODE_LITERAL
                case EXPR_IDENTIFIER: // Covered by NODE_IDENTIFIER
                default:
                    // No children or specific fields to free for base expression, literals, identifiers.
                    // Or for cases not yet implemented/recognized.
                    break;
            }
            break;
        }
        case NODE_STATEMENT: {
            StatementNode* stmt_node = (StatementNode*)node; // Cast to base StatementNode
            switch (stmt_node->stmt_type) {
                case STMT_BLOCK: {
                    BlockStatementNode* block_node = (BlockStatementNode*)node;
                    for (size_t i = 0; i < block_node->statement_count; ++i) {
                        ast_free_node((ASTNode*)block_node->statements[i]);
                    }
                    free(block_node->statements);
                    break;
                }
                case STMT_EXPRESSION: {
                    ExpressionStatementNode* expr_stmt_node = (ExpressionStatementNode*)node;
                    ast_free_node((ASTNode*)expr_stmt_node->expression);
                    break;
                }
                case STMT_VAR_DECL: {
                    VariableDeclarationStatementNode* var_decl_node = (VariableDeclarationStatementNode*)node;
                    // var_decl_node->modifier, immutability_modifier are Tokens*
                    ast_free_node((ASTNode*)var_decl_node->type);
                    ast_free_node((ASTNode*)var_decl_node->name);
                    ast_free_node((ASTNode*)var_decl_node->initializer);
                    break;
                }
                case STMT_IF: {
                    IfStatementNode* if_node = (IfStatementNode*)node;
                    ast_free_node((ASTNode*)if_node->condition);
                    ast_free_node((ASTNode*)if_node->then_branch);
                    ast_free_node((ASTNode*)if_node->else_branch); // Could be NULL
                    break;
                }
                case STMT_WHILE: {
                    WhileStatementNode* while_node = (WhileStatementNode*)node;
                    ast_free_node((ASTNode*)while_node->condition);
                    ast_free_node((ASTNode*)while_node->body);
                    break;
                }
                case STMT_FOR: {
                    ForStatementNode* for_node = (ForStatementNode*)node;
                    ast_free_node((ASTNode*)for_node->initializer);
                    ast_free_node((ASTNode*)for_node->condition);
                    ast_free_node((ASTNode*)for_node->incrementer);
                    ast_free_node((ASTNode*)for_node->body);
                    break;
                }
                case STMT_DO_WHILE: {
                    DoWhileStatementNode* do_while_node = (DoWhileStatementNode*)node;
                    ast_free_node((ASTNode*)do_while_node->body);
                    ast_free_node((ASTNode*)do_while_node->condition);
                    break;
                }
                case STMT_FOREACH: {
                    ForEachStatementNode* foreach_node = (ForEachStatementNode*)node;
                    ast_free_node((ASTNode*)foreach_node->item_type);
                    ast_free_node((ASTNode*)foreach_node->item_name);
                    ast_free_node((ASTNode*)foreach_node->collection);
                    ast_free_node((ASTNode*)foreach_node->body);
                    break;
                }
                case STMT_BREAK:
                case STMT_CONTINUE: {
                    JumpStatementNode* jump_node = (JumpStatementNode*)node;
                    ast_free_node((ASTNode*)jump_node->label); // Label is optional
                    break;
                }
                case STMT_RETURN: {
                    ReturnStatementNode* return_node = (ReturnStatementNode*)node;
                    ast_free_node((ASTNode*)return_node->value); // Value is optional
                    break;
                }
                case STMT_THROW: {
                    ThrowStatementNode* throw_node = (ThrowStatementNode*)node;
                    ast_free_node((ASTNode*)throw_node->exception);
                    break;
                }
                case STMT_TRY_CATCH_FINALLY: {
                    TryCatchFinallyStatementNode* try_node = (TryCatchFinallyStatementNode*)node;
                    ast_free_node((ASTNode*)try_node->try_block);
                    for (size_t i = 0; i < try_node->catch_clause_count; ++i) {
                        ast_free_node((ASTNode*)try_node->catch_clauses[i]->exception_type);
                        ast_free_node((ASTNode*)try_node->catch_clauses[i]->variable_name);
                        ast_free_node((ASTNode*)try_node->catch_clauses[i]->catch_block);
                        free(try_node->catch_clauses[i]); // Free the CatchClauseNode struct itself
                    }
                    free(try_node->catch_clauses);
                    ast_free_node((ASTNode*)try_node->finally_block);
                    break;
                }
                default:
                    break; // No children or specific fields for other statement types
            }
            break;
        }
        case NODE_DECLARATION: {
            DeclarationNode* decl_node = (DeclarationNode*)node; // Cast to base DeclarationNode
            switch (decl_node->decl_type) {
                case DECL_VARIABLE: { // For FieldDeclarationNode
                    FieldDeclarationNode* field_node = (FieldDeclarationNode*)node;
                    // modifiers are Tokens*
                    ast_free_node((ASTNode*)field_node->type);
                    ast_free_node((ASTNode*)field_node->name);
                    ast_free_node((ASTNode*)field_node->initializer);
                    break;
                }
                case DECL_FUNCTION: { // For FunctionDeclarationNode or ConstructorDeclarationNode
                    if (node->token->type == VOID || (node->token->type == IDENTIFIER && strcmp(node->token->lexeme, ((FunctionDeclarationNode*)node)->name->name) != 0)) { // Simple heuristic for function vs constructor
                        FunctionDeclarationNode* func_node = (FunctionDeclarationNode*)node;
                        // modifiers are Tokens*
                        ast_free_node((ASTNode*)func_node->return_type);
                        ast_free_node((ASTNode*)func_node->name);
                        for (size_t i = 0; i < func_node->parameter_count; ++i) {
                            // func_node->parameters[i]->modifier is a Token*
                            ast_free_node((ASTNode*)func_node->parameters[i]->type);
                            ast_free_node((ASTNode*)func_node->parameters[i]->name);
                            ast_free_node((ASTNode*)func_node->parameters[i]->default_value);
                            free(func_node->parameters[i]); // Free ParameterNode struct
                        }
                        free(func_node->parameters);
                        for (size_t i = 0; i < func_node->thrown_exception_count; ++i) {
                            ast_free_node((ASTNode*)func_node->thrown_exceptions[i]);
                        }
                        free(func_node->thrown_exceptions);
                        ast_free_node((ASTNode*)func_node->body);
                    } else { // It's a constructor
                        ConstructorDeclarationNode* ctor_node = (ConstructorDeclarationNode*)node;
                        ast_free_node((ASTNode*)ctor_node->name);
                         for (size_t i = 0; i < ctor_node->parameter_count; ++i) {
                            ast_free_node((ASTNode*)ctor_node->parameters[i]->type);
                            ast_free_node((ASTNode*)ctor_node->parameters[i]->name);
                            ast_free_node((ASTNode*)ctor_node->parameters[i]->default_value);
                            free(ctor_node->parameters[i]); // Free ParameterNode struct
                        }
                        free(ctor_node->parameters);
                        ast_free_node((ASTNode*)ctor_node->body);
                    }
                    break;
                }
                case DECL_CLASS:
                case DECL_INTERFACE: {
                    ClassInterfaceDeclarationNode* class_int_node = (ClassInterfaceDeclarationNode*)node;
                    // access_modifier is a Token*
                    ast_free_node((ASTNode*)class_int_node->name);
                    for (size_t i = 0; i < class_int_node->super_class_count; ++i) {
                        ast_free_node((ASTNode*)class_int_node->super_classes[i]);
                    }
                    free(class_int_node->super_classes);
                    for (size_t i = 0; i < class_int_node->implemented_interface_count; ++i) {
                        ast_free_node((ASTNode*)class_int_node->implemented_interfaces[i]);
                    }
                    free(class_int_node->implemented_interfaces);
                    for (size_t i = 0; i < class_int_node->member_count; ++i) {
                        ast_free_node(class_int_node->members[i]);
                    }
                    free(class_int_node->members);
                    break;
                }
                case DECL_ENUM: {
                    EnumDeclarationNode* enum_node = (EnumDeclarationNode*)node;
                    ast_free_node((ASTNode*)enum_node->name);
                    for (size_t i = 0; i < enum_node->enum_value_count; ++i) {
                        ast_free_node((ASTNode*)enum_node->enum_values[i]);
                    }
                    free(enum_node->enum_values);
                    break;
                }
                case DECL_PACKAGE: {
                    PackageDeclarationNode* pkg_node = (PackageDeclarationNode*)node;
                    ast_free_node((ASTNode*)pkg_node->package_name);
                    break;
                }
                case DECL_IMPORT: {
                    ImportDeclarationNode* import_node = (ImportDeclarationNode*)node;
                    ast_free_node((ASTNode*)import_node->imported_name);
                    break;
                }
                default:
                    break;
            }
            break;
        }
        case NODE_PROGRAM: {
            ProgramNode* program_node = (ProgramNode*)node;
            for (size_t i = 0; i < program_node->declaration_count; ++i) {
                ast_free_node(program_node->declarations[i]);
            }
            free(program_node->declarations);
            break;
        }
        default:
            // This should ideally not be reached if all node types are handled.
            break;
    }
    free(node); // Finally, free the node itself
}

/**
 * Frees the entire AST starting from the ProgramNode.
 */
void ast_free_program(ProgramNode* program_node) {
    ast_free_node((ASTNode*)program_node);
}

