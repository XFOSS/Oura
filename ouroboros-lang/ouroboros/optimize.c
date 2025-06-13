#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "optimize.h"
// #include "parser.h" // No longer needed if ast_types.h is included by optimize.h
#include "ast_types.h" // For node_type_to_string and ASTNode structure

void constant_fold(ASTNode *node) {
    if (!node) return;

    // Fold children first, unless this node itself will become a literal
    // (in which case its children will be detached and freed).
    if (node->type != AST_BINARY_OP) { 
      if (node->left) constant_fold(node->left);
      if (node->right) constant_fold(node->right);
    } else { 
        if (node->left && node->left->type != AST_LITERAL) constant_fold(node->left);
        if (node->right && node->right->type != AST_LITERAL) constant_fold(node->right);
    }


    if (node->type == AST_BINARY_OP) {
        if (node->left && node->right &&
            node->left->type == AST_LITERAL &&
            node->right->type == AST_LITERAL) {

            int l_val = atoi(node->left->value);
            int r_val = atoi(node->right->value);
            int result = 0;
            int folded = 1; 

            if (strcmp(node->value, "+") == 0) result = l_val + r_val;
            else if (strcmp(node->value, "-") == 0) result = l_val - r_val;
            else if (strcmp(node->value, "*") == 0) result = l_val * r_val;
            else if (strcmp(node->value, "/") == 0) {
                if (r_val != 0) result = l_val / r_val;
                else {
                    fprintf(stderr, "[OPT L%d:%d] Error: Division by zero during constant folding: %s / %s \n", node->line, node->col, node->left->value, node->right->value);
                    folded = 0; 
                }
            } else {
                folded = 0; 
            }

            if (folded) {
                free_ast(node->left); 
                free_ast(node->right);

                snprintf(node->value, sizeof(node->value), "%d", result); 
                node->type = AST_LITERAL;
                node->left = NULL; 
                node->right = NULL;
                
                strncpy(node->data_type, "int", sizeof(node->data_type) - 1);
                node->data_type[sizeof(node->data_type) - 1] = '\0';

                printf("[OPT] Folded constant: %d at L%d:%d (New type: %s)\n", result, node->line, node->col, node->data_type);
            }
        }
    }
}

void optimize_ast(ASTNode *root) {
    if (!root) return;
    
    if (root->left) {
        optimize_ast(root->left);
    }
    if (root->right) {
        optimize_ast(root->right);
    }
    
    constant_fold(root); 

    if (root->next) {
        optimize_ast(root->next);
    }
}
