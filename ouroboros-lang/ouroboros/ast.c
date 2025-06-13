#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast_types.h"

// Create a new AST node
ASTNode* create_node(ASTNodeType type, const char* value, int line, int col) { // Added line, col
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    if (!node) {
        fprintf(stderr, "Error: Failed to allocate memory for AST node\n");
        return NULL;
    }
    
    node->type = type;
    if (value) {
        strncpy(node->value, value, sizeof(node->value) - 1);
        node->value[sizeof(node->value) - 1] = '\0';
    } else {
        node->value[0] = '\0';
    }
    
    node->left = NULL;
    node->right = NULL;
    node->next = NULL;
    
    node->line = line; // Initialize line
    node->col = col;   // Initialize col
    
    // Initialize other fields
    node->data_type[0] = '\0';
    node->generic_type[0] = '\0';
    node->is_void = 0;
    node->is_array = 0;
    node->array_size = 0;
    node->access_modifier[0] = '\0';
    node->parent_class_name = NULL; // Changed from parent_class
    
    return node;
}

// Function to print indentation
static void print_indent(int indent) {
    for (int i = 0; i < indent; i++) {
        printf("  ");
    }
}

// Convert node type to string for debug printing
const char* node_type_to_string(ASTNodeType type) {
    switch (type) {
        case AST_PROGRAM: return "Program";
        case AST_FUNCTION: return "Function";
        case AST_CLASS: return "Class";
        case AST_VAR_DECL: return "VarDecl";
        case AST_ASSIGN: return "Assign";
        case AST_RETURN: return "Return";
        case AST_IF: return "If";
        case AST_ELSE: return "Else";
        case AST_WHILE: return "While";
        case AST_FOR: return "For";
        case AST_BLOCK: return "Block";
        case AST_CALL: return "Call";
        case AST_BINARY_OP: return "BinaryOp";
        case AST_UNARY_OP: return "UnaryOp";
        case AST_LITERAL: return "Literal";
        case AST_IDENTIFIER: return "Identifier";
        case AST_ARRAY: return "Array";
        case AST_IMPORT: return "Import";
        case AST_STRUCT: return "Struct";
        case AST_STRUCT_INIT: return "StructInit";
        case AST_CLASS_METHOD: return "ClassMethod";
        case AST_NEW: return "New";
        case AST_MEMBER_ACCESS: return "MemberAccess";
        case AST_THIS: return "This";
        case AST_GENERIC: return "Generic";
        case AST_TYPED_VAR_DECL: return "TypedVarDecl";
        case AST_TYPED_FUNCTION: return "TypedFunction";
        case AST_TYPE: return "Type";
        case AST_PARAMETER: return "Parameter";
        case AST_STRUCT_FIELD: return "StructField";
        case AST_CLASS_FIELD: return "ClassField";
        case AST_PRINT: return "Print";
        case AST_INDEX_ACCESS: return "IndexAccess";
        case AST_UNKNOWN: return "Unknown";
        default: return "Unknown";
    }
}

// Print the AST (for debugging)
void print_ast(ASTNode* node, int indent) {
    if (!node) return;
    
    print_indent(indent);
    printf("%s: %s (L%d:C%d)", node_type_to_string(node->type), node->value, node->line, node->col); // Added line/col
    
    // Print type information if available
    if (node->data_type[0] != '\0') {
        printf(" (Type: %s", node->data_type);
        
        if (node->generic_type[0] != '\0') {
            printf("<%s>", node->generic_type);
        }
        
        if (node->is_void) {
            printf(", void");
        }
        
        if (node->is_array) {
            printf(", array");
        }
        
        printf(")");
    } else if (node->generic_type[0] != '\0') {
        printf(" (Generic: %s)", node->generic_type);
    } else if (node->is_void) {
        printf(" (void)");
    }
    
    // Print access modifier if present
    if (node->access_modifier[0] != '\0') {
        printf(" [%s]", node->access_modifier);
    }
    
    // Print parent class if present (for methods)
    if (node->parent_class_name) {
        printf(" [ParentClass: %s]", node->parent_class_name);
    }
    
    printf("\n");
    
    // Recursively print children
    if (node->left) {
        print_indent(indent);
        printf("Left:\n");
        print_ast(node->left, indent + 1);
    }
    
    if (node->right) {
        print_indent(indent);
        printf("Right:\n");
        print_ast(node->right, indent + 1);
    }
    
    if (node->next) {
        print_indent(indent);
        printf("Next:\n");
        print_ast(node->next, indent + 1);
    }
}

// Free the AST
void free_ast(ASTNode* node) {
    if (!node) return;
    
    // Free children first
    free_ast(node->left);
    free_ast(node->right);
    free_ast(node->next);
    
    // Free parent class name if allocated
    if (node->parent_class_name) { // Changed from parent_class
        free(node->parent_class_name);
        node->parent_class_name = NULL;
    }
    
    // Free this node
    free(node);
}
