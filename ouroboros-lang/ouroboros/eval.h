#ifndef EVAL_H
#define EVAL_H

#include "stack.h"    // For StackFrame
#include "ast_types.h" // For ASTNode

// Main evaluation function for an expression AST node
// Returns a C-string representing the evaluated value.
// For complex types (objects, arrays), this might be a reference string (e.g., "obj:123").
// The caller should be aware that some results might point to a static internal buffer
// in eval.c and should be used or copied immediately.
const char* evaluate_expression(ASTNode *expr_node, StackFrame *frame);

// Helper to check if a string is numeric (used internally by eval.c, but could be util)
int is_numeric_string(const char *s);

// Note: call_user_function was removed as execute_function_call in vm.c serves this purpose.

#endif // EVAL_H
