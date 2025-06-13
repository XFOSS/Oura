#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include "eval.h"
#include "ast_types.h"
#include "vm.h" // For Object, find_object_by_id, find_static_class_object, current_class, execute_function_call, etc.
#include "semantic.h" // For Symbol, SymbolTable related types (if needed directly, though usually through vm)

#define MAX_RESULT_LENGTH 1024

extern ASTNode *program; 
static char result_buffer[1024]; // Static buffer for string results of operations

// Forward declarations for VM helpers when vm.h not available in include path
#ifndef VM_HELPERS_DECL
#define VM_HELPERS_DECL
extern Object* create_object(const char *class_name);
extern void set_object_property(Object *obj, const char *name, const char *value);
extern void register_user_function(ASTNode *func_node);
extern ASTNode* find_user_function(const char *name, const char *class_name);
#endif

// Helper function to check if a string represents a number
int is_numeric_string(const char* str) {
    if (!str || !*str) return 0;
    
    // Check for negative sign
    if (*str == '-') str++;
    
    // Empty string after negative sign or just a negative sign is not a number
    if (!*str) return 0;
    
    int has_decimal = 0;
    
    while (*str) {
        if (*str == '.') {
            // Only one decimal point allowed
            if (has_decimal) return 0;
            has_decimal = 1;
        } else if (!isdigit(*str)) {
            return 0;  // Non-digit character found
        }
        str++;
    }
    
    return 1;
}

// Forward declarations for internal functions
#define LOCAL_COPY_IF_ALIAS(ptr, bufname) \
    const char *bufname = (ptr);          \
    char bufname##_copy[512];             \
    if ((ptr) == result_buffer) {         \
        strncpy(bufname##_copy, (ptr), sizeof(bufname##_copy)-1); \
        bufname##_copy[sizeof(bufname##_copy)-1] = '\0';          \
        bufname = bufname##_copy;         \
    }

static const char* evaluate_binary_op_internal(ASTNode* expr_node, const char *op_str, const char *left_val_str_final, const char *right_val_str_final);
const char* evaluate_member_access(ASTNode *member_access_expr_node, StackFrame *frame); // Added forward declaration


const char* evaluate_expression(ASTNode *expr_node, StackFrame *frame) {
    if (!expr_node) return "undefined";
    
    switch (expr_node->type) {
        case AST_LITERAL:
            return expr_node->value;
            
        case AST_IDENTIFIER: {
            const char *var_name = expr_node->value;
            const char *value_str = get_variable(frame, var_name);
            if (value_str) return value_str;

            if (current_class[0] != '\0') {
                const char *this_obj_ref_str = get_variable(frame, "this");
                if (this_obj_ref_str && strncmp(this_obj_ref_str, "obj:", 4) == 0) {
                    int this_obj_id = atoi(this_obj_ref_str + 4);
                    Object *this_obj = find_object_by_id(this_obj_id);
                    if (this_obj) {
                        const char *instance_member_val = get_object_property_with_access(this_obj, var_name, current_class);
                        if (instance_member_val && strcmp(instance_member_val, "undefined") != 0) {
                            return instance_member_val;
                        }
                    }
                }
                Object *static_obj_for_current_class = find_static_class_object(current_class);
                if (static_obj_for_current_class) {
                     const char *static_member_val = get_object_property_with_access(static_obj_for_current_class, var_name, current_class);
                     if (static_member_val && strcmp(static_member_val, "undefined") != 0) {
                        return static_member_val;
                     }
                }
            }
            
            if (isupper((unsigned char)var_name[0])) { 
                 // Check if it's a registered class to return its name for static member access
                 // This needs vm.c to expose a "is_class_registered" or similar.
                 // For now, relying on semantic analysis to have typed it or this heuristic.
                 return var_name; 
            }

            // fprintf(stderr, "Error (L%d:%d): Undefined identifier '%s'.\n", expr_node->line, expr_node->col, var_name);
            return "undefined";
        }
            
        case AST_BINARY_OP: {
            if (strcmp(expr_node->value, "=") == 0 || strcmp(expr_node->value, "+=") == 0 || strcmp(expr_node->value, "-=") == 0 || strcmp(expr_node->value, "*=") == 0 || strcmp(expr_node->value, "/=") == 0 || strcmp(expr_node->value, "%=") == 0) { 
                const char *rhs_val_str = evaluate_expression(expr_node->right, frame);

                /* For compound assignments, compute new RHS as lhs <op> rhs */
                const char *effective_rhs = rhs_val_str;
                char compound_result_buf[64];
                if (strcmp(expr_node->value, "=") != 0) {
                    /* Need current LHS value */
                    const char *lhs_current_val = evaluate_expression(expr_node->left, frame);
                    const char *op_for_compound = NULL;
                    if (strcmp(expr_node->value, "+=") == 0) op_for_compound = "+";
                    else if (strcmp(expr_node->value, "-=") == 0) op_for_compound = "-";
                    else if (strcmp(expr_node->value, "*=") == 0) op_for_compound = "*";
                    else if (strcmp(expr_node->value, "/=") == 0) op_for_compound = "/";
                    else if (strcmp(expr_node->value, "%=") == 0) op_for_compound = "%"; /* modulus to implement later */

                    if (op_for_compound) {
                        const char *temp_res = evaluate_binary_op_internal(expr_node, op_for_compound, lhs_current_val, rhs_val_str);
                        strncpy(compound_result_buf, temp_res, sizeof(compound_result_buf)-1);
                        compound_result_buf[sizeof(compound_result_buf)-1] = '\0';
                        effective_rhs = compound_result_buf;
                    }
                }

                if (expr_node->left->type == AST_IDENTIFIER) {
                    set_variable(frame, expr_node->left->value, effective_rhs);
                    return effective_rhs; 
                } else if (expr_node->left->type == AST_MEMBER_ACCESS) {
                    ASTNode *member_access = expr_node->left; 
                    ASTNode *target_node = member_access->left;   
                    const char *prop_name = member_access->value; 

                    const char *target_ref_str;
                    if (target_node->type == AST_THIS) target_ref_str = get_variable(frame, "this");
                    else target_ref_str = evaluate_expression(target_node, frame);

                    if (target_ref_str && strncmp(target_ref_str, "obj:", 4) == 0) { 
                        int obj_id = atoi(target_ref_str + 4);
                        Object *obj_instance = find_object_by_id(obj_id);
                        if (obj_instance) {
                            set_object_property_with_access(obj_instance, prop_name, effective_rhs, ACCESS_MODIFIER_PUBLIC, 0);
                            return effective_rhs;
                        } else { fprintf(stderr, "Error (L%d:%d): Object %s not found for assignment to '%s'.\n", member_access->line, member_access->col, target_ref_str, prop_name); }
                    } else if (target_ref_str && isupper((unsigned char)target_ref_str[0])) { // Assume ClassName for static
                        Object* static_obj = find_static_class_object(target_ref_str);
                        if (static_obj) {
                             set_object_property_with_access(static_obj, prop_name, effective_rhs, ACCESS_MODIFIER_PUBLIC, 1);
                             return effective_rhs;
                        } else { fprintf(stderr, "Error (L%d:%d): Class %s not found for static assignment to '%s'.\n", target_node->line, target_node->col, target_ref_str, prop_name); }
                    } else { fprintf(stderr, "Error (L%d:%d): Invalid target for member assignment to '%s'. Target was '%s'\n", target_node->line, target_node->col, prop_name, target_ref_str ? target_ref_str : "null");}
                    return "undefined"; 
                } else {
                    fprintf(stderr, "Error (L%d:%d): Invalid left-hand side in assignment.\n", expr_node->left->line, expr_node->left->col);
                    return "undefined";
                }
            } else { 
                const char *left_eval_result = evaluate_expression(expr_node->left, frame);
                char left_stable_val[1024]; 
                const char* left_final_val_ptr;

                if (left_eval_result == result_buffer) { 
                    strncpy(left_stable_val, result_buffer, sizeof(left_stable_val) - 1);
                    left_stable_val[sizeof(left_stable_val) - 1] = '\0';
                    left_final_val_ptr = left_stable_val;
                } else {
                    left_final_val_ptr = left_eval_result; 
                }

                if (strcmp(expr_node->value, "&&") == 0) {
                    int left_truthy = left_final_val_ptr && strcmp(left_final_val_ptr, "0") != 0 && strcmp(left_final_val_ptr, "false") != 0 && strcmp(left_final_val_ptr, "") != 0;
                    if (!left_truthy) return "false"; 
                    const char *right_eval_result = evaluate_expression(expr_node->right, frame);
                    int right_truthy = right_eval_result && strcmp(right_eval_result, "0") != 0 && strcmp(right_eval_result, "false") != 0 && strcmp(right_eval_result, "") != 0;
                    return right_truthy ? "true" : "false";
                }
                if (strcmp(expr_node->value, "||") == 0) {
                    int left_truthy = left_final_val_ptr && strcmp(left_final_val_ptr, "0") != 0 && strcmp(left_final_val_ptr, "false") != 0 && strcmp(left_final_val_ptr, "") != 0;
                    if (left_truthy) return "true"; 
                    const char *right_eval_result = evaluate_expression(expr_node->right, frame);
                    int right_truthy = right_eval_result && strcmp(right_eval_result, "0") != 0 && strcmp(right_eval_result, "false") != 0 && strcmp(right_eval_result, "") != 0;
                    return right_truthy ? "true" : "false";
                }
                const char *right_eval_result = evaluate_expression(expr_node->right, frame);
                return evaluate_binary_op_internal(expr_node, expr_node->value, left_final_val_ptr, right_eval_result);
            }
        } // End AST_BINARY_OP
        case AST_UNARY_OP: {
            const char *operand_val_str = evaluate_expression(expr_node->left, frame);
            if (strcmp(expr_node->value, "-") == 0) {
                if (!operand_val_str || !is_numeric_string(operand_val_str)) {
                     fprintf(stderr, "Error (L%d:%d): Unary '-' requires numeric operand, got '%s'.\n", expr_node->line, expr_node->col, operand_val_str ? operand_val_str : "undefined"); return "undefined";
                }
                double val = atof(operand_val_str);
                snprintf(result_buffer, sizeof(result_buffer), "%g", -val); return result_buffer;
            } else if (strcmp(expr_node->value, "!") == 0) {
                 int truthy = operand_val_str && strcmp(operand_val_str, "0") != 0 && strcmp(operand_val_str, "false") != 0 && strcmp(operand_val_str, "") != 0;
                 return truthy ? "false" : "true";
            } else if (strcmp(expr_node->value, "++") == 0 || strcmp(expr_node->value, "--") == 0) {
                 int delta = (expr_node->value[0] == '+') ? 1 : -1;
                 if (!operand_val_str || !is_numeric_string(operand_val_str)) {
                     fprintf(stderr, "Error (L%d:%d): '%s' operator requires numeric operand, got '%s'.\n", expr_node->line, expr_node->col, expr_node->value, operand_val_str ? operand_val_str : "undefined");
                     return "undefined";
                 }
                 int new_val = atoi(operand_val_str) + delta;
                 char new_val_str[64]; snprintf(new_val_str, sizeof(new_val_str), "%d", new_val);

                 /* Assign back to operand (identifier or member) */
                 if (expr_node->left->type == AST_IDENTIFIER) {
                     set_variable(frame, expr_node->left->value, new_val_str);
                 } else if (expr_node->left->type == AST_MEMBER_ACCESS) {
                     ASTNode *member_access = expr_node->left;
                     const char *prop_name = member_access->value;
                     const char *target_ref_str;
                     if (member_access->left->type == AST_THIS) target_ref_str = get_variable(frame, "this");
                     else target_ref_str = evaluate_expression(member_access->left, frame);

                     if (target_ref_str && strncmp(target_ref_str, "obj:", 4) == 0) {
                         int obj_id = atoi(target_ref_str + 4);
                         Object *obj_instance = find_object_by_id(obj_id);
                         if (obj_instance) {
                             set_object_property_with_access(obj_instance, prop_name, new_val_str, ACCESS_MODIFIER_PUBLIC, 0);
                         }
                     }
                 }
                 strncpy(result_buffer, new_val_str, sizeof(result_buffer)-1);
                 result_buffer[sizeof(result_buffer)-1] = '\0';
                 return result_buffer;
            }
            fprintf(stderr, "Error (L%d:%d): Unknown unary operator '%s'.\n", expr_node->line, expr_node->col, expr_node->value);
            return "undefined";
        }
        case AST_CALL: {
            // Dispatch user-defined class methods on instances
            char qualified_name_buffer[512];
            if (expr_node->right) {
                const char *t = evaluate_expression(expr_node->right, frame);
                if (t && strncmp(t, "obj:", 4) == 0) {
                    int inst_id = atoi(t + 4);
                    Object *inst = find_object_by_id(inst_id);
                    if (inst) {
                        // Extract class name before '#' in instance class_name
                        char class_name_only[128];
                        char *hash_pos = strchr(inst->class_name, '#');
                        if (hash_pos) {
                            size_t class_len = hash_pos - inst->class_name;
                            if (class_len < sizeof(class_name_only)) {
                                strncpy(class_name_only, inst->class_name, class_len);
                                class_name_only[class_len] = '\0';
                            } else {
                                strncpy(class_name_only, inst->class_name, sizeof(class_name_only) - 1);
                                class_name_only[sizeof(class_name_only) - 1] = '\0';
                            }
                        } else {
                            strncpy(class_name_only, inst->class_name, sizeof(class_name_only) - 1);
                            class_name_only[sizeof(class_name_only) - 1] = '\0';
                        }
                        snprintf(qualified_name_buffer, sizeof(qualified_name_buffer), "%s.%s", class_name_only, expr_node->value);
                    } else {
                        // Fallback if instance not found
                        snprintf(qualified_name_buffer, sizeof(qualified_name_buffer), "%s.%s", t, expr_node->value);
                    }
                } else {
                    // Plain method or function call on class/static
                    snprintf(qualified_name_buffer, sizeof(qualified_name_buffer), "%s.%s", t ? t : "undefined_target", expr_node->value);
                }
            } else {
                // Simple function call
                strncpy(qualified_name_buffer, expr_node->value, sizeof(qualified_name_buffer) - 1);
                qualified_name_buffer[sizeof(qualified_name_buffer) - 1] = '\0';
            }
            return execute_function_call(qualified_name_buffer, expr_node->left, frame);
        }
        case AST_ARRAY: {
            // Simplified: if parser put elements in value, use that. Otherwise, placeholder.
            if (expr_node->value[0] != '\0' && strcmp(expr_node->value, "array_literal") != 0) {
                return expr_node->value; 
            } else if (expr_node->left) { // Chain of expression nodes for elements
                // This needs to build a string representation or an actual array object.
                // For now, very basic string join into result_buffer for demo.
                result_buffer[0] = '['; result_buffer[1] = '\0';
                ASTNode* elem = expr_node->left;
                int first = 1;
                size_t current_len = 1;
                while(elem) {
                    if (!first) { strncat(result_buffer, ",", sizeof(result_buffer) - current_len -1); current_len++;}
                    const char* elem_val = evaluate_expression(elem, frame);
                    strncat(result_buffer, elem_val, sizeof(result_buffer) - current_len -1);
                    current_len += strlen(elem_val);
                    first = 0;
                    elem = elem->next;
                }
                strncat(result_buffer, "]", sizeof(result_buffer) - current_len -1);
                return result_buffer;
            }
            return "[array_obj_ref]"; 
        }
        case AST_NEW: {
            if (!expr_node->value[0]) {
                fprintf(stderr, "Error (L%d:%d): Class name missing in new expression\n", expr_node->line, expr_node->col);
                return "undefined";
            }
            Object *obj = create_object(expr_node->value); 
            if (!obj) {
                fprintf(stderr, "Error (L%d:%d): Failed to create object of class '%s'\n", expr_node->line, expr_node->col, expr_node->value);
                return "undefined";
            }
            int obj_id_val = 0;
            sscanf(obj->class_name, "%*[^#]#%d", &obj_id_val);
            snprintf(result_buffer, sizeof(result_buffer), "obj:%d", obj_id_val); 

            /* Only attempt to invoke constructor if user actually defined one */
            if (find_user_function(expr_node->value, expr_node->value)) {
                // Invoke constructor using qualified name and args list
                execute_function_call(expr_node->value, expr_node->left, frame);
            }
            return result_buffer; // Return the object reference string
        }
        case AST_MEMBER_ACCESS: {
            return evaluate_member_access(expr_node, frame);
        }
        case AST_THIS: {
            const char *this_val = get_variable(frame, "this");
            if (!this_val) {
                fprintf(stderr, "Error (L%d:%d): 'this' is undefined in current context.\n", expr_node->line, expr_node->col);
                return "undefined";
            }
            return this_val;
        }
        case AST_SUPER: {
            /* For now, treat 'super' the same as 'this' (no real inheritance yet) */
            const char *this_val = get_variable(frame, "this");
            if (!this_val) {
                fprintf(stderr, "Error (L%d:%d): 'super' is undefined in current context.\n", expr_node->line, expr_node->col);
                return "undefined";
            }
            extern char g_super_target_class[128];
            const char *parent = NULL;
            extern const char* get_parent_class_name(const char* class_name);
            parent = get_parent_class_name(current_class);
            if (parent) { strncpy(g_super_target_class, parent, sizeof(g_super_target_class)-1); g_super_target_class[sizeof(g_super_target_class)-1] = '\0'; }
            else g_super_target_class[0] = '\0';
            return this_val;
        }
        case AST_INDEX_ACCESS: {
            const char* target_val_str = evaluate_expression(expr_node->left, frame);
            const char* index_val_str = evaluate_expression(expr_node->right, frame);
            
            // Rudimentary string/array indexing for demo, add guard to suppress spam
            if (!target_val_str || strcmp(target_val_str, "undefined") == 0) {
                return "undefined"; // nothing to index
            }

            // Handle pseudo array literal represented as "[a,b,c]"
            if (target_val_str[0] == '[' && target_val_str[strlen(target_val_str)-1] == ']') {
                // This is a placeholder for actual array object indexing
                // It assumes target_val_str is like "[elem1,elem2,elem3]"
                int index_num = atoi(index_val_str);
                /* Depth-aware tokenisation so commas inside nested sub-arrays are ignored. */
                char token_buf[512]; int token_len = 0; int depth = 0; int curr_idx = 0; int found = 0;
                const char *p = target_val_str + 1; /* skip opening '[' */
                for (; *p && !(depth == 0 && *p == ']'); ++p) {
                    char ch = *p;
                    if (ch == '[') { depth++; token_buf[token_len++] = ch; }
                    else if (ch == ']') { depth--; token_buf[token_len++] = ch; }
                    else if (ch == ',' && depth == 0) {
                        token_buf[token_len] = '\0';
                        if (curr_idx == index_num) { found = 1; break; }
                        curr_idx++; token_len = 0; /* reset for next token */
                    } else {
                        token_buf[token_len++] = ch;
                    }
                    if (token_len >= (int)sizeof(token_buf)-1) token_len = sizeof(token_buf)-2; /* prevent overflow */
                }
                if (!found) {
                    token_buf[token_len] = '\0'; /* null terminate last element */
                    if (curr_idx == index_num) found = 1; /* last element matches */
                }
                if (found) {
                    /* Trim leading/trailing whitespace */
                    char *start = token_buf; while(isspace((unsigned char)*start)) start++;
                    char *end = start + strlen(start) - 1; while(end >= start && isspace((unsigned char)*end)) { *end = '\0'; end--; }
                    strncpy(result_buffer, start, sizeof(result_buffer)-1);
                    result_buffer[sizeof(result_buffer)-1] = '\0';
                    return result_buffer;
                }
                if (index_num < 50) {
                    fprintf(stderr, "Warning (L%d:%d): Index %d out of bounds for pseudo-array.\n", expr_node->line, expr_node->col, index_num);
                }
                return "undefined";
            } else if (is_numeric_string(index_val_str)) { // Basic string indexing on plain string
                int index = atoi(index_val_str);
                if (index >= 0 && index < (int)strlen(target_val_str)) {
                    snprintf(result_buffer, 2, "%c", target_val_str[index]);
                    return result_buffer;
                } else {
                    // warn once, but avoid spamming by length threshold
                    if (index < 50) {
                        fprintf(stderr, "Warning (L%d:%d): Index %d out of bounds for string '%s'.\n", expr_node->line, expr_node->col, index, target_val_str);
                    }
                    return "undefined";
                }
            }
            // Fallback for unhandled index access
            snprintf(result_buffer, sizeof(result_buffer), "indexed_value_of_%s_at_%s", target_val_str ? target_val_str : "null", index_val_str ? index_val_str : "null");
            return result_buffer;
        }
        case AST_TERNARY: {
            const char *cond_val = evaluate_expression(expr_node->left, frame);
            int truthy = cond_val && strcmp(cond_val, "0") != 0 && strcmp(cond_val, "false") != 0 && strcmp(cond_val, "") != 0;
            if (truthy) return evaluate_expression(expr_node->right, frame);
            else if (expr_node->next) return evaluate_expression(expr_node->next, frame);
            return "undefined";
        }
        case AST_FUNCTION: {
            /* Anonymous function expression – register on first evaluation and return its name */
            if (!find_user_function(expr_node->value, NULL)) {
                register_user_function(expr_node);
            }
            return expr_node->value; // function name string acts as reference
        }
        case AST_MAP: {
            /* Create a real runtime object instead of serialising to a string */
            Object *map_obj = create_object("Object");
            if (!map_obj) return "undefined";

            ASTNode *pair = expr_node->left;
            while (pair) {
                const char *key_str;
                if (pair->left->type == AST_IDENTIFIER || pair->left->type == AST_LITERAL) {
                    key_str = pair->left->value; // use raw token text
                } else {
                    key_str = evaluate_expression(pair->left, frame);
                }

                const char *val_result;
                if (pair->right->type == AST_FUNCTION) {
                    /* Evaluate will register and return the synthetic name */
                    val_result = evaluate_expression(pair->right, frame);
                } else {
                    val_result = evaluate_expression(pair->right, frame);
                }
                set_object_property_with_access(map_obj, key_str, val_result, ACCESS_MODIFIER_PUBLIC, 1);
                pair = pair->next;
            }

            /* Build and return the obj reference string */
            static char obj_ref_buf[32];
            int obj_id_val = 0;
            sscanf(map_obj->class_name, "%*[^#]#%d", &obj_id_val);
            snprintf(obj_ref_buf, sizeof(obj_ref_buf), "obj:%d", obj_id_val);
            return obj_ref_buf;
        }
        default:
            fprintf(stderr, "Error (L%d:%d): Cannot evaluate unknown AST node type %s (%d).\n", expr_node->line, expr_node->col, node_type_to_string(expr_node->type), expr_node->type);
            return "undefined";
    }
}

// Forward declaration for binary op evaluation
static const char* evaluate_binary_op_internal(ASTNode* expr_node, const char *op_str, const char *left_val_str_final, const char *right_val_str_final) {
    (void)expr_node;
    /* Single shared buffer for return value */
    static char result_buffer[MAX_RESULT_LENGTH];
    result_buffer[0] = '\0';

    /* Ensure we never read from the same buffer we are about to overwrite */
    LOCAL_COPY_IF_ALIAS(left_val_str_final, safe_left);
    LOCAL_COPY_IF_ALIAS(right_val_str_final, safe_right);
    
    // Arithmetic operations
    if (strcmp(op_str, "+") == 0) {
        // Numeric addition when BOTH operands are numeric
        if (is_numeric_string(safe_left) && is_numeric_string(safe_right)) {
            double left_num = atof(safe_left);
            double right_num = atof(safe_right);
            double result = left_num + right_num;

            // Emit integer without decimal part when possible
            if (result == (int)result) {
                snprintf(result_buffer, sizeof(result_buffer), "%d", (int)result);
            } else {
                snprintf(result_buffer, sizeof(result_buffer), "%g", result);
            }
        } else {
            // Treat as string concatenation otherwise (default behaviour in many scripting languages)
            snprintf(result_buffer, sizeof(result_buffer), "%s%s", safe_left ? safe_left : "", safe_right ? safe_right : "");
        }
    }
    else if (strcmp(op_str, "-") == 0) {
        if (is_numeric_string(safe_left) && is_numeric_string(safe_right)) {
            double left_num = atof(safe_left);
            double right_num = atof(safe_right);
            double result = left_num - right_num;
            
            if (result == (int)result) {
                snprintf(result_buffer, sizeof(result_buffer), "%d", (int)result);
            } else {
                snprintf(result_buffer, sizeof(result_buffer), "%g", result);
            }
        }
    }
    else if (strcmp(op_str, "*") == 0) {
        if (is_numeric_string(safe_left) && is_numeric_string(safe_right)) {
            double left_num = atof(safe_left);
            double right_num = atof(safe_right);
            double result = left_num * right_num;
            
            if (result == (int)result) {
                snprintf(result_buffer, sizeof(result_buffer), "%d", (int)result);
            } else {
                snprintf(result_buffer, sizeof(result_buffer), "%g", result);
            }
        }
    }
    else if (strcmp(op_str, "/") == 0) {
        if (is_numeric_string(safe_left) && is_numeric_string(safe_right)) {
            double left_num = atof(safe_left);
            double right_num = atof(safe_right);
            
            if (right_num == 0) {
                fprintf(stderr, "[RUNTIME] Error: Division by zero\n");
                strcpy(result_buffer, "NaN");
            } else {
                double result = left_num / right_num;
                
                if (result == (int)result) {
                    snprintf(result_buffer, sizeof(result_buffer), "%d", (int)result);
                } else {
                    snprintf(result_buffer, sizeof(result_buffer), "%g", result);
                }
            }
        }
    }
    else if (strcmp(op_str, "%") == 0) {
        if (is_numeric_string(safe_left) && is_numeric_string(safe_right)) {
            long left_num = atol(safe_left);
            long right_num = atol(safe_right);
            if (right_num == 0) {
                fprintf(stderr, "[RUNTIME] Error: Modulus by zero\n");
                strcpy(result_buffer, "NaN");
            } else {
                long result = left_num % right_num;
                snprintf(result_buffer, sizeof(result_buffer), "%ld", result);
            }
        }
    }
    else if (strcmp(op_str, "<<") == 0 || strcmp(op_str, ">>") == 0 || strcmp(op_str, ">>>") == 0) {
        if (is_numeric_string(safe_left) && is_numeric_string(safe_right)) {
            long left_num = atol(safe_left);
            int shift = atoi(safe_right);
            long result;
            if (strcmp(op_str, "<<") == 0) result = left_num << shift;
            else result = left_num >> shift; // '>>>' treated same as '>>' in this simple impl
            snprintf(result_buffer, sizeof(result_buffer), "%ld", result);
        }
    }
    
    // Comparison operations
    else if (strcmp(op_str, "==") == 0) {
        int result;
        if (is_numeric_string(safe_left) && is_numeric_string(safe_right)) {
            double left_num = atof(safe_left);
            double right_num = atof(safe_right);
            result = (left_num == right_num);
        } else {
            result = (strcmp(safe_left, safe_right) == 0);
        }
        strcpy(result_buffer, result ? "true" : "false");
    }
    else if (strcmp(op_str, "!=") == 0) {
        int result;
        if (is_numeric_string(safe_left) && is_numeric_string(safe_right)) {
            double left_num = atof(safe_left);
            double right_num = atof(safe_right);
            result = (left_num != right_num);
        } else {
            result = (strcmp(safe_left, safe_right) != 0);
        }
        strcpy(result_buffer, result ? "true" : "false");
    }
    else if (strcmp(op_str, "<") == 0) {
        int result;
        if (is_numeric_string(safe_left) && is_numeric_string(safe_right)) {
            double left_num = atof(safe_left);
            double right_num = atof(safe_right);
            result = (left_num < right_num);
        } else {
            result = (strcmp(safe_left, safe_right) < 0);
        }
        strcpy(result_buffer, result ? "true" : "false");
    }
    else if (strcmp(op_str, ">") == 0) {
        int result;
        if (is_numeric_string(safe_left) && is_numeric_string(safe_right)) {
            double left_num = atof(safe_left);
            double right_num = atof(safe_right);
            result = (left_num > right_num);
        } else {
            result = (strcmp(safe_left, safe_right) > 0);
        }
        strcpy(result_buffer, result ? "true" : "false");
    }
    else if (strcmp(op_str, "<=") == 0) {
        int result;
        if (is_numeric_string(safe_left) && is_numeric_string(safe_right)) {
            double left_num = atof(safe_left);
            double right_num = atof(safe_right);
            result = (left_num <= right_num);
        } else {
            result = (strcmp(safe_left, safe_right) <= 0);
        }
        strcpy(result_buffer, result ? "true" : "false");
    }
    else if (strcmp(op_str, ">=") == 0) {
        int result;
        if (is_numeric_string(safe_left) && is_numeric_string(safe_right)) {
            double left_num = atof(safe_left);
            double right_num = atof(safe_right);
            result = (left_num >= right_num);
        } else {
            result = (strcmp(safe_left, safe_right) >= 0);
        }
        strcpy(result_buffer, result ? "true" : "false");
    }
    
    // Logical operations
    else if (strcmp(op_str, "&&") == 0) {
        int left_bool = (strcmp(safe_left, "true") == 0);
        int right_bool = (strcmp(safe_right, "true") == 0);
        strcpy(result_buffer, (left_bool && right_bool) ? "true" : "false");
    }
    else if (strcmp(op_str, "||") == 0) {
        int left_bool = (strcmp(safe_left, "true") == 0);
        int right_bool = (strcmp(safe_right, "true") == 0);
        strcpy(result_buffer, (left_bool || right_bool) ? "true" : "false");
    }
    
    return result_buffer;
}
