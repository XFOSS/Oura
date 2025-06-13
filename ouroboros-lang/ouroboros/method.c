#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "method.h"
#include "vm.h"

// Simple array-based method storage
#define MAX_METHODS 100
static Method methods[MAX_METHODS];
static int method_count = 0;

void register_method(const char *class_name, const char *method_name, ASTNode *body) {
    if (method_count >= MAX_METHODS) {
        printf("[METHOD] Error: Maximum number of methods reached\n");
        return;
    }
    
    strncpy(methods[method_count].class_name, class_name, sizeof(methods[method_count].class_name) - 1);
    methods[method_count].class_name[sizeof(methods[method_count].class_name) - 1] = '\0';
    
    strncpy(methods[method_count].method_name, method_name, sizeof(methods[method_count].method_name) - 1);
    methods[method_count].method_name[sizeof(methods[method_count].method_name) - 1] = '\0';
    
    methods[method_count].body = body;
    
    printf("[METHOD] Registered %s::%s\n", class_name, method_name);
    method_count++;
}

ASTNode* find_method(const char *class_name, const char *method_name) {
    for (int i = 0; i < method_count; i++) {
        if (strcmp(methods[i].class_name, class_name) == 0 && 
            strcmp(methods[i].method_name, method_name) == 0) {
            return methods[i].body;
        }
    }
    return NULL;
}

void call_method(const char *class_name, const char *method_name, StackFrame *frame) {
    (void)frame; // Mark parameter as unused
    ASTNode *body = find_method(class_name, method_name);
    if (!body) {
        printf("[METHOD] Not found: %s::%s\n", class_name, method_name);
        return;
    }
    printf("[METHOD] Calling %s::%s\n", class_name, method_name);
    // Frame is unused in this implementation
    run_vm(body);
}
