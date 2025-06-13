#ifndef OPTIMIZE_H
#define OPTIMIZE_H

#include "ast_types.h" // Changed from parser.h to ast_types.h for ASTNode definition

void optimize_ast(ASTNode *root);
void constant_fold(ASTNode *node); // Expose for potential direct use or testing

#endif // OPTIMIZE_H
