#ifndef OUROBOROS_CODEGEN_H
#define OUROBOROS_CODEGEN_H

#include <stdio.h>
#include <stdbool.h>

// Forward declarations
typedef struct ASTNode ASTNode;
typedef struct ProgramNode ProgramNode;
typedef struct ExpressionNode ExpressionNode;

// CodeGenerator structure
typedef struct {
    FILE* output_file;
    // Add other fields as needed
} CodeGenerator;

// Public API
CodeGenerator* codegen_init(FILE* output_file);
bool codegen_generate(CodeGenerator* codegen, ASTNode* ast_root);
void codegen_free(CodeGenerator* codegen);

#endif // OUROBOROS_CODEGEN_H 