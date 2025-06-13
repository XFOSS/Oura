#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h> // For isupper
#include "semantic.h" 
#include "ast_types.h"
#include "parser.h" // For is_builtin_type_keyword
#include "vm.h" // For AccessModifierEnum

// --- Symbol Table Implementation ---
SymbolTable* g_st = NULL; 

// --- Forward declarations for analysis functions ---
static void analyze_node(ASTNode *node); 
static void analyze_function_decl(ASTNode *func_node, ASTNode *parent_class_node_or_null);
static void analyze_block_stmts(ASTNode *block_node);
static void analyze_var_decl_stmt(ASTNode *decl_node);
static void analyze_assignment_stmt(ASTNode *assign_node);
static void analyze_return_stmt(ASTNode *return_node);
static void analyze_if_stmt(ASTNode *if_node);
static void analyze_while_stmt(ASTNode *while_node);
static void analyze_for_stmt(ASTNode *for_node);
static void analyze_call_expr_or_stmt(ASTNode *call_node);
static const char* analyze_expression_node(ASTNode *expr_node); 
static void analyze_struct_decl(ASTNode *struct_node);
static void analyze_class_decl(ASTNode *class_node);
static const char* analyze_member_access_expr(ASTNode *access_node);
static void analyze_new_expr(ASTNode *new_node);

// Added definition for is_builtin_type_keyword to fix linker issue
int is_builtin_type_keyword(const char* s);

SymbolTable* symbol_table_create() {
    SymbolTable* st = (SymbolTable*)malloc(sizeof(SymbolTable));
    if (!st) {
        fprintf(stderr, "Fatal Error: Could not allocate symbol table.\n");
        exit(EXIT_FAILURE);
    }
    st->current_scope_idx = -1; 
    st->next_scope_level_to_assign = 0;
    symbol_table_enter_scope(st, "global"); 
    return st;
}

void symbol_table_destroy(SymbolTable* st) {
    if (!st) return;
    while (st->current_scope_idx >= 0) {
        Scope* current = st->scope_stack[st->current_scope_idx];
        free(current);
        st->scope_stack[st->current_scope_idx] = NULL;
        st->current_scope_idx--;
    }
    free(st);
}

Scope* symbol_table_get_current_scope(SymbolTable* st) {
    if (!st || st->current_scope_idx < 0) {
        return NULL;
    }
    return st->scope_stack[st->current_scope_idx];
}

void symbol_table_enter_scope(SymbolTable* st, const char* scope_name) {
    if (!st) return;
    if (st->current_scope_idx + 1 >= MAX_SCOPE_DEPTH) {
        fprintf(stderr, "Fatal Error: Maximum scope depth (%d) exceeded for scope '%s'.\n", MAX_SCOPE_DEPTH, scope_name);
        exit(EXIT_FAILURE); 
    }
    Scope* new_scope = (Scope*)calloc(1, sizeof(Scope)); // Use calloc for zero-initialization
    if (!new_scope) {
        fprintf(stderr, "Fatal Error: Could not allocate new scope '%s'.\n", scope_name);
        exit(EXIT_FAILURE);
    }
    new_scope->parent_scope = (st->current_scope_idx >= 0) ? st->scope_stack[st->current_scope_idx] : NULL;
    new_scope->level = st->next_scope_level_to_assign++;
    strncpy(new_scope->scope_name, scope_name, sizeof(new_scope->scope_name) - 1);
    new_scope->scope_name[sizeof(new_scope->scope_name) - 1] = '\0';

    st->current_scope_idx++;
    st->scope_stack[st->current_scope_idx] = new_scope;
    // printf("[Scope] Entered scope: %s (level %d)\n", scope_name, new_scope->level);
}

void symbol_table_exit_scope(SymbolTable* st) {
    if (!st || st->current_scope_idx < 0) {
        fprintf(stderr, "Warning: Attempted to exit scope when no scope is active.\n");
        return;
    }
    Scope* exited_scope = st->scope_stack[st->current_scope_idx];
    // printf("[Scope] Exited scope: %s (level %d)\n", exited_scope->scope_name, exited_scope->level);
    
    free(exited_scope);
    st->scope_stack[st->current_scope_idx] = NULL;
    st->current_scope_idx--;
}

int symbol_table_add_symbol(SymbolTable* st, const char* name, SymbolKind kind, const char* type_name, ASTNode* decl_node) {
    if (!st) return 0;
    Scope* current_scope = symbol_table_get_current_scope(st);
    if (!current_scope) {
        fprintf(stderr, "Error (L%d:%d): Cannot add symbol '%s', no active scope.\n", decl_node->line, decl_node->col, name);
        return 0;
    }

    for (int i = 0; i < current_scope->symbol_count; ++i) {
        if (strcmp(current_scope->symbols[i].name, name) == 0) {
            fprintf(stderr, "[SEMANTIC L%d:%d] Error: Symbol '%s' already defined in this scope (previous def at L%d:%d as %s).\n",
                    decl_node->line, decl_node->col, name, 
                    current_scope->symbols[i].declaration_node->line, current_scope->symbols[i].declaration_node->col,
                    current_scope->symbols[i].type_name);
            return 0; 
        }
    }

    if (current_scope->symbol_count >= MAX_SCOPE_SYMBOLS) {
        fprintf(stderr, "Error (L%d:%d): Maximum symbols (%d) reached in scope '%s' when adding '%s'.\n",
                decl_node->line, decl_node->col, MAX_SCOPE_SYMBOLS, current_scope->scope_name, name);
        return 0;
    }

    Symbol* new_sym = &current_scope->symbols[current_scope->symbol_count];
    strncpy(new_sym->name, name, sizeof(new_sym->name) - 1);
    new_sym->name[sizeof(new_sym->name) - 1] = '\0';
    new_sym->kind = kind;
    if (type_name) {
        strncpy(new_sym->type_name, type_name, sizeof(new_sym->type_name) - 1);
        new_sym->type_name[sizeof(new_sym->type_name) - 1] = '\0';
    } else {
        strcpy(new_sym->type_name, "unknown_type"); 
    }
    new_sym->declaration_node = decl_node;
    new_sym->scope_level = current_scope->level;

    current_scope->symbol_count++;
    // printf("  [Symbol Added in %s (L%d)]: %s (Kind: %d, Type: %s)\n", current_scope->scope_name, decl_node->line, name, kind, new_sym->type_name);
    return 1; 
}

Symbol* symbol_table_lookup_current_scope(SymbolTable* st, const char* name) {
    if (!st) return NULL;
    Scope* current_scope = symbol_table_get_current_scope(st);
    if (!current_scope) return NULL;

    for (int i = 0; i < current_scope->symbol_count; ++i) {
        if (strcmp(current_scope->symbols[i].name, name) == 0) {
            return &current_scope->symbols[i];
        }
    }
    return NULL; 
}

Symbol* symbol_table_lookup_all_scopes(SymbolTable* st, const char* name) {
    if (!st) return NULL;
    Scope* scope_to_search = symbol_table_get_current_scope(st);
    while (scope_to_search) {
        for (int i = 0; i < scope_to_search->symbol_count; ++i) {
            if (strcmp(scope_to_search->symbols[i].name, name) == 0) {
                return &scope_to_search->symbols[i];
            }
        }
        scope_to_search = scope_to_search->parent_scope; 
    }
    return NULL; 
}

// --- Stub implementations for missing functions ---
static void analyze_struct_decl(ASTNode *struct_node) {
    if (!struct_node) return;

    /* Register the struct itself in the current scope (normally global) */
    if (!symbol_table_add_symbol(g_st, struct_node->value, SYMBOL_STRUCT, struct_node->value, struct_node)) {
        /* Duplicate definition – error already emitted inside add_symbol */
        return;
    }

    /* Enter struct scope so member fields can be looked up later */
    char scope_name_buf[256];
    snprintf(scope_name_buf, sizeof(scope_name_buf), "struct_%s", struct_node->value);
    symbol_table_enter_scope(g_st, scope_name_buf);

    /* Record each field (only var declarations are allowed inside struct) */
    ASTNode *field = struct_node->left;
    while (field) {
        if (field->type == AST_VAR_DECL || field->type == AST_TYPED_VAR_DECL) {
            const char *field_type = field->data_type[0] ? field->data_type : "any";
            symbol_table_add_symbol(g_st, field->value, SYMBOL_VARIABLE, field_type, field);
        } else {
            /* For now ignore methods or nested types inside structs */
            analyze_node(field);
        }
        field = field->next;
    }

    symbol_table_exit_scope(g_st);
}

static void analyze_class_decl(ASTNode *class_node) {
    if (!class_node) return;

    // Add the class symbol to the current (likely global) scope
    if (!symbol_table_add_symbol(g_st, class_node->value, SYMBOL_CLASS, class_node->value, class_node)) {
        // Duplicate class definition – error already reported inside add_symbol.
        return;
    }

    // Enter a new scope representing the class body
    char scope_name_buf[256];
    snprintf(scope_name_buf, sizeof(scope_name_buf), "class_%s", class_node->value);
    symbol_table_enter_scope(g_st, scope_name_buf);

    // Iterate over class members
    ASTNode *member = class_node->left;
    while (member) {
        if (member->type == AST_VAR_DECL || member->type == AST_TYPED_VAR_DECL) {
            const char *member_type = member->data_type[0] ? member->data_type : "any";
            symbol_table_add_symbol(g_st, member->value, SYMBOL_VARIABLE, member_type, member);
        } else if (member->type == AST_FUNCTION || member->type == AST_TYPED_FUNCTION) {
            analyze_function_decl(member, class_node); // Pass parent class node
        } else {
            // Other member kinds can be handled later (e.g., nested classes/structs, etc.)
            analyze_node(member);
        }
        member = member->next;
    }

    symbol_table_exit_scope(g_st);
}

static void analyze_function_decl(ASTNode *func_node, ASTNode *parent_class_node_or_null) {
    if (!func_node) return;

    const char* func_name = func_node->value;
    if (!func_name || strlen(func_name) == 0) {
        fprintf(stderr, "Error: Function declaration has no name\n");
        return;
    }

    if (func_node->type == AST_CLASS_METHOD) {
        if (!parent_class_node_or_null) {
            fprintf(stderr, "Error: Class method '%s' declared outside of class\n", func_name);
            return;
        }
        ASTNode* this_param = create_node(AST_PARAMETER, "this", func_node->line, func_node->col);
        strncpy(this_param->value, "this", sizeof(this_param->value) - 1);
        this_param->left = create_node(AST_TYPE, parent_class_node_or_null->value, func_node->line, func_node->col);
        // Set access modifier string to "public"
        strncpy(this_param->access_modifier, "public", sizeof(this_param->access_modifier) - 1);
        this_param->access_modifier[sizeof(this_param->access_modifier) - 1] = '\0';

        if (func_node->left) {
            ASTNode* current = func_node->left;
            while (current->next) {
                current = current->next;
            }
            current->next = this_param;
        } else {
            func_node->left = this_param;
        }
    }

    const char* func_return_type = (func_node->type == AST_TYPED_FUNCTION && func_node->data_type[0]) ? func_node->data_type : "any";
    (void)func_return_type; // suppress unused variable warning
    
    char scope_name_buf[256];
    if (parent_class_node_or_null) {
        snprintf(scope_name_buf, sizeof(scope_name_buf), "method_%s.%s", parent_class_node_or_null->value, func_node->value);
    } else {
        snprintf(scope_name_buf, sizeof(scope_name_buf), "function_%s", func_node->value);
    }
    symbol_table_enter_scope(g_st, scope_name_buf);

    // If this is a method (i.e., has a parent class), inject implicit 'this' parameter
    if (parent_class_node_or_null) {
        const char *class_name = parent_class_node_or_null->value;
        symbol_table_add_symbol(g_st, "this", SYMBOL_VARIABLE, class_name, func_node);
    }

    if (func_node->left) { 
        ASTNode *param = func_node->left;
        while (param) {
            if (param->type == AST_PARAMETER) {
                const char* param_type_name = param->data_type[0] ? param->data_type : "any";
                symbol_table_add_symbol(g_st, param->value, SYMBOL_PARAMETER, param_type_name, param);
            }
            param = param->next;
        }
    }
    
    if (func_node->right && func_node->right->type == AST_BLOCK) {
        analyze_block_stmts(func_node->right); 
    }
    symbol_table_exit_scope(g_st);
}

static void analyze_return_stmt(ASTNode *return_node) {
    Scope* func_scope = symbol_table_get_current_scope(g_st);
    Symbol* func_sym = NULL;

    while(func_scope){ // Find the enclosing function's symbol
        if(strncmp(func_scope->scope_name, "method_", strlen("method_")) == 0 || strncmp(func_scope->scope_name, "function_", strlen("function_")) == 0) {
            char func_name_from_scope[128];
            const char* name_start = strchr(func_scope->scope_name, '_') + 1;
            
            // Simplified logic: Just look up the function/method name from its containing scope
            Scope* search_in_scope = func_scope->parent_scope;
            if (search_in_scope) {
                 const char* method_sep = strchr(name_start, '.');
                 if (method_sep) {
                     // This part is complex, for now we rely on finding the method in class scope later
                 } else {
                    strncpy(func_name_from_scope, name_start, sizeof(func_name_from_scope)-1);
                    func_name_from_scope[sizeof(func_name_from_scope)-1] = '\0';
                    // The lookup was incorrect. It should search the global table, not just one scope.
                    func_sym = symbol_table_lookup_all_scopes(g_st, func_name_from_scope);
                 }
            }
            break; 
        }
        if(strcmp(func_scope->scope_name, "global")==0) break;
        func_scope = func_scope->parent_scope;
    }
    
    const char* expected_return_type = func_sym ? func_sym->type_name : "any";

    if (return_node->left) { 
        const char* actual_return_type = analyze_expression_node(return_node->left);
        if (strcmp(expected_return_type, "void") == 0 && strcmp(actual_return_type, "void") != 0 && strcmp(actual_return_type, "any") !=0 && strcmp(actual_return_type, "error_type") != 0 ) {
             fprintf(stderr, "[SEMANTIC L%d:%d] Error: Function declared as void cannot return a value of type '%s'.\n",
                    return_node->line, return_node->col, actual_return_type);
        } else if (strcmp(expected_return_type, "void") != 0 && strcmp(actual_return_type, "void") == 0 ) {
             fprintf(stderr, "[SEMANTIC L%d:%d] Error: Function expects return type '%s' but got void/no value.\n",
                    return_node->line, return_node->col, expected_return_type);
        }
        else if (strcmp(expected_return_type, "any") != 0 && strcmp(actual_return_type, "any") != 0 &&
            strcmp(expected_return_type, actual_return_type) != 0 && strcmp(actual_return_type, "error_type") != 0) {
            fprintf(stderr, "[SEMANTIC L%d:%d] Type Mismatch: Function expects return type %s but got %s.\n",
                    return_node->line, return_node->col, expected_return_type, actual_return_type);
        }
    } else { 
        if (strcmp(expected_return_type, "void") != 0 && strcmp(expected_return_type, "any") != 0) {
            fprintf(stderr, "[SEMANTIC L%d:%d] Error: Function expects return type %s but no value was returned.\n",
                    return_node->line, return_node->col, expected_return_type);
        }
    }
}

static const char* analyze_member_access_expr(ASTNode *access_node) {
    if (!access_node || !access_node->left || !access_node->value[0]) {
        if (access_node) { // Corrected misleading indentation
            strcpy(access_node->data_type, "error_type");
        }
        return "error_type";
    }
    // ... rest of function is unchanged
    const char* target_type_name = analyze_expression_node(access_node->left);
    if (strcmp(target_type_name, "error_type") == 0) {
        strcpy(access_node->data_type, "error_type"); return "error_type";
    }
    if (strcmp(target_type_name, "any") == 0) {
         strcpy(access_node->data_type, "any"); return "any"; 
    }

    Symbol* type_sym = symbol_table_lookup_all_scopes(g_st, target_type_name);
    if (type_sym && (type_sym->kind == SYMBOL_CLASS || type_sym->kind == SYMBOL_STRUCT)) {
        ASTNode* type_decl_node = type_sym->declaration_node;
        if (!type_decl_node) { // Should not happen if symbol table is consistent
             strcpy(access_node->data_type, "error_type"); return "error_type";
        }
        ASTNode* member_decl = type_decl_node->left; 
        int found = 0;
        while(member_decl) {
            // Check for variable members or function members (methods)
            if ((member_decl->type == AST_VAR_DECL || member_decl->type == AST_TYPED_VAR_DECL || 
                 member_decl->type == AST_FUNCTION || member_decl->type == AST_TYPED_FUNCTION) &&
                strcmp(member_decl->value, access_node->value) == 0) {
                
                Scope* current_analyzer_scope = symbol_table_get_current_scope(g_st);
                char current_class_context_name[128] = "";

                Scope* temp_scope = current_analyzer_scope;
                while(temp_scope) {
                    if(strncmp(temp_scope->scope_name, "class_", strlen("class_"))==0) {
                        strncpy(current_class_context_name, temp_scope->scope_name + strlen("class_"), sizeof(current_class_context_name)-1);
                        break;
                    } else if (strncmp(temp_scope->scope_name, "method_", strlen("method_"))==0) {
                         const char* name_ptr = strchr(temp_scope->scope_name, '_') + 1;
                         const char* dot_ptr = strchr(name_ptr, '.');
                         if(dot_ptr) strncpy(current_class_context_name, name_ptr, dot_ptr - name_ptr);
                         break;
                    }
                    if(strcmp(temp_scope->scope_name, "global")==0) break;
                    temp_scope = temp_scope->parent_scope;
                }

                if(member_decl->access_modifier[0] && strcmp(member_decl->access_modifier, "private") == 0) {
                    if(strcmp(target_type_name, current_class_context_name) != 0) {
                        fprintf(stderr, "[SEMANTIC L%d:%d] Error: Member '%s' of type '%s' is private and cannot be accessed from context '%s'.\n", 
                                 access_node->line, access_node->col, access_node->value, target_type_name, current_class_context_name[0] ? current_class_context_name : "global");
                        strcpy(access_node->data_type, "error_type"); return "error_type";
                    }
                }
                int is_static_access_attempt = (access_node->left->type == AST_IDENTIFIER && type_sym && strcmp(access_node->left->value, type_sym->name)==0);
                int member_is_static = (member_decl->access_modifier[0] && strcmp(member_decl->access_modifier, "static")==0);

                if(is_static_access_attempt && !member_is_static) {
                     fprintf(stderr, "[SEMANTIC L%d:%d] Error: Cannot access instance member '%s' of type '%s' statically.\n", 
                                 access_node->line, access_node->col, access_node->value, target_type_name);
                     strcpy(access_node->data_type, "error_type"); return "error_type";
                }

                if(member_decl->data_type[0]) { 
                    strncpy(access_node->data_type, member_decl->data_type, sizeof(access_node->data_type)-1);
                } else if (member_decl->type == AST_VAR_DECL || member_decl->type == AST_FUNCTION) { 
                     strcpy(access_node->data_type, "any"); 
                }
                found = 1;
                break;
            }
            member_decl = member_decl->next;
        }
        if (!found) {
             /* Dynamic property: allow, assume type 'any' */
             strcpy(access_node->data_type, "any");
        }
    } else if ( (strcmp(target_type_name, "string")==0 || strstr(target_type_name, "[]") || strcmp(target_type_name, "array")==0 ) &&
                strcmp(access_node->value, "length")==0) {
        strcpy(access_node->data_type, "int");
    } else {
        fprintf(stderr, "[SEMANTIC L%d:%d] Error: Cannot access member '%s' on primitive or unknown type '%s'.\n", 
                access_node->line, access_node->col, access_node->value, target_type_name);
        strcpy(access_node->data_type, "error_type");
    }
    return access_node->data_type[0] ? access_node->data_type : "error_type";
}

static void analyze_new_expr(ASTNode *new_node) {
    Symbol* class_sym = symbol_table_lookup_all_scopes(g_st, new_node->value);
    if (!class_sym || (class_sym->kind != SYMBOL_CLASS && class_sym->kind != SYMBOL_STRUCT)) {
        fprintf(stderr, "[SEMANTIC L%d:%d] Error: Class or struct '%s' not found for 'new' expression.\n", 
                new_node->line, new_node->col, new_node->value);
        strncpy(new_node->data_type, "error_type", sizeof(new_node->data_type)-1);
        new_node->data_type[sizeof(new_node->data_type)-1] = '\0';
        return;
    }
    strncpy(new_node->data_type, new_node->value, sizeof(new_node->data_type)-1);
    new_node->data_type[sizeof(new_node->data_type)-1] = '\0';

    if (new_node->left) { 
        ASTNode *arg = new_node->left;
        while (arg) {
            analyze_expression_node(arg);
            arg = arg->next;
        }
        // TODO: Find constructor for class_sym and check args.
    }
}


static void analyze_node(ASTNode *node) {
    if (!node) return;
    
    switch (node->type) {
        case AST_PROGRAM:
            {
                ASTNode *child = node->left; 
                while (child) {
                    analyze_node(child); 
                    child = child->next;
                }
            }
            break;
        case AST_FUNCTION: 
        case AST_TYPED_FUNCTION:
            if (g_st && g_st->current_scope_idx == 0 && strcmp(g_st->scope_stack[0]->scope_name, "global")==0) {
                analyze_function_decl(node, NULL); 
            } else {
                 // This implies method declaration, which should be handled by analyze_class_decl calling analyze_function_decl
                 // If not in class scope, it's an error (nested function not in class)
                 Scope* current_scope = symbol_table_get_current_scope(g_st);
                 if (!(current_scope && strncmp(current_scope->scope_name, "class_", strlen("class_")) == 0)) {
                     fprintf(stderr, "[SEMANTIC L%d:%d] Error: Function '%s' declared in unexpected scope '%s'. Functions can only be global or class methods.\n",
                             node->line, node->col, node->value, 
                             current_scope ? current_scope->scope_name : "unknown");
                 }
                 // If it *is* in class scope, analyze_class_decl would have called analyze_function_decl(node, class_node)
            }
            break;
        case AST_BLOCK: analyze_block_stmts(node); break;
        case AST_VAR_DECL: 
        case AST_TYPED_VAR_DECL: analyze_var_decl_stmt(node); break;
        case AST_ASSIGN: analyze_assignment_stmt(node); break;
        case AST_RETURN: analyze_return_stmt(node); break;
        case AST_IF: analyze_if_stmt(node); break;
        case AST_WHILE: analyze_while_stmt(node); break;
        case AST_FOR: analyze_for_stmt(node); break;
        case AST_CALL: analyze_call_expr_or_stmt(node); break;
        case AST_STRUCT: analyze_struct_decl(node); break;
        case AST_CLASS: analyze_class_decl(node); break;
        case AST_PRINT:
            if(node->left) analyze_expression_node(node->left);
            else fprintf(stderr, "[SEMANTIC L%d:%d] Error: Print statement missing expression.\n", node->line, node->col);
            break;
        case AST_IMPORT: /* TODO */ break;
        case AST_LITERAL: case AST_IDENTIFIER: case AST_BINARY_OP: case AST_UNARY_OP:
        case AST_ARRAY:   case AST_MEMBER_ACCESS: case AST_NEW:     case AST_THIS:
        case AST_INDEX_ACCESS:
            analyze_expression_node(node);
            break;
        case AST_ELSE: break; 
        default: break;
    }
}

void analyze_program(ASTNode *program_ast_root) {
    if (!program_ast_root) {
        fprintf(stderr, "[SEMANTIC] Error: NULL AST provided for analysis.\n");
        return;
    }
    if (program_ast_root->type != AST_PROGRAM) {
        fprintf(stderr, "[SEMANTIC] Error: Expected AST_PROGRAM node at root, got %s.\n", node_type_to_string(program_ast_root->type));
        return;
    }
    
    printf("\n==== Semantic Analysis ====\n");
    if (g_st) symbol_table_destroy(g_st); 
    g_st = symbol_table_create();
    
    /* First pass: predeclare global functions so they can be called before their textual definition */
    if (program_ast_root->left) {
        ASTNode *child = program_ast_root->left;
        while (child) {
            if (child->type == AST_FUNCTION || child->type == AST_TYPED_FUNCTION) {
                const char *return_type = (child->type == AST_TYPED_FUNCTION && child->data_type[0]) ? child->data_type : "any";
                /* avoid duplicate error if same function name already predeclared */
                if (!symbol_table_lookup_current_scope(g_st, child->value)) {
                    symbol_table_add_symbol(g_st, child->value, SYMBOL_FUNCTION, return_type, child);
                }
            }
            child = child->next;
        }
    }

    /* Second pass: full semantic analysis */
    analyze_node(program_ast_root);
    
    symbol_table_destroy(g_st);
    g_st = NULL;
    printf("[SEMANTIC] Semantic analysis pass complete.\n");
}

void check_semantics(ASTNode *program_ast_root) {
    if (!program_ast_root) return;
    // This is now largely integrated into the main analyze_program pass.
    // Could be used for multi-pass analysis or more complex checks later.
    // printf("[SEMANTIC CHECKS] Detailed semantic checks (currently placeholder).\n");
}

static const char* analyze_expression_node(ASTNode *expr_node) {
    if (!expr_node) return "error_type";
    
    switch (expr_node->type) {
        case AST_LITERAL:
            if (expr_node->value[0] == '"') {
                strcpy(expr_node->data_type, "string");
            } else if (isdigit(expr_node->value[0]) || (expr_node->value[0] == '-' && isdigit(expr_node->value[1]))) {
                if (strchr(expr_node->value, '.')) {
                    strcpy(expr_node->data_type, "float");
                } else {
                    strcpy(expr_node->data_type, "int");
                }
            } else if (strcmp(expr_node->value, "true") == 0 || strcmp(expr_node->value, "false") == 0) {
                strcpy(expr_node->data_type, "bool");
            }
            return expr_node->data_type[0] ? expr_node->data_type : "any";
            
        case AST_IDENTIFIER: {
            Symbol* sym = symbol_table_lookup_all_scopes(g_st, expr_node->value);
            if (sym) {
                if (sym->type_name[0]) {
                    strcpy(expr_node->data_type, sym->type_name);
                } else {
                    strcpy(expr_node->data_type, "any");
                }
            } else {
                // fprintf(stderr, "[SEMANTIC L%d:%d] Warning: Undefined variable '%s'.\n", 
                //         expr_node->line, expr_node->col, expr_node->value);
                strcpy(expr_node->data_type, "error_type");
            }
            return expr_node->data_type[0] ? expr_node->data_type : "any";
        }
        
        case AST_BINARY_OP:
            // Simplified binary operation type checking
            {
                const char* left_type = analyze_expression_node(expr_node->left);
                const char* right_type = analyze_expression_node(expr_node->right);
                
                if (strcmp(left_type, "error_type") == 0 || strcmp(right_type, "error_type") == 0) {
                    strcpy(expr_node->data_type, "error_type");
                    return "error_type";
                }
                
                // Arithmetic operators
                if (strcmp(expr_node->value, "+") == 0 || 
                    strcmp(expr_node->value, "-") == 0 ||
                    strcmp(expr_node->value, "*") == 0 ||
                    strcmp(expr_node->value, "/") == 0 ||
                    strcmp(expr_node->value, "%") == 0) {
                    
                    if (strcmp(left_type, "string") == 0 && strcmp(expr_node->value, "+") == 0) {
                        strcpy(expr_node->data_type, "string");
                    } else if ((strcmp(left_type, "int") == 0 || strcmp(left_type, "float") == 0) &&
                               (strcmp(right_type, "int") == 0 || strcmp(right_type, "float") == 0)) {
                        if (strcmp(left_type, "float") == 0 || strcmp(right_type, "float") == 0) {
                            strcpy(expr_node->data_type, "float");
                        } else {
                            strcpy(expr_node->data_type, "int");
                        }
                    } else {
                        strcpy(expr_node->data_type, "error_type");
                    }
                }
                // Comparison operators
                else if (strcmp(expr_node->value, "==") == 0 || 
                         strcmp(expr_node->value, "!=") == 0 ||
                         strcmp(expr_node->value, "<") == 0 ||
                         strcmp(expr_node->value, ">") == 0 ||
                         strcmp(expr_node->value, "<=") == 0 ||
                         strcmp(expr_node->value, ">=") == 0) {
                    
                    strcpy(expr_node->data_type, "bool");
                }
                // Logical operators
                else if (strcmp(expr_node->value, "&&") == 0 || 
                         strcmp(expr_node->value, "||") == 0) {
                    
                    strcpy(expr_node->data_type, "bool");
                } else {
                    strcpy(expr_node->data_type, "any");
                }
                
                return expr_node->data_type[0] ? expr_node->data_type : "any";
            }
        
        case AST_UNARY_OP:
            {
                const char* operand_type = analyze_expression_node(expr_node->left);
                
                if (strcmp(expr_node->value, "!") == 0) {
                    strcpy(expr_node->data_type, "bool");
                } else if (strcmp(expr_node->value, "-") == 0 || strcmp(expr_node->value, "+") == 0) {
                    if (strcmp(operand_type, "int") == 0 || strcmp(operand_type, "float") == 0) {
                        strcpy(expr_node->data_type, operand_type);
                    } else {
                        strcpy(expr_node->data_type, "error_type");
                    }
                } else {
                    strcpy(expr_node->data_type, "any");
                }
                
                return expr_node->data_type[0] ? expr_node->data_type : "any";
            }
            
        case AST_CALL:
            {
                // Simply mark as "any" type for now
                strcpy(expr_node->data_type, "any");
                return "any";
            }
            
        case AST_MEMBER_ACCESS:
            return analyze_member_access_expr(expr_node);
            
        case AST_NEW:
            analyze_new_expr(expr_node);
            return expr_node->data_type[0] ? expr_node->data_type : "any";
            
        default:
            strcpy(expr_node->data_type, "any");
            return "any";
    }
}

// Stub implementations for missing functions
static void analyze_block_stmts(ASTNode *block_node) {
    if (!block_node) return;
    
    // Enter a new lexical scope for the block
    char scope_name[128];
    snprintf(scope_name, sizeof(scope_name), "block_L%d", block_node->line);
    symbol_table_enter_scope(g_st, scope_name);
    
    ASTNode *stmt = block_node->left;
    while (stmt) {
        analyze_node(stmt);
        stmt = stmt->next;
    }
    
    symbol_table_exit_scope(g_st);
}

static void analyze_var_decl_stmt(ASTNode *decl_node) {
    if (!decl_node) return;
    
    const char* var_type = decl_node->data_type[0] ? decl_node->data_type : "any";
    symbol_table_add_symbol(g_st, decl_node->value, SYMBOL_VARIABLE, var_type, decl_node);
    
    if (decl_node->left) {
        const char* init_expr_type = analyze_expression_node(decl_node->left);
        if (strcmp(var_type, "any") != 0 && strcmp(init_expr_type, "any") != 0 && 
            strcmp(init_expr_type, "error_type") != 0 && strcmp(var_type, init_expr_type) != 0) {
            fprintf(stderr, "[SEMANTIC L%d:%d] Type Mismatch: Cannot initialize variable '%s' of type '%s' with a value of type '%s'.\n",
                   decl_node->line, decl_node->col, decl_node->value, var_type, init_expr_type);
        }
    }
}

static void analyze_assignment_stmt(ASTNode *assign_node) {
    if (!assign_node || !assign_node->left) return;
    
    // First check that we're assigning to a valid l-value
    ASTNode* lhs = assign_node->left;
    const char* lhs_type = "error_type";
    
    if (lhs->type == AST_IDENTIFIER) {
        Symbol* sym = symbol_table_lookup_all_scopes(g_st, lhs->value);
        if (!sym) {
            fprintf(stderr, "[SEMANTIC L%d:%d] Error: Assignment to undeclared variable '%s'.\n",
                   lhs->line, lhs->col, lhs->value);
            return;
        }
        lhs_type = sym->type_name;
        if (sym->declaration_node && strcmp(sym->declaration_node->access_modifier, "const") == 0) {
            fprintf(stderr, "[SEMANTIC L%d:%d] Error: Cannot assign to constant variable '%s'.\n",
                    lhs->line, lhs->col, lhs->value);
            return;
        }
    } 
    else if (lhs->type == AST_MEMBER_ACCESS) {
        lhs_type = analyze_member_access_expr(lhs);
    }
    else {
        fprintf(stderr, "[SEMANTIC L%d:%d] Error: Invalid assignment target.\n", lhs->line, lhs->col);
        return;
    }
    
    if (assign_node->right) {
        const char* rhs_type = analyze_expression_node(assign_node->right);
        if (strcmp(lhs_type, "any") != 0 && strcmp(rhs_type, "any") != 0 && 
            strcmp(rhs_type, "error_type") != 0 && strcmp(lhs_type, rhs_type) != 0) {
            fprintf(stderr, "[SEMANTIC L%d:%d] Type Mismatch: Cannot assign value of type '%s' to variable of type '%s'.\n",
                   assign_node->line, assign_node->col, rhs_type, lhs_type);
        }
    }
}

static void analyze_if_stmt(ASTNode *if_node) {
    if (!if_node) return;
    
    if (if_node->left) {
        const char* cond_type = analyze_expression_node(if_node->left);
        if (strcmp(cond_type, "bool") != 0 && strcmp(cond_type, "any") != 0 && strcmp(cond_type, "error_type") != 0) {
            fprintf(stderr, "[SEMANTIC L%d:%d] Error: If condition must be a boolean expression, got '%s'.\n",
                   if_node->line, if_node->col, cond_type);
        }
    }
    
    if (if_node->right) {
        ASTNode* then_else = if_node->right;
        if (then_else->left && then_else->left->type == AST_BLOCK) {
            analyze_block_stmts(then_else->left);
        }
        if (then_else->right && then_else->right->type == AST_BLOCK) {
            analyze_block_stmts(then_else->right);
        }
    }
}

static void analyze_while_stmt(ASTNode *while_node) {
    if (!while_node) return;
    
    if (while_node->left) {
        const char* cond_type = analyze_expression_node(while_node->left);
        if (strcmp(cond_type, "bool") != 0 && strcmp(cond_type, "any") != 0 && strcmp(cond_type, "error_type") != 0) {
            fprintf(stderr, "[SEMANTIC L%d:%d] Error: While condition must be a boolean expression, got '%s'.\n",
                   while_node->line, while_node->col, cond_type);
        }
    }
    
    if (while_node->right && while_node->right->type == AST_BLOCK) {
        analyze_block_stmts(while_node->right);
    }
}

static void analyze_for_stmt(ASTNode *for_node) {
    if (!for_node) return;
    
    char scope_name[128];
    snprintf(scope_name, sizeof(scope_name), "for_loop_L%d", for_node->line);
    symbol_table_enter_scope(g_st, scope_name);
    
    // For simplicity, just analyze each part of the for loop
    if (for_node->left) {
        ASTNode* for_parts = for_node->left;
        
        // Init
        if (for_parts->left) {
            analyze_node(for_parts->left);
        }
        
        // Condition
        if (for_parts->right && for_parts->right->left) {
            const char* cond_type = analyze_expression_node(for_parts->right->left);
            if (strcmp(cond_type, "bool") != 0 && strcmp(cond_type, "any") != 0 && strcmp(cond_type, "error_type") != 0) {
                fprintf(stderr, "[SEMANTIC L%d:%d] Error: For loop condition must be a boolean expression, got '%s'.\n",
                       for_node->line, for_node->col, cond_type);
            }
            
            // Update
            if (for_parts->right->right) {
                analyze_node(for_parts->right->right);
            }
        }
    }
    
    // Body
    if (for_node->right && for_node->right->type == AST_BLOCK) {
        analyze_block_stmts(for_node->right);
    }
    
    symbol_table_exit_scope(g_st);
}

static void analyze_call_expr_or_stmt(ASTNode *call_node) {
    if (!call_node) return;
    
    Symbol* func_sym = symbol_table_lookup_all_scopes(g_st, call_node->value);
    if (!func_sym) {
        fprintf(stderr, "[SEMANTIC L%d:%d] Error: Call to undefined function '%s'.\n",
               call_node->line, call_node->col, call_node->value);
        strcpy(call_node->data_type, "error_type");
        return;
    }
    
    if (func_sym->kind != SYMBOL_FUNCTION) {
        fprintf(stderr, "[SEMANTIC L%d:%d] Error: '%s' is not a function.\n",
               call_node->line, call_node->col, call_node->value);
        strcpy(call_node->data_type, "error_type");
        return;
    }
    
    strcpy(call_node->data_type, func_sym->type_name);
    
    // For now, just analyze the arguments without parameter matching
    ASTNode* arg = call_node->left;
    while (arg) {
        analyze_expression_node(arg);
        arg = arg->next;
    }
}
