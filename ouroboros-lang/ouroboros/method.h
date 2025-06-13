#ifndef METHOD_H
#define METHOD_H

#include "ast_types.h"
#include "stack.h"

// Method structure
typedef struct {
    char class_name[128];
    char method_name[128];
    ASTNode *body;
} Method;

void register_method(const char *class_name, const char *method_name, ASTNode *body);
ASTNode* find_method(const char *class_name, const char *method_name);
void call_method(const char *class_name, const char *method_name, StackFrame *frame);

#endif // METHOD_H
