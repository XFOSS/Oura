#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h> // For isupper in identifier evaluation
#include "vm.h"
#include "ast_types.h"
#include "stack.h"
#include "eval.h"    // For evaluate_expression
#include "stdlib.h"  // For actual call_builtin_function, register_stdlib_functions
#include "module.h"  // For Module types, if used for imports

// Using AccessModifierEnum from vm.h; remove string macro definition

extern ASTNode *program; // Global AST root from parser

typedef struct FunctionEntry {
    ASTNode *func;
    struct FunctionEntry *next;
} FunctionEntry;

typedef struct ClassEntry {
    char name[128];
    char parent_name[128];
    ASTNode *class_node; 
    struct ClassEntry *next;
} ClassEntry;

static StackFrame *global_frame = NULL;
static char *return_value = NULL;
static FunctionEntry *registered_functions = NULL;
static ClassEntry *registered_classes = NULL;
static ClassEntry *registered_classes_tail = NULL;

char current_class[128] = {0}; // Global current class context for resolution
char g_super_target_class[128] = {0};
Object *objects = NULL;
static int next_object_id = 1;

static int g_break_flag = 0;
static int g_continue_flag = 0;

static int is_class_registered(const char *name);
static ClassEntry* find_class_entry(const char *name);
static void initialize_default_instance_fields(const char *class_name, Object *instance, StackFrame* frame_for_eval);
const char* get_parent_class_name(const char *class_name);
ASTNode* find_class_method(const char *class_name, const char *method_name);

const char* get_return_value() {
    return return_value ? return_value : "0"; 
}

void set_return_value(const char* value) {
    if (return_value) {
        free(return_value);
        return_value = NULL;
    }
    if (value) {
        return_value = strdup(value);
        if (!return_value) {
            fprintf(stderr, "Error: Memory allocation failed for return value. Defaulting to \"0\".\n");
            return_value = strdup("0"); 
        }
    } else {
        return_value = strdup("0"); 
    }
}

Object* create_object(const char *class_name) {
    Object *obj = (Object*)calloc(1, sizeof(Object)); // Use calloc
    if (!obj) {
        fprintf(stderr, "Error: Failed to allocate memory for object of class '%s'\n", class_name);
        return NULL;
    }
    snprintf(obj->class_name, sizeof(obj->class_name), "%s#%d", class_name, next_object_id++);
    obj->next = objects;
    objects = obj;
    // printf("[OBJECT] Created new object: %s (class: %s)\n", obj->class_name, class_name);
    initialize_default_instance_fields(class_name, obj, global_frame); 
    return obj;
}

void set_object_property_with_access(Object *obj, const char *name, const char *value, AccessModifierEnum access, int is_static) {
    if (!obj) { fprintf(stderr, "Error: Cannot set property '%s' on null object\n", name); return; }
    if (!name || !value) { fprintf(stderr, "Error: Invalid parameters for setting object property (name or value is null)\n"); return; }
    
    ObjectProperty *prop = obj->properties;
    while (prop) {
        if (strcmp(prop->name, name) == 0) {
            strncpy(prop->value, value, sizeof(prop->value) - 1);
            prop->value[sizeof(prop->value) - 1] = '\0';
            prop->access = access;
            prop->is_static = is_static;
            return;
        }
        prop = prop->next;
    }
    
    ObjectProperty *new_prop = (ObjectProperty*)calloc(1, sizeof(ObjectProperty)); // Use calloc
    if (!new_prop) { fprintf(stderr, "Error: Failed to allocate memory for object property '%s'\n", name); return; }
    
    strncpy(new_prop->name, name, sizeof(new_prop->name) - 1);
    new_prop->name[sizeof(new_prop->name) - 1] = '\0';
    strncpy(new_prop->value, value, sizeof(new_prop->value) - 1);
    new_prop->value[sizeof(new_prop->value) - 1] = '\0';
    new_prop->access = access;
    new_prop->is_static = is_static;
    new_prop->next = obj->properties;
    obj->properties = new_prop;
}

const char* get_object_property(Object *obj, const char *name) {
    return get_object_property_with_access_check(obj, name, NULL); 
}

const char* get_object_property_with_access_check(Object *obj, const char *name, const char *accessing_class_context) {
    if (!obj || !name) return NULL;
    
    char obj_base_class_name[128] = {0};
    char *hash_pos = strchr(obj->class_name, '#');
    if (hash_pos) {
        size_t len = hash_pos - obj->class_name;
        if (len < sizeof(obj_base_class_name)) {
            strncpy(obj_base_class_name, obj->class_name, len);
            obj_base_class_name[len] = '\0';
        } else {
            strncpy(obj_base_class_name, obj->class_name, sizeof(obj_base_class_name) -1);
            obj_base_class_name[sizeof(obj_base_class_name)-1] = '\0';
        }
    } else { 
        strncpy(obj_base_class_name, obj->class_name, sizeof(obj_base_class_name) - 1);
        obj_base_class_name[sizeof(obj_base_class_name)-1] = '\0';
        if(strstr(obj_base_class_name, "_static")) { // "ClassName_static"
             char * p = strstr(obj_base_class_name, "_static");
             if(p) *p = '\0';
        }
    }
    
    ObjectProperty *prop = obj->properties;
    while (prop) {
        if (strcmp(prop->name, name) == 0) {
            if (prop->access == ACCESS_MODIFIER_PUBLIC) return prop->value;
            if (prop->access == ACCESS_MODIFIER_PRIVATE) {
                if (accessing_class_context && strcmp(accessing_class_context, obj_base_class_name) == 0) {
                    return prop->value;
                } else {
                    // fprintf(stderr, "Access Denied: Cannot access private property '%s.%s' from context '%s'.\n", 
                    //         obj_base_class_name, name, accessing_class_context ? accessing_class_context : "global/unknown");
                    return NULL; 
                }
            }
            return prop->value; 
        }
        prop = prop->next;
    }
    return NULL; 
}

const char* get_object_property_with_access(Object *obj, const char *property_name, const char *current_class_context_for_access_check) {
    if (!obj) return "undefined";
    const char* instance_prop_val = get_object_property_with_access_check(obj, property_name, current_class_context_for_access_check);
    if (instance_prop_val) return instance_prop_val;

    char obj_base_class_name[128] = {0};
    char *hash_pos = strchr(obj->class_name, '#');
    if (hash_pos) {
        size_t len = hash_pos - obj->class_name;
         if (len < sizeof(obj_base_class_name)) {
            strncpy(obj_base_class_name, obj->class_name, len);
            obj_base_class_name[len] = '\0';
        } else {
            strncpy(obj_base_class_name, obj->class_name, sizeof(obj_base_class_name)-1);
            obj_base_class_name[sizeof(obj_base_class_name)-1] = '\0';
        }
        if(strstr(obj_base_class_name, "_static")) { // if "ClassName_static"
             char * p = strstr(obj_base_class_name, "_static");
             if(p) *p = '\0'; // Get "ClassName"
        }


        Object *static_class_obj = find_static_class_object(obj_base_class_name);
        if (static_class_obj) {
            ObjectProperty *static_prop = static_class_obj->properties;
            while(static_prop) {
                if (strcmp(static_prop->name, property_name) == 0 && static_prop->is_static) {
                    if (static_prop->access == ACCESS_MODIFIER_PUBLIC) return static_prop->value;
                    if (static_prop->access == ACCESS_MODIFIER_PRIVATE && 
                        current_class_context_for_access_check && 
                        strcmp(current_class_context_for_access_check, obj_base_class_name) == 0) {
                        return static_prop->value;
                    }
                    // fprintf(stderr, "Access Denied: Cannot access private static property '%s.%s' from context '%s'.\n",
                    //          obj_base_class_name, property_name, current_class_context_for_access_check ? current_class_context_for_access_check : "global/unknown");
                    return "undefined"; 
                }
                static_prop = static_prop->next;
            }
        }
    }
    return "undefined";
}

const char* get_static_property(const char *class_name, const char *prop_name) {
    if (!class_name || !prop_name) return NULL;
    Object *static_obj = find_static_class_object(class_name);
    if (static_obj) {
        return get_object_property_with_access_check(static_obj, prop_name, class_name); 
    }
    return NULL;
}

static void vm_register_class(ASTNode *class_node) { 
    if (!class_node || (class_node->type != AST_CLASS && class_node->type != AST_STRUCT) || !class_node->value[0]) return;
    const char *name = class_node->value;
    if (is_class_registered(name)) {
        // printf("[VM] Warning (L%d:%d): Class '%s' already registered. Skipping.\n", class_node->line, class_node->col, name);
        return;
    }
    ClassEntry *entry = (ClassEntry*)calloc(1, sizeof(ClassEntry)); // Use calloc
    if (!entry) {
        fprintf(stderr, "Error (L%d:%d): Failed to allocate memory for class entry '%s'\n", class_node->line, class_node->col, name);
        return;
    }
    strncpy(entry->name, name, sizeof(entry->name) - 1);
    entry->name[sizeof(entry->name) - 1] = '\0';

    if (class_node->right && class_node->right->type == AST_IDENTIFIER) {
        strncpy(entry->parent_name, class_node->right->value, sizeof(entry->parent_name)-1);
        entry->parent_name[sizeof(entry->parent_name)-1] = '\0';
    } else { entry->parent_name[0] = '\0'; }

    entry->class_node = class_node; 
    if (!registered_classes) registered_classes = registered_classes_tail = entry;
    else { registered_classes_tail->next = entry; registered_classes_tail = entry; }
    // printf("[VM] Registered class: %s (from L%d:%d)\n", name, class_node->line, class_node->col);
}

static ClassEntry* find_class_entry(const char *name) {
    if (!name) return NULL;
    ClassEntry *entry = registered_classes;
    while (entry) {
        if (strcmp(entry->name, name) == 0) return entry;
        entry = entry->next;
    }
    return NULL;
}

static int is_class_registered(const char *name) {
    return find_class_entry(name) != NULL;
}

void free_object(Object *obj) {
    if (!obj) return;
    ObjectProperty *prop = obj->properties;
    while (prop) {
        ObjectProperty *next_prop = prop->next;
        free(prop);
        prop = next_prop;
    }
    free(obj);
}

void vm_init() {
    if (global_frame) destroy_stack_frame(global_frame);
    global_frame = create_stack_frame("global", NULL);
    
    if (return_value) free(return_value);
    return_value = strdup("0"); 
    
    Object *obj = objects;
    while (obj) { Object *next = obj->next; free_object(obj); obj = next; }
    objects = NULL;
    next_object_id = 1;

    FunctionEntry *fn_entry = registered_functions;
    while(fn_entry) { FunctionEntry* next = fn_entry->next; free(fn_entry); fn_entry = next; }
    registered_functions = NULL;

    ClassEntry *cls_entry = registered_classes;
    while(cls_entry) { ClassEntry* next = cls_entry->next; free(cls_entry); cls_entry = next; }
    registered_classes = NULL;
    registered_classes_tail = NULL;

    current_class[0] = '\0';
}

void vm_cleanup() {
    if (return_value) { free(return_value); return_value = NULL; }
    if (global_frame) { destroy_stack_frame(global_frame); global_frame = NULL; }
    
    FunctionEntry *entry = registered_functions;
    while (entry) { FunctionEntry *next = entry->next; free(entry); entry = next; }
    registered_functions = NULL;
    
    ClassEntry *class_entry = registered_classes;
    while (class_entry) { ClassEntry *next_entry = class_entry->next; free(class_entry); class_entry = next_entry; }
    registered_classes = NULL;
    registered_classes_tail = NULL;
    
    Object *obj = objects;
    while (obj) { Object *next_obj = obj->next; free_object(obj); obj = next_obj; }
    objects = NULL;
    // printf("[VM] Cleanup complete.\n");
}

void register_user_function(ASTNode *func_node) {
    if (!func_node || (func_node->type != AST_FUNCTION && func_node->type != AST_TYPED_FUNCTION)) {
        if(func_node) fprintf(stderr, "Error (L%d:%d): Attempted to register non-function node '%s' as function.\n", func_node->line, func_node->col, func_node->value);
        return;
    }
    FunctionEntry *entry = (FunctionEntry *)calloc(1, sizeof(FunctionEntry)); // Use calloc
    if (!entry) {
        fprintf(stderr, "Error (L%d:%d): Failed to allocate memory for function entry '%s'\n", func_node->line, func_node->col, func_node->value);
        return;
    }
    entry->func = func_node;
    entry->next = registered_functions;
    registered_functions = entry;
}

ASTNode* find_user_function(const char *name, const char* class_context_name) {
    FunctionEntry *entry = registered_functions;
    while (entry) {
        if (entry->func && entry->func->value && strcmp(entry->func->value, name) == 0) {
            if (class_context_name) { 
                if (entry->func->parent_class_name && strcmp(entry->func->parent_class_name, class_context_name) == 0) {
                    return entry->func;
                }
            } else { 
                if (entry->func->parent_class_name == NULL) {
                    return entry->func;
                }
            }
        }
        entry = entry->next;
    }
    /* Inheritance lookup */
    if (class_context_name) {
        const char *parent = get_parent_class_name(class_context_name);
        while (parent) {
            entry = registered_functions;
            while (entry) {
                if (entry->func && strcmp(entry->func->value, name) == 0) {
                    if (entry->func->parent_class_name && strcmp(entry->func->parent_class_name, parent) == 0) {
                        return entry->func;
                    }
                }
                entry = entry->next;
            }
            parent = get_parent_class_name(parent);
        }
    }
    return NULL;
}

const char* execute_function_call(const char* qualified_name, ASTNode* args_ast_list, StackFrame *caller_frame) {
    // Parse qualified name: obj:123.method or class.method
    char obj_name[128] = "";
    char method_name[128] = "";
    int is_class_method = 0;
    
    // Check if it's a class method call (e.g., "class.method")
    char* dot_pos = strchr(qualified_name, '.');
    if (dot_pos) {
        strncpy(obj_name, qualified_name, dot_pos - qualified_name);
        obj_name[dot_pos - qualified_name] = '\0';
        strcpy(method_name, dot_pos + 1);
        
        // Check if it's a class method (starts with class name)
        ClassEntry* class_entry = find_class_entry(obj_name);
        if (class_entry) {
            is_class_method = 1;
        }
    }
    
    // Find the function/method
    ASTNode* func_node = NULL;
    if (is_class_method) {
        func_node = find_class_method(obj_name, method_name);
    } else {
        func_node = find_user_function(qualified_name, NULL);
        // Check for object property holding a function
        if (!func_node && strncmp(obj_name, "obj:", 4) == 0) {
            Object* inst = find_object_by_id(atoi(obj_name + 4));
            if (inst) {
                const char* fn = get_object_property(inst, method_name);
                if (fn && strcmp(fn, "undefined") != 0) {
                    func_node = find_user_function(fn, NULL);
                }
            }
        }
    }
    
    if (!func_node) {
        // Attempt to call built-in function
        const char* builtin_res = call_built_in_function(qualified_name, args_ast_list, caller_frame);
        if (builtin_res && strcmp(builtin_res, "undefined") != 0) {
            return builtin_res;
        }
        fprintf(stderr, "Error: Function '%s' not found\n", qualified_name);
        return "undefined";
    }
    
    // Create new stack frame for function execution
    StackFrame* new_frame = create_stack_frame(qualified_name, caller_frame);
    
    // Optionally update global current class context for methods
    char prev_class[128]; strncpy(prev_class, current_class, sizeof(prev_class)-1);
    if (is_class_method) {
        strncpy(current_class, obj_name, sizeof(current_class) - 1);
    }

    // Evaluate and set parameters as local variables
    ASTNode* param = func_node->left;
    ASTNode* arg = args_ast_list;
    
    while (param && arg) {
        const char* arg_value = evaluate_expression(arg, caller_frame);
        if (arg_value) {
            set_variable(new_frame, param->value, arg_value);
            // Do not free arg_value, as it may not be heap allocated
        }
        param = param->next;
        arg = arg->next;
    }
    
    // If it's a class method, set 'this' parameter to instance or static class object
    if (func_node->type == AST_CLASS_METHOD) {
        Object* instance = is_class_method ? find_static_class_object(obj_name)
                                            : find_object_by_id(atoi(obj_name));
        if (instance) {
            set_variable(new_frame, "this", instance->class_name);
        }
    }
    
    // Evaluate function body
    run_vm_node(func_node->right, new_frame);
    // Restore previous context
    if (is_class_method) {
        strncpy(current_class, prev_class, sizeof(current_class)-1);
    }
    // Destroy frame
    destroy_stack_frame(new_frame);
    return get_return_value();
}

ASTNode* find_class_method(const char *class_name, const char *method_name) {
    ClassEntry* entry = find_class_entry(class_name);
    if (!entry) return NULL;
    
    ASTNode* class_node = entry->class_node;
    if (!class_node || !class_node->left) return NULL;
    
    ASTNode* member = class_node->left;
    while (member) {
        // Treat function declarations inside classes as class methods
        if ((member->type == AST_CLASS_METHOD || member->type == AST_FUNCTION || member->type == AST_TYPED_FUNCTION)
            && strcmp(member->value, method_name) == 0) {
            return member;
        }
        member = member->next;
    }
    
    // Check parent class
    const char* parent_name = get_parent_class_name(class_name);
    if (parent_name && strlen(parent_name) > 0) {
        return find_class_method(parent_name, method_name);
    }
    
    return NULL;
}

void run_vm_node(ASTNode *node, StackFrame *frame) {
    if (!node) return;
    
    // This current_class update is simplistic. execute_function_call is better placed to manage it.
    // if (frame && frame->function_name) {
    //     const char *dot_pos = strrchr(frame->function_name, '.'); // Use strrchr
    //     if (dot_pos) {
    //         if (strncmp(frame->function_name, "obj:", 4) != 0) { 
    //             size_t class_name_len = dot_pos - frame->function_name;
    //             if (class_name_len < sizeof(current_class)) {
    //                 strncpy(current_class, frame->function_name, class_name_len);
    //                 current_class[class_name_len] = '\0';
    //             }
    //         }
    //     } else {
    //          if(strcmp(frame->name, "global") == 0) current_class[0] = '\0';
    //     }
    // }

    switch (node->type) {
        case AST_PROGRAM: {
            ASTNode *stmt = node->left;
            while (stmt) { run_vm_node(stmt, frame); stmt = stmt->next; }
            break;
        }
        case AST_FUNCTION: case AST_TYPED_FUNCTION: break; 
        case AST_BLOCK: {
            ASTNode *stmt = node->left;
            while (stmt) { run_vm_node(stmt, frame); stmt = stmt->next; }
            break;
        }
        case AST_PRINT: {
            const char *value_to_print = evaluate_expression(node->left, frame);
            printf("[OUTPUT] %s\n", value_to_print ? value_to_print : "undefined");
            fflush(stdout);
            break;
        }
        case AST_VAR_DECL: 
        case AST_TYPED_VAR_DECL: { 
            const char *var_name = node->value; 
            const char *initial_value_str = "undefined"; 
            if (node->right) { 
                initial_value_str = evaluate_expression(node->right, frame);
            } else if (node->type == AST_TYPED_VAR_DECL) { // Default init for typed vars if no explicit init
                if(strcmp(node->data_type, "int")==0 || strcmp(node->data_type, "long")==0) initial_value_str = "0";
                else if(strcmp(node->data_type, "float")==0 || strcmp(node->data_type, "double")==0) initial_value_str = "0.0";
                else if(strcmp(node->data_type, "bool")==0) initial_value_str = "false";
                else if(strcmp(node->data_type, "string")==0) initial_value_str = "";
                // Object types default to null/undefined implicitly
            }
            set_variable(frame, var_name, initial_value_str);
            break;
        }
        case AST_ASSIGN: { 
            const char *value_to_assign = evaluate_expression(node->right, frame);
            if (node->left->type == AST_IDENTIFIER) {
                set_variable(frame, node->left->value, value_to_assign);
            } else if (node->left->type == AST_MEMBER_ACCESS) {
                // This case should ideally be fully handled by AST_BINARY_OP with "="
                // Forcing it here means re-evaluating parts of member access.
                // Simplified: Assume evaluate_expression for AST_BINARY_OP handles it.
                // If AST_ASSIGN is kept distinct, it needs full LHS evaluation logic.
                // For now, rely on the BINARY_OP path.
                fprintf(stderr, "Warning L%d: AST_ASSIGN for member access is less robust, prefer BINARY_OP for assignment.\n", node->line);
                // Quick attempt to make it work similarly to BINARY_OP's assignment logic
                ASTNode temp_binary_op_assign_node; // Stack allocate a temporary node
                temp_binary_op_assign_node.type = AST_BINARY_OP;
                strcpy(temp_binary_op_assign_node.value, "="); // Operator is "="
                temp_binary_op_assign_node.left = node->left;   // Original LHS (e.g. member access node)
                temp_binary_op_assign_node.right = node->right; // Original RHS (expression node for value)
                temp_binary_op_assign_node.line = node->line;
                temp_binary_op_assign_node.col = node->col;
                evaluate_expression(&temp_binary_op_assign_node, frame); // Let eval handle it
            } else {
                 fprintf(stderr, "Error (L%d:%d): Invalid left-hand side for AST_ASSIGN operation.\n", node->left->line, node->left->col);
            }
            break;
        }
        case AST_RETURN: {
            const char *ret_val_str = node->left ? evaluate_expression(node->left, frame) : "0"; 
            set_return_value(ret_val_str);
            break;
        }
        case AST_IF: {
            const char *cond_val_str = evaluate_expression(node->left, frame); 
            int truthy = cond_val_str && strcmp(cond_val_str, "0") != 0 && strcmp(cond_val_str, "false") != 0 && strcmp(cond_val_str, "") != 0;
            if (truthy) run_vm_node(node->right, frame); 
            else if (node->next && node->next->type == AST_ELSE) run_vm_node(node->next->left, frame); 
            break;
        }
        case AST_WHILE: {
            while(1) {
                const char *cond_val_str = evaluate_expression(node->left, frame); 
                int truthy = cond_val_str && strcmp(cond_val_str, "0") != 0 && strcmp(cond_val_str, "false") != 0 && strcmp(cond_val_str, "") != 0;
                if (!truthy) break;
                g_continue_flag = 0; // reset at start of iteration
                run_vm_node(node->right, frame);
                if (g_break_flag) { g_break_flag = 0; break; }
                if (g_continue_flag) { g_continue_flag = 0; continue; }
            }
            break;
        }
        case AST_FOR: {
            ASTNode* init_expr = NULL; ASTNode* cond_expr = NULL; ASTNode* incr_expr = NULL;
            ASTNode* current_control_part = node->left;
            if (current_control_part) { init_expr = current_control_part; current_control_part = current_control_part->next; }
            if (current_control_part) { cond_expr = current_control_part; current_control_part = current_control_part->next; }
            if (current_control_part) { incr_expr = current_control_part; }

            if (init_expr) {
                if(init_expr->type == AST_VAR_DECL || init_expr->type == AST_TYPED_VAR_DECL) run_vm_node(init_expr, frame);
                else evaluate_expression(init_expr, frame); 
            }
            while (1) {
                int truthy = 1; 
                if (cond_expr) {
                    const char *cond_val_str = evaluate_expression(cond_expr, frame);
                    truthy = cond_val_str && strcmp(cond_val_str, "0") != 0 && strcmp(cond_val_str, "false") != 0 && strcmp(cond_val_str, "") != 0;
                }
                if (!truthy) break; 
                run_vm_node(node->right, frame); 
                g_continue_flag = 0; // reset for each iteration
                if (g_break_flag) { g_break_flag = 0; break; }
                if (g_continue_flag) { g_continue_flag = 0; if (incr_expr) evaluate_expression(incr_expr, frame); continue; }
                if (incr_expr) evaluate_expression(incr_expr, frame); 
            }
            break;
        }
        case AST_CALL: {
            // Reconstruct qualified name if method call stored as target->right, value->method_name
            char qualified_name_buffer[512];
            if (node->right) { // Method call (target in right, method name in value)
                const char* target_eval_str = evaluate_expression(node->right, frame);
                snprintf(qualified_name_buffer, sizeof(qualified_name_buffer), "%s.%s", target_eval_str ? target_eval_str : "undefined_target", node->value);
            } else { // Plain function call
                strncpy(qualified_name_buffer, node->value, sizeof(qualified_name_buffer)-1);
                qualified_name_buffer[sizeof(qualified_name_buffer)-1] = '\0';
            }
            execute_function_call(qualified_name_buffer, node->left, frame);
            break;
        }
        case AST_BINARY_OP: case AST_UNARY_OP: case AST_LITERAL: 
        case AST_IDENTIFIER: case AST_MEMBER_ACCESS: case AST_NEW:
            evaluate_expression(node, frame);
            break;
        case AST_BREAK: {
            g_break_flag = 1;
            break;
        }
        case AST_CONTINUE: {
            g_continue_flag = 1;
            break;
        }
        default:
            // fprintf(stderr, "Warning (L%d:%d): VM cannot run unknown AST node type %s (%d).\n", node->line, node->col, node_type_to_string(node->type), node->type);
            break;
    }
}

void run_vm(ASTNode *root_ast_node) {
    if (!root_ast_node) { fprintf(stderr, "[VM] Error: Cannot run VM on NULL AST.\n"); return; }
    vm_init(); 
    
    // printf("\n==== Program Output (VM Run) ====\n");
    
    if (root_ast_node->type == AST_PROGRAM) {
        ASTNode *node = root_ast_node->left;
        while (node) {
            if (node->type == AST_FUNCTION || node->type == AST_TYPED_FUNCTION) {
                register_user_function(node);
            } else if (node->type == AST_CLASS || node->type == AST_STRUCT) {
                vm_register_class(node); 
                ASTNode *class_member = node->left; 
                while (class_member) {
                    if (class_member->type == AST_FUNCTION || class_member->type == AST_TYPED_FUNCTION || class_member->type == AST_CLASS_METHOD) {
                        if (class_member->parent_class_name) free(class_member->parent_class_name);
                        class_member->parent_class_name = strdup(node->value); 
                        register_user_function(class_member);
                    }
                    class_member = class_member->next;
                }
            } else if (node->type == AST_IMPORT) {
                // Handle module import and register its functions and classes
                {
                    Module *mod = module_load(node->value);
                    if (mod && mod->ast && mod->ast->type == AST_PROGRAM) {
                        ASTNode *imp_node = mod->ast->left;
                        while (imp_node) {
                            if (imp_node->type == AST_FUNCTION || imp_node->type == AST_TYPED_FUNCTION) {
                                register_user_function(imp_node);
                            } else if (imp_node->type == AST_CLASS || imp_node->type == AST_STRUCT) {
                                vm_register_class(imp_node);
                                ASTNode *cm = imp_node->left;
                                while (cm) {
                                    if (cm->type == AST_FUNCTION || cm->type == AST_TYPED_FUNCTION || cm->type == AST_CLASS_METHOD) {
                                        if (cm->parent_class_name) free(cm->parent_class_name);
                                        cm->parent_class_name = strdup(imp_node->value);
                                        register_user_function(cm);
                                    }
                                    cm = cm->next;
                                }
                            }
                            imp_node = imp_node->next;
                        }
                    }
                }
            }
            node = node->next;
        }
    }
    
    typedef struct LifecycleInstance {
        char obj_ref_str[32]; 
        const char* class_name;
        struct LifecycleInstance *next;
    } LifecycleInstance;

    LifecycleInstance *lifecycle_instances_list = NULL;
    LifecycleInstance *lifecycle_instances_tail = NULL;

    ClassEntry *cls_iter = registered_classes;
    while (cls_iter) {
        find_static_class_object(cls_iter->name); 
        Object *instance_obj = create_object(cls_iter->name); 
        if (instance_obj) {
            int obj_id_val = 0;
            sscanf(instance_obj->class_name, "%*[^#]#%d", &obj_id_val); 
            LifecycleInstance *inst_entry = (LifecycleInstance*)calloc(1, sizeof(LifecycleInstance)); // Use calloc
            if (!inst_entry) { fprintf(stderr, "Mem alloc failed for lifecycle entry\n"); break;}
            snprintf(inst_entry->obj_ref_str, sizeof(inst_entry->obj_ref_str), "obj:%d", obj_id_val);
            inst_entry->class_name = cls_iter->name; 
            if (!lifecycle_instances_list) lifecycle_instances_list = lifecycle_instances_tail = inst_entry;
            else { lifecycle_instances_tail->next = inst_entry; lifecycle_instances_tail = inst_entry; }
            
            Object* static_obj_for_class = find_static_class_object(cls_iter->name);
            if (static_obj_for_class) {
                int has_static_singleton_field = 0;
                if (cls_iter->class_node && cls_iter->class_node->left) {
                    ASTNode* member = cls_iter->class_node->left;
                    while(member) {
                        if ((member->type == AST_VAR_DECL || member->type == AST_TYPED_VAR_DECL) &&
                            strcmp(member->value, "singleton") == 0 &&
                            member->access_modifier[0] != '\0' && strcmp(member->access_modifier, "static") == 0) {
                            has_static_singleton_field = 1; break;
                        }
                        member = member->next;
                    }
                }
                if (has_static_singleton_field) {
                    set_object_property_with_access(static_obj_for_class, "singleton", inst_entry->obj_ref_str, ACCESS_MODIFIER_PUBLIC, 1);
                }
            }
        }
        cls_iter = cls_iter->next;
    }

    const char *lifecycle_names[] = {"Awake", "Start", "FixedUpdate", "Update", "LateUpdate"};
    (void)lifecycle_names; // suppress unused variable warning
#if 0  // disable lifecycle loops due to call_node undefined
    // Awake & Start
    for(int lc_idx = 0; lifecycle_names[lc_idx]; ++lc_idx) { // Awake, then Start
        const char* lc_name = lifecycle_names[lc_idx];
        if (find_user_function(lc_name, NULL)) execute_function_call(call_node, global_frame);
        for (LifecycleInstance *it = lifecycle_instances_list; it; it = it->next) {
            char qmn[256]; snprintf(qmn, sizeof(qmn), "%s.%s", it->obj_ref_str, lc_name);
            execute_function_call(call_node, global_frame);
        }
    }
    
    int has_any_update = 0;
    for(int lc_idx = 2; lc_idx < 5; ++lc_idx) { // FixedUpdate, Update, LateUpdate
        if(find_user_function(lifecycle_names[lc_idx], NULL)) { has_any_update = 1; break; }
    }
    if (!has_any_update) {
        for (LifecycleInstance *it = lifecycle_instances_list; it; it = it->next) {
             for(int lc_idx = 2; lc_idx < 5; ++lc_idx) {
                if (find_user_function(lifecycle_names[lc_idx], it->class_name)) { has_any_update = 1; break; }
             }
             if(has_any_update) break;
        }
    }

    if (has_any_update) {
        const int FRAME_COUNT = 1; // Reduced for quicker test runs
        for (int frame_idx = 0; frame_idx < FRAME_COUNT; ++frame_idx) {
            for(int lc_idx = 2; lc_idx < 5; ++lc_idx) { // FixedUpdate, Update, LateUpdate
                const char* lc_name = lifecycle_names[lc_idx];
                if (find_user_function(lc_name, NULL)) execute_function_call(call_node, global_frame);
                for (LifecycleInstance *it = lifecycle_instances_list; it; it = it->next) {
                    char qmn[256]; snprintf(qmn, sizeof(qmn), "%s.%s", it->obj_ref_str, lc_name);
                    execute_function_call(call_node, global_frame);
                }
            }
        }
    } else {
        run_vm_node(root_ast_node, global_frame);
    }
#endif  // end disable lifecycle loops

    // Fallback: execute top-level statements for scripts without main
    run_vm_node(root_ast_node, global_frame);

    // Call main() if it exists
    ASTNode* main_func = find_user_function("main", NULL);
    if (main_func) {
        printf("\n\n=========================\n");
        printf("==== EXECUTING MAIN() ====\n");
        printf("=========================\n\n");
        fflush(stdout);
        execute_function_call(main_func->value, main_func->left, global_frame);
        printf("\n\n===========================\n");
        printf("==== EXECUTION COMPLETE ====\n");
        printf("===========================\n\n");
        fflush(stdout);
    }

    while(lifecycle_instances_list) {
        LifecycleInstance* next = lifecycle_instances_list->next;
        free(lifecycle_instances_list);
        lifecycle_instances_list = next;
    }
    // vm_cleanup called by main
}

void set_object_property(Object *obj, const char *name, const char *value) {
    set_object_property_with_access(obj, name, value, ACCESS_MODIFIER_PUBLIC, 0); 
}

const char* evaluate_member_access(ASTNode *member_access_expr_node, StackFrame *frame) {
    if (!member_access_expr_node || member_access_expr_node->type != AST_MEMBER_ACCESS || 
        !member_access_expr_node->left || !member_access_expr_node->value[0]) {
        // fprintf(stderr, "Error (L%d:%d): Invalid member access expression.\n", member_access_expr_node ? member_access_expr_node->line: 0, member_access_expr_node ? member_access_expr_node->col : 0);
        return "undefined";
    }
    
    const char *object_or_class_ref_str;
    ASTNode *target_expr_node = member_access_expr_node->left; 
    const char *property_name_str = member_access_expr_node->value; 

    if (target_expr_node->type == AST_THIS) {
        object_or_class_ref_str = get_variable(frame, "this");
        if (!object_or_class_ref_str) {
            fprintf(stderr, "Error (L%d:%d): 'this' is undefined in current context for member access '%s'.\n", target_expr_node->line, target_expr_node->col, property_name_str);
            return "undefined";
        }
    } else {
        object_or_class_ref_str = evaluate_expression(target_expr_node, frame);
    }
    
    // Early universal .length support for plain strings and pseudo array literals
    if (object_or_class_ref_str && strcmp(property_name_str, "length") == 0) {
        if (object_or_class_ref_str[0] == '[') {
            /* Count only top-level elements: keep track of nested bracket depth so that
               commas inside sub-arrays are ignored.  This prevents exaggerated length
               values for multidimensional literals like [[1,2],[3,4]]. */
            int depth = 0;
            int elem_count = 0;
            int in_elem = 0;

            for (const char *p = object_or_class_ref_str + 1; *p && !(depth == 0 && *p == ']'); ++p) {
                char ch = *p;
                if (ch == '[') {
                    depth++;
                    in_elem = 1;
                }
                else if (ch == ']') {
                    if (depth > 0) depth--; /* close nested */
                    in_elem = 1;
                }
                else if (ch == ',' && depth == 0) {
                    /* End of a top-level element */
                    elem_count++;
                    in_elem = 0;
                }
                else if (!isspace((unsigned char)ch)) {
                    in_elem = 1; /* non-space char inside current element */
                }
            }
            if (in_elem) elem_count++; /* account for final element if any */

            static char len_buf[32];
            snprintf(len_buf, sizeof(len_buf), "%d", elem_count);
            return len_buf;
        } else {
            static char len_buf[32];
            snprintf(len_buf, sizeof(len_buf), "%zu", strlen(object_or_class_ref_str));
            return len_buf;
        }
    }
    
    if (!object_or_class_ref_str || strcmp(object_or_class_ref_str, "undefined") == 0) {
        // Semantic analysis should catch most of these if target_expr_node->value is an undeclared identifier
        // This error might still occur if evaluate_expression for target_expr_node results in "undefined" at runtime
        // printf("[VM EVAL_MEMBER_ACCESS] Error: Cannot access property '%s' of undefined or unresolved target '%s' (L%d).\n", 
        //        property_name_str, target_expr_node->value, member_access_expr_node->line);
        return "undefined";
    }

    if (strncmp(object_or_class_ref_str, "obj:", 4) == 0) { 
        int obj_id = atoi(object_or_class_ref_str + 4);
        Object *target_obj = find_object_by_id(obj_id);
        if (target_obj) {
            return get_object_property_with_access(target_obj, property_name_str, current_class);
        } else {
            fprintf(stderr, "Error (L%d:%d): Object %s not found for property access '%s'.\n", member_access_expr_node->line, member_access_expr_node->col, object_or_class_ref_str, property_name_str);
            return "undefined";
        }
    } else { 
        const char* class_name_str = object_or_class_ref_str;
        ClassEntry* ce = find_class_entry(class_name_str); // Check if it's a known class
        if (!ce) { // If not a registered class, it might be some other non-object string
             fprintf(stderr, "Error (L%d:%d): Target '%s' for member access '%s' is not a known class or object instance.\n", 
                target_expr_node->line, target_expr_node->col, class_name_str, property_name_str);
            return "undefined";
        }
        Object *static_obj = find_static_class_object(class_name_str); 
        if (static_obj) {
            return get_object_property_with_access(static_obj, property_name_str, current_class);
        } else {
             fprintf(stderr, "Error (L%d:%d): Could not find/create static object for class '%s' to access '%s'.\n", 
                target_expr_node->line, target_expr_node->col, class_name_str, property_name_str);
            return "undefined";
        }
    }
}

Object* find_object_by_id(int id) {
    Object *obj_iter = objects;
    while (obj_iter) {
        int current_obj_id = 0;
        char *hash_pos = strchr(obj_iter->class_name, '#');
        if (hash_pos) {
            current_obj_id = atoi(hash_pos + 1);
            if (current_obj_id == id) return obj_iter;
        }
        obj_iter = obj_iter->next;
    }
    return NULL;
}

Object* find_static_class_object(const char *class_name) {
    char static_obj_prefix[128 + 8]; 
    snprintf(static_obj_prefix, sizeof(static_obj_prefix), "%s_static", class_name); 

    Object *obj_iter = objects;
    while (obj_iter) {
        if (strncmp(obj_iter->class_name, static_obj_prefix, strlen(static_obj_prefix)) == 0) {
             char char_after_prefix = obj_iter->class_name[strlen(static_obj_prefix)];
             if (char_after_prefix == '#' || char_after_prefix == '\0') return obj_iter;
        }
        obj_iter = obj_iter->next;
    }
    return create_object(static_obj_prefix); 
}

void initialize_test_class(Object *obj) {
    if (!obj || strstr(obj->class_name, "TestClass") == NULL) return; 
    if (strstr(obj->class_name, "_static") != NULL) { 
        set_object_property_with_access(obj, "static_prop", "Static Property Value", ACCESS_MODIFIER_PUBLIC, 1);
    } else { 
        set_object_property_with_access(obj, "public_prop", "Public Property Value", ACCESS_MODIFIER_PUBLIC, 0);
        set_object_property_with_access(obj, "private_prop", "Private Property Value", ACCESS_MODIFIER_PRIVATE, 0);
        Object* static_companion = find_static_class_object("TestClass");
        if(static_companion && get_object_property_with_access_check(static_companion, "static_prop", "TestClass") == NULL) {
             set_object_property_with_access(static_companion, "static_prop", "Static Property Value", ACCESS_MODIFIER_PUBLIC, 1);
        }
    }
}

/// Bridge to stdlib.c's call_builtin_function
const char* call_built_in_function(const char* func_name_to_call, ASTNode* args_ast_list, StackFrame* frame_for_evaluating_args) {
    if (!func_name_to_call) return "undefined";

    int arg_count = 0;
    ASTNode *iter = args_ast_list;
    while (iter) { arg_count++; iter = iter->next; }

    const char **arg_values_evaluated = NULL; // Array of C-string pointers
    if (arg_count > 0) {
        arg_values_evaluated = (const char**)calloc(arg_count, sizeof(char*)); // Use calloc
        if (!arg_values_evaluated) {
            fprintf(stderr, "VM Error (L%d): Out of memory marshalling args for builtin '%s'.\n", args_ast_list ? args_ast_list->line : 0, func_name_to_call);
            return "undefined";
        }
        iter = args_ast_list;
        for (int i = 0; i < arg_count; ++i) {
            arg_values_evaluated[i] = evaluate_expression(iter, frame_for_evaluating_args);
            iter = iter->next;
        }
    }
    
    // Corrected function call
    int was_found_and_called = call_builtin_function_impl(func_name_to_call, arg_values_evaluated, arg_count);

    if (arg_values_evaluated) free((void*)arg_values_evaluated);

    if (was_found_and_called) {
        return get_return_value(); 
    }
    return "undefined"; 
}

static void initialize_default_instance_fields(const char *class_name_param, Object *instance_obj, StackFrame *frame_for_eval) {
    // Initialize fields from this class
    ClassEntry* class_entry = find_class_entry(class_name_param);
    if (class_entry) {
        ASTNode* class_node = class_entry->class_node;
        if (class_node && class_node->left) {
            ASTNode* member = class_node->left;
            while (member) {
                if (member->type == AST_CLASS_FIELD) {
                    // Initialize field with default value or expression
                    if (member->left) {
                        // Prepare buffer for field value
                        char value[256] = "";
                        const char* result = evaluate_expression(member->left, frame_for_eval);
                        if (result) {
                            strncpy(value, result, sizeof(value) - 1);
                        }
                        set_object_property_with_access(instance_obj, member->value, value, ACCESS_MODIFIER_PUBLIC, 0);
                    }
                }
                member = member->next;
            }
        }
    }

    // Initialize parent class fields if any
    const char* parent_name = get_parent_class_name(class_name_param);
    if (parent_name && strlen(parent_name) > 0) {
        initialize_default_instance_fields(parent_name, instance_obj, frame_for_eval);
    }
}

const char* get_parent_class_name(const char *class_name) {
    ClassEntry *entry = find_class_entry(class_name);
    if (entry && entry->parent_name[0]) return entry->parent_name;
    return NULL;
}

#if 0  // disable stub frame and local functions
StackFrame* push_frame(StackFrame* caller_frame) {
    StackFrame* new_frame = (StackFrame*)malloc(sizeof(StackFrame));
    if (!new_frame) return NULL;
    
    new_frame->symbol_table = symbol_table_create();
    if (caller_frame) {
        strncpy(new_frame->current_class, caller_frame->current_class, sizeof(new_frame->current_class) - 1);
    } else {
        new_frame->current_class[0] = '\0';
    }
    return new_frame;
}

void pop_frame(void) {
    // Assume global current_frame exists
    if (current_frame) {
        symbol_table_free(current_frame->symbol_table);
        free(current_frame);
        current_frame = NULL; // Or set to previous frame if tracking a stack
    }
}

void define_local(StackFrame* frame, const char* name, const char* value) {
    if (!frame || !name || !value) return;
    symbol_table_add_symbol(frame->symbol_table, name, SYMBOL_VARIABLE, "any", NULL); // Adjust type as needed
}

const char* run_vm_node(ASTNode* node, StackFrame* frame) {
    if (!node || !frame) return NULL;
    // Placeholder: Implement actual VM logic here
    return "";
}
#endif
