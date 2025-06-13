#include <stdio.h>
#include "ir.h"

void generate_ir(ASTNode *root) {
    if (!root) return;
    printf("[IR] Generating IR for node: %s\n", root->value);
    generate_ir(root->left);
    generate_ir(root->right);
    generate_ir(root->next);
}
