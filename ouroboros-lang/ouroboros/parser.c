#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"
#include "lexer.h"
#include "ast_types.h"

// --- Forward Declarations ---
static ASTNode* parse_statement();
static ASTNode* parse_expression();
static ASTNode* parse_primary();
static ASTNode* parse_block();
static ASTNode* parse_variable_declaration();
static ASTNode* parse_typed_variable_declaration();
static ASTNode* parse_if_statement();
static ASTNode* parse_while_statement();
static ASTNode* parse_for_statement();
static ASTNode* parse_return_statement();
static ASTNode* parse_function();
static ASTNode* parse_typed_function();
static ASTNode* parse_parameters();
static ASTNode* parse_print_statement();
static ASTNode* parse_class_declaration();
static ASTNode* parse_struct_declaration();
static ASTNode* parse_binary_expression(ASTNode* left, int min_precedence);
static ASTNode* parse_literal_or_identifier();
static ASTNode* parse_array_literal();
static ASTNode* parse_new_expression();
static ASTNode* parse_member_access(ASTNode* target);
static ASTNode* parse_this_reference();
static ASTNode* parse_import();
static ASTNode* parse_break_statement();
static ASTNode* parse_continue_statement();
static ASTNode* parse_super_reference();
static ASTNode* parse_map_literal();
static ASTNode* parse_anonymous_function();


// --- Globals ---
static Token* tokens;
static int token_pos;
static int num_tokens;
static Token current_token;

ASTNode* program = NULL;

// --- Helpers ---
static void advance() {
    if (token_pos < num_tokens) {
        current_token = tokens[token_pos++];
    }
}

static Token peek_token() {
    if (token_pos < num_tokens) {
        return tokens[token_pos];
    }
    Token eof_token = { .type = TOKEN_EOF, .text = "", .line = current_token.line, .col = current_token.col };
    return eof_token;
}

static Token peek_token_n(int n) {
    if (token_pos + n - 1 < num_tokens) {
        return tokens[token_pos + n - 1];
    }
    Token eof_token = { .type = TOKEN_EOF, .text = "", .line = current_token.line, .col = current_token.col };
    return eof_token;
}

// Utility to check if a string is a built-in type keyword
int is_builtin_type_keyword(const char* s) { // Renamed to avoid conflict if parser.c included elsewhere
    return strcmp(s, "int") == 0 || strcmp(s, "float") == 0 ||
        strcmp(s, "bool") == 0 || strcmp(s, "string") == 0 ||
        strcmp(s, "void") == 0 || strcmp(s, "array") == 0 || // "array" might be a type
        strcmp(s, "object") == 0 || strcmp(s, "any") == 0 ||
        strcmp(s, "long") == 0 || strcmp(s, "double") == 0 || // added extra primitive types
        strcmp(s, "map") == 0 || strcmp(s, "char") == 0; // added map and char types
}


static int get_precedence(const char* op) {
    if (strcmp(op, "=") == 0 || strcmp(op, "+=") == 0 || strcmp(op, "-=") == 0 || strcmp(op, "*=") == 0 || strcmp(op, "/=") == 0 || strcmp(op, "%=") == 0) return 1; // Assignment family (right-associative)
    if (strcmp(op, "||") == 0) return 2;
    if (strcmp(op, "&&") == 0) return 3;
    // Bitwise ops could go here if added
    if (strcmp(op, "==") == 0 || strcmp(op, "!=") == 0) return 7;
    if (strcmp(op, "<") == 0 || strcmp(op, "<=") == 0 || strcmp(op, ">") == 0 || strcmp(op, ">=") == 0) return 8;
    // Bitwise shifts
    if (strcmp(op, "<<<") == 0) return 9; // Though '<<<' not defined, keep order if added
    if (strcmp(op, "<<") == 0 || strcmp(op, ">>") == 0 || strcmp(op, ">>>") == 0) return 9;
    if (strcmp(op, "+") == 0 || strcmp(op, "-") == 0) return 10; // Additive
    if (strcmp(op, "*") == 0 || strcmp(op, "/") == 0 || strcmp(op, "%") == 0) return 11; // Multiplicative
    // Unary operators are handled by parse_primary or a dedicated unary parse function
    // Member access (.), array index ([]), function call (()) are usually highest or handled by parse_primary loop
    return 0;
}


// --- Main Parsing Function ---
ASTNode* parse(Token* token_array) {
    tokens = token_array;
    token_pos = 0;
    num_tokens = 0;
    while (token_array[num_tokens].type != TOKEN_EOF) {
        num_tokens++;
    }
    num_tokens++; // for EOF

    advance();

    program = create_node(AST_PROGRAM, "program", 1, 1);
    ASTNode* last_stmt = NULL;

    // printf("\n==== Parsing ====\n");
    while (current_token.type != TOKEN_EOF) {
        ASTNode* stmt = parse_statement();
        if (stmt) {
            if (last_stmt == NULL) {
                program->left = stmt;
                last_stmt = stmt;
            }
            else {
                last_stmt->next = stmt;
                last_stmt = stmt;
            }
        }
        else {
            fprintf(stderr, "Error: Failed to parse statement at line %d, col %d. Current token: '%s' (Type %d). Skipping.\n",
                current_token.line, current_token.col, current_token.text, current_token.type);
            if (current_token.type != TOKEN_EOF) advance(); else break;
        }
    }
    return program;
}

// --- Statement Parsers ---

static ASTNode* parse_statement() {
    char modifiers[32] = ""; // Buffer to hold combined modifiers like "public static"
    Token first_modifier_token = {0};

    // **FIX 1: Loop to consume a sequence of modifiers.**
    while (current_token.type == TOKEN_KEYWORD &&
           (strcmp(current_token.text, "public") == 0 ||
            strcmp(current_token.text, "private") == 0 ||
            strcmp(current_token.text, "static") == 0 ||
            strcmp(current_token.text, "constructor") == 0)) {

        if (modifiers[0] == '\0') {
            first_modifier_token = current_token;
        } else {
            strcat(modifiers, " ");
        }
        strcat(modifiers, current_token.text);
        advance();
    }

    ASTNode* stmt = NULL;

    // --- Dispatch based on the token *after* any modifiers ---
    if (current_token.type == TOKEN_KEYWORD) {
        if (strcmp(current_token.text, "let") == 0 || strcmp(current_token.text, "var") == 0 || strcmp(current_token.text, "const") == 0) {
            stmt = parse_variable_declaration();
        } else if (strcmp(current_token.text, "if") == 0) {
            stmt = parse_if_statement();
        } else if (strcmp(current_token.text, "while") == 0) {
            stmt = parse_while_statement();
        } else if (strcmp(current_token.text, "for") == 0) {
            stmt = parse_for_statement();
        } else if (strcmp(current_token.text, "return") == 0) {
            stmt = parse_return_statement();
        } else if (strcmp(current_token.text, "function") == 0 || strcmp(current_token.text, "func") == 0 || strcmp(current_token.text, "fn") == 0) {
            stmt = parse_function();
            // Apply constructor modifier if present - just mark it as a class method
            if (modifiers[0] != '\0' && strstr(modifiers, "constructor")) {
                // Mark this function as a constructor
                if (stmt && strcmp(stmt->value, "new") == 0) {
                    stmt->type = AST_CLASS_METHOD;
                }
            }
        } else if (strcmp(current_token.text, "print") == 0) {
            stmt = parse_print_statement();
        } else if (strcmp(current_token.text, "class") == 0) {
            stmt = parse_class_declaration();
        } else if (strcmp(current_token.text, "struct") == 0) {
            stmt = parse_struct_declaration();
        } else if (strcmp(current_token.text, "import") == 0) {
            stmt = parse_import();
        } else if (is_builtin_type_keyword(current_token.text)) {
            Token peek = peek_token();
            // Case 1: Standard typed declaration 'int x' or typed function 'int func('
            if (peek.type == TOKEN_IDENTIFIER) {
                Token peek2 = peek_token_n(2);
                if (peek2.type == TOKEN_SYMBOL && strcmp(peek2.text, "(") == 0) {
                    stmt = parse_typed_function();
                } else {
                    stmt = parse_typed_variable_declaration();
                }
            }
            // Case 2: Array type: built-in type followed by '[' (e.g., 'int[] numbers')
            else if (peek.type == TOKEN_SYMBOL && strcmp(peek.text, "[") == 0) {
                stmt = parse_typed_variable_declaration();
            }
        } else if (strcmp(current_token.text, "break") == 0) {
            stmt = parse_break_statement();
        } else if (strcmp(current_token.text, "continue") == 0) {
            stmt = parse_continue_statement();
        }
    } else if (current_token.type == TOKEN_IDENTIFIER) {
        Token peek = peek_token();
        if (peek.type == TOKEN_IDENTIFIER) { // MyType myVar;
            stmt = parse_typed_variable_declaration();
        }
        else if (peek.type == TOKEN_SYMBOL && strcmp(peek.text, "[") == 0) { // MyType[] ...
            stmt = parse_typed_variable_declaration();
        }
        else if (peek.type == TOKEN_SYMBOL && strcmp(peek.text, ":") == 0) { // myVar: MyType
            stmt = parse_typed_variable_declaration();
        }
    }

    // Apply modifiers if they were found
    if (stmt && modifiers[0] != '\0') {
        // HACK: The ASTNode only supports one modifier. Prioritize 'static'.
        if (strstr(modifiers, "static")) {
            strncpy(stmt->access_modifier, "static", sizeof(stmt->access_modifier) - 1);
        } else if (strstr(modifiers, "private")) {
            strncpy(stmt->access_modifier, "private", sizeof(stmt->access_modifier) - 1);
        } else {
            strncpy(stmt->access_modifier, "public", sizeof(stmt->access_modifier) - 1);
        }
        stmt->access_modifier[sizeof(stmt->access_modifier) - 1] = '\0';
        stmt->line = first_modifier_token.line;
        stmt->col = first_modifier_token.col;
    } else if (!stmt && modifiers[0] != '\0') {
        // Modifiers were present but did not precede a declaration; treat them as no-ops and
        // continue parsing the upcoming expression/statement. This supports patterns like
        // 'private this.prop = 1;' or 'static obj.count = 0;'.
    }

    // If no statement was parsed yet, check if we have an identifier with colon (could be a field declaration with modifiers)
    if (!stmt && current_token.type == TOKEN_IDENTIFIER) {
        Token peek = peek_token();
        if (peek.type == TOKEN_SYMBOL && strcmp(peek.text, ":") == 0) {
            // This is a colon-style type annotation, possibly with modifiers
            stmt = parse_typed_variable_declaration();
        }
    }

    // If no statement was parsed yet, it must be an expression statement
    if (!stmt) {
        stmt = parse_expression();
        if (!stmt) return NULL;

        if (current_token.type == TOKEN_SYMBOL && strcmp(current_token.text, ";") == 0) {
            advance();
            return stmt;
        }

        fprintf(stderr, "Error (L%d:%d): Expected ';' after expression statement. Got token '%s' (type %d) after expression starting L%d:%d.\n",
            current_token.line, current_token.col, current_token.text, current_token.type, stmt->line, stmt->col);
        return NULL; // No semicolon
    }

    return stmt;
}

static ASTNode* parse_block() {
    Token start_token = current_token;
    ASTNode* block = create_node(AST_BLOCK, "block", start_token.line, start_token.col);
    ASTNode* last_stmt = NULL;

    while (current_token.type != TOKEN_EOF && !(current_token.type == TOKEN_SYMBOL && strcmp(current_token.text, "}") == 0)) {
        ASTNode* stmt = parse_statement();
        if (stmt) {
            if (last_stmt == NULL) {
                block->left = stmt;
                last_stmt = stmt;
            }
            else {
                last_stmt->next = stmt;
                last_stmt = stmt;
            }
        }
        else {
            fprintf(stderr, "Error in block (L%d:%d): Failed to parse statement. Skipping token: '%s'\n",
                current_token.line, current_token.col, current_token.text);
            if (current_token.type != TOKEN_EOF) advance(); else break;
        }
    }
    return block;
}


static ASTNode* parse_typed_variable_declaration() {
    Token start_token = current_token;
    
    // Check if this is colon-style type annotation (name: type)
    if (current_token.type == TOKEN_IDENTIFIER) {
        Token name_token = current_token;
        Token next = peek_token();
        if (next.type == TOKEN_SYMBOL && strcmp(next.text, ":") == 0) {
            // This is colon-style: name: type
            char var_name_str[sizeof(((ASTNode*)0)->value)];
            strncpy(var_name_str, name_token.text, sizeof(var_name_str) - 1);
            var_name_str[sizeof(var_name_str) - 1] = '\0';
            advance(); // consume name
            advance(); // consume ':'
            
            if (!is_builtin_type_keyword(current_token.text) && 
                current_token.type != TOKEN_IDENTIFIER &&
                current_token.type != TOKEN_KEYWORD) {
                fprintf(stderr, "Error (L%d:%d): Expected type name after ':' in variable declaration.\n", current_token.line, current_token.col);
                return NULL;
            }
            
            char* type_str = strdup(current_token.text);
            advance();
            
            // Check for generic type syntax like array<int> or map<string, any>
            int array_dims = 0;
            if ((current_token.type == TOKEN_SYMBOL || current_token.type == TOKEN_OPERATOR) && strcmp(current_token.text, "<") == 0) {
                // Handle generic types
                char generic_type[256];
                snprintf(generic_type, sizeof(generic_type), "%s<", type_str);
                advance(); // consume '<'
                
                // Parse inner type(s)
                int first = 1;
                while (!((current_token.type == TOKEN_SYMBOL || current_token.type == TOKEN_OPERATOR) && strcmp(current_token.text, ">") == 0)) {
                    if (!first) {
                        strcat(generic_type, ", ");
                    }
                    first = 0;
                    
                    if (is_builtin_type_keyword(current_token.text) || 
                        current_token.type == TOKEN_IDENTIFIER ||
                        current_token.type == TOKEN_KEYWORD) {
                        strcat(generic_type, current_token.text);
                        advance();
                    } else if (current_token.type == TOKEN_SYMBOL && strcmp(current_token.text, ",") == 0) {
                        advance();
                    } else {
                        fprintf(stderr, "Error (L%d:%d): Invalid token in generic type specification.\n", current_token.line, current_token.col);
                        free(type_str);
                        return NULL;
                    }
                }
                strcat(generic_type, ">");
                advance(); // consume '>'
                
                free(type_str);
                type_str = strdup(generic_type);
            }
            
            // Check for array brackets
            while (current_token.type == TOKEN_SYMBOL && strcmp(current_token.text, "[") == 0) {
                advance(); // eat '['
                if (current_token.type != TOKEN_SYMBOL || strcmp(current_token.text, "]") != 0) {
                    fprintf(stderr, "Error (L%d:%d): Expected ']' after '[' in array type declaration.\n", current_token.line, current_token.col);
                    free(type_str);
                    return NULL;
                }
                advance(); // eat ']'
                array_dims++;
            }
            
            ASTNode* var_decl = create_node(AST_TYPED_VAR_DECL, var_name_str, start_token.line, start_token.col);
            strncpy(var_decl->data_type, type_str, sizeof(var_decl->data_type) - 1);
            var_decl->data_type[sizeof(var_decl->data_type) - 1] = '\0';
            for(int i = 0; i < array_dims; i++) strcat(var_decl->data_type, "[]");
            var_decl->is_array = array_dims > 0;
            free(type_str);
            
            var_decl->left = NULL;
            if ((current_token.type == TOKEN_SYMBOL || current_token.type == TOKEN_OPERATOR) && strcmp(current_token.text, "=") == 0) {
                advance();
                var_decl->right = parse_expression();
                if (!var_decl->right) {
                    fprintf(stderr, "Error (L%d:%d): Expected expression after '='\n", current_token.line, current_token.col);
                    free_ast(var_decl);
                    return NULL;
                }
            } else {
                var_decl->right = NULL;
            }
            
            if (current_token.type != TOKEN_SYMBOL || strcmp(current_token.text, ";") != 0) {
                fprintf(stderr, "Error (L%d:%d): Expected ';' after variable declaration of '%s'\n", current_token.line, current_token.col, var_name_str);
                free_ast(var_decl);
                return NULL;
            }
            advance();
            return var_decl;
        }
    }
    
    // Traditional style: type name
    Token type_token = current_token;

    if (!is_builtin_type_keyword(current_token.text) && current_token.type != TOKEN_IDENTIFIER) {
        fprintf(stderr, "Error (L%d:%d): Expected type name for variable declaration.\n", current_token.line, current_token.col);
        return NULL;
    }
    char* type_str = strdup(current_token.text);
    advance();

    // Check for generic type syntax like map<string, any>
    if ((current_token.type == TOKEN_SYMBOL || current_token.type == TOKEN_OPERATOR) && strcmp(current_token.text, "<") == 0) {
        // Handle generic types
        char generic_type[256];
        snprintf(generic_type, sizeof(generic_type), "%s<", type_str);
        advance(); // consume '<'
        
        // Parse inner type(s)
        int first = 1;
        while (!((current_token.type == TOKEN_SYMBOL || current_token.type == TOKEN_OPERATOR) && strcmp(current_token.text, ">") == 0)) {
            if (!first) {
                strcat(generic_type, ", ");
            }
            first = 0;
            
            if (is_builtin_type_keyword(current_token.text) || 
                current_token.type == TOKEN_IDENTIFIER ||
                current_token.type == TOKEN_KEYWORD) {
                strcat(generic_type, current_token.text);
                advance();
            } else if (current_token.type == TOKEN_SYMBOL && strcmp(current_token.text, ",") == 0) {
                advance();
            } else {
                fprintf(stderr, "Error (L%d:%d): Invalid token in generic type specification.\n", current_token.line, current_token.col);
                free(type_str);
                return NULL;
            }
        }
        strcat(generic_type, ">");
        advance(); // consume '>'
        
        free(type_str);
        type_str = strdup(generic_type);
    }

    int array_dims = 0;
    while (current_token.type == TOKEN_SYMBOL &&
        strcmp(current_token.text, "[") == 0) {

        advance();                           /* 1. eat '[' */

        if (current_token.type != TOKEN_SYMBOL ||
            strcmp(current_token.text, "]") != 0) {
            fprintf(stderr,
                "Error (L%d:%d): Expected ']' after '[' in array type declaration.\n",
                current_token.line, current_token.col);
            free(type_str);
            return NULL;
        }
        advance();                           /* 2. eat ']'   */
        array_dims++;                        /* 3. done – now current_token                                              is the NEXT real token     */
    }

    // Now expect the variable name
    if (current_token.type != TOKEN_IDENTIFIER) {
        fprintf(stderr, "Error (L%d:%d): Expected identifier after type '%s'\n", current_token.line, current_token.col, type_str);
        free(type_str);
        return NULL;
    }
    char var_name_str[sizeof(((ASTNode*)0)->value)]; // Ensure buffer is same size as ASTNode.value
    strncpy(var_name_str, current_token.text, sizeof(var_name_str) - 1);
    var_name_str[sizeof(var_name_str) - 1] = '\0';
    advance();

    // Do not modify type_str in-place; we'll build array suffix directly in var_decl below.
    // We'll set var_decl->is_array after we create the node below.
    ASTNode* var_decl = create_node(AST_TYPED_VAR_DECL, var_name_str, type_token.line, type_token.col);
    strncpy(var_decl->data_type, type_str, sizeof(var_decl->data_type) - 1);
    var_decl->data_type[sizeof(var_decl->data_type) - 1] = '\0';
    for(int i=0;i<array_dims;i++) strcat(var_decl->data_type, "[]");
    var_decl->is_array = array_dims > 0;
    free(type_str);

    var_decl->left = NULL;
    if ((current_token.type == TOKEN_SYMBOL || current_token.type == TOKEN_OPERATOR) && strcmp(current_token.text, "=") == 0) {
        advance();
        var_decl->right = parse_expression();
        if (!var_decl->right) {
            fprintf(stderr, "Error (L%d:%d): Expected expression after '='\n", current_token.line, current_token.col);
            free_ast(var_decl);
            return NULL;
        }
    }
    else {
        var_decl->right = NULL;
    }

    if (current_token.type != TOKEN_SYMBOL || strcmp(current_token.text, ";") != 0) {
        fprintf(stderr, "Error (L%d:%d): Expected ';' after variable declaration of '%s'\n", current_token.line, current_token.col, var_name_str);
        free_ast(var_decl);
        return NULL;
    }
    advance();
    return var_decl;
}

static ASTNode* parse_variable_declaration() {
    Token keyword_token = current_token;
    advance();

    // Support var[] declarations as typed declarations of type any[]
    if (strcmp(keyword_token.text, "var") == 0 && current_token.type == TOKEN_SYMBOL && strcmp(current_token.text, "[") == 0) {
        // Parse array dimensions
        char type_str[16] = "any";
        int array_dims = 0;
        while (current_token.type == TOKEN_SYMBOL && strcmp(current_token.text, "[") == 0) {
            advance(); // eat '['
            if (current_token.type != TOKEN_SYMBOL || strcmp(current_token.text, "]") != 0) {
                fprintf(stderr, "Error (L%d:%d): Expected ']' after '[' in var[] declaration.\n", current_token.line, current_token.col);
                return NULL;
            }
            advance(); // eat ']'
            array_dims++;
        }
        // Expect identifier
        if (current_token.type != TOKEN_IDENTIFIER) {
            fprintf(stderr, "Error (L%d:%d): Expected identifier after var[] declaration\n", current_token.line, current_token.col);
            return NULL;
        }
        char var_name_str[256];
        strncpy(var_name_str, current_token.text, sizeof(var_name_str) - 1);
        var_name_str[sizeof(var_name_str) - 1] = '\0';
        advance();
        // Create typed variable declaration node
        ASTNode* var_decl = create_node(AST_TYPED_VAR_DECL, var_name_str, keyword_token.line, keyword_token.col);
        strncpy(var_decl->data_type, type_str, sizeof(var_decl->data_type) - 1);
        var_decl->data_type[sizeof(var_decl->data_type) - 1] = '\0';
        for (int i = 0; i < array_dims; i++) strcat(var_decl->data_type, "[]");
        var_decl->is_array = array_dims > 0;
        var_decl->left = NULL;
        // Parse optional initializer
        if (current_token.type == TOKEN_SYMBOL && strcmp(current_token.text, "=") == 0) {
            advance();
            var_decl->right = parse_expression();
            if (!var_decl->right) {
                fprintf(stderr, "Error (L%d:%d): Failed to parse initializer expression for '%s'\n", current_token.line, current_token.col, var_decl->value);
                free_ast(var_decl);
                return NULL;
            }
        } else {
            var_decl->right = NULL;
        }
        // Expect semicolon
        if (current_token.type != TOKEN_SYMBOL || strcmp(current_token.text, ";") != 0) {
            fprintf(stderr, "Error (L%d:%d): Expected ';' after variable declaration of '%s'\n", current_token.line, current_token.col, var_decl->value);
            free_ast(var_decl);
            return NULL;
        }
        advance();
        return var_decl;
    }

    // Check for colon-style type annotation: let/var/const name: type
    if (current_token.type == TOKEN_IDENTIFIER) {
        Token name_token = current_token;
        Token next = peek_token();
        if (next.type == TOKEN_SYMBOL && strcmp(next.text, ":") == 0) {
            // This is colon-style with let/var/const
            char var_name_str[sizeof(((ASTNode*)0)->value)];
            strncpy(var_name_str, name_token.text, sizeof(var_name_str) - 1);
            var_name_str[sizeof(var_name_str) - 1] = '\0';
            advance(); // consume name
            advance(); // consume ':'
            
            if (!is_builtin_type_keyword(current_token.text) && 
                current_token.type != TOKEN_IDENTIFIER &&
                current_token.type != TOKEN_KEYWORD) {
                fprintf(stderr, "Error (L%d:%d): Expected type name after ':' in variable declaration.\n", current_token.line, current_token.col);
                return NULL;
            }
            
            char* type_str = strdup(current_token.text);
            advance();
            
            // Check for generic type syntax like array<int> or map<string, any>
            int array_dims = 0;
            if ((current_token.type == TOKEN_SYMBOL || current_token.type == TOKEN_OPERATOR) && strcmp(current_token.text, "<") == 0) {
                // Handle generic types
                char generic_type[256];
                snprintf(generic_type, sizeof(generic_type), "%s<", type_str);
                advance(); // consume '<'
                
                // Parse inner type(s)
                int first = 1;
                while (!((current_token.type == TOKEN_SYMBOL || current_token.type == TOKEN_OPERATOR) && strcmp(current_token.text, ">") == 0)) {
                    if (!first) {
                        strcat(generic_type, ", ");
                    }
                    first = 0;
                    
                    if (is_builtin_type_keyword(current_token.text) || 
                        current_token.type == TOKEN_IDENTIFIER ||
                        current_token.type == TOKEN_KEYWORD) {
                        strcat(generic_type, current_token.text);
                        advance();
                    } else if (current_token.type == TOKEN_SYMBOL && strcmp(current_token.text, ",") == 0) {
                        advance();
                    } else {
                        fprintf(stderr, "Error (L%d:%d): Invalid token in generic type specification.\n", current_token.line, current_token.col);
                        free(type_str);
                        return NULL;
                    }
                }
                strcat(generic_type, ">");
                advance(); // consume '>'
                
                free(type_str);
                type_str = strdup(generic_type);
            }
            
            // Check for array brackets
            while (current_token.type == TOKEN_SYMBOL && strcmp(current_token.text, "[") == 0) {
                advance(); // eat '['
                if (current_token.type != TOKEN_SYMBOL || strcmp(current_token.text, "]") != 0) {
                    fprintf(stderr, "Error (L%d:%d): Expected ']' after '[' in array type declaration.\n", current_token.line, current_token.col);
                    free(type_str);
                    return NULL;
                }
                advance(); // eat ']'
                array_dims++;
            }
            
            ASTNode* var_decl = create_node(AST_TYPED_VAR_DECL, var_name_str, keyword_token.line, keyword_token.col);
            strncpy(var_decl->data_type, type_str, sizeof(var_decl->data_type) - 1);
            var_decl->data_type[sizeof(var_decl->data_type) - 1] = '\0';
            for(int i = 0; i < array_dims; i++) strcat(var_decl->data_type, "[]");
            var_decl->is_array = array_dims > 0;
            free(type_str);
            
            if (strcmp(keyword_token.text, "const") == 0) {
                strncpy(var_decl->access_modifier, "const", sizeof(var_decl->access_modifier)-1);
            }
            
            var_decl->left = NULL;
            if ((current_token.type == TOKEN_SYMBOL || current_token.type == TOKEN_OPERATOR) && strcmp(current_token.text, "=") == 0) {
                advance();
                var_decl->right = parse_expression();
                if (!var_decl->right) {
                    fprintf(stderr, "Error (L%d:%d): Expected expression after '='\n", current_token.line, current_token.col);
                    free_ast(var_decl);
                    return NULL;
                }
            } else {
                var_decl->right = NULL;
            }
            
            if (current_token.type != TOKEN_SYMBOL || strcmp(current_token.text, ";") != 0) {
                fprintf(stderr, "Error (L%d:%d): Expected ';' after variable declaration of '%s'\n", current_token.line, current_token.col, var_name_str);
                free_ast(var_decl);
                return NULL;
            }
            advance();
            return var_decl;
        }
    }

    // Existing untyped var declaration
    if (current_token.type != TOKEN_IDENTIFIER) {
        fprintf(stderr, "Error (L%d:%d): Expected identifier after '%s'\n",
            keyword_token.line, keyword_token.col, keyword_token.text);
        return NULL;
    }

    ASTNode* var_decl = create_node(AST_VAR_DECL, current_token.text, keyword_token.line, keyword_token.col);
    if (strcmp(keyword_token.text, "const") == 0) {
        strncpy(var_decl->access_modifier, "const", sizeof(var_decl->access_modifier)-1);
    }
    var_decl->left = NULL; // For untyped VarDecl, left is not used for name node. Name is in value.
    advance();

    if ((current_token.type == TOKEN_OPERATOR || current_token.type == TOKEN_SYMBOL) && strcmp(current_token.text, "=") == 0) {
        advance();
        var_decl->right = parse_expression();
        if (!var_decl->right) {
            fprintf(stderr, "Error (L%d:%d): Failed to parse initializer expression for '%s'\n", current_token.line, current_token.col, var_decl->value);
            free_ast(var_decl); return NULL;
        }
    }
    else {
        var_decl->right = NULL;
    }

    if (current_token.type != TOKEN_SYMBOL || strcmp(current_token.text, ";") != 0) {
        fprintf(stderr, "Error (L%d:%d): Expected ';' after variable declaration of '%s'\n", current_token.line, current_token.col, var_decl->value);
        free_ast(var_decl); return NULL;
    }
    advance();
    return var_decl;
}

// The rest of parser.c (parse_typed_function, parse_parameters, etc.) would be here.
// I'll continue with the rest of the file, assuming the create_node updates are applied.

static ASTNode* parse_typed_function() {
    Token type_token = current_token;
    if (!is_builtin_type_keyword(current_token.text) && current_token.type != TOKEN_IDENTIFIER) {
        fprintf(stderr, "Error (L%d:%d): Expected return type for function.\n", current_token.line, current_token.col);
        return NULL;
    }
    char* type_str = strdup(current_token.text);
    advance();

    if (current_token.type != TOKEN_IDENTIFIER) {
        fprintf(stderr, "Error (L%d:%d): Expected function name after type '%s'\n", current_token.line, current_token.col, type_token.text);
        free(type_str);
        return NULL;
    }
    ASTNode* func = create_node(AST_TYPED_FUNCTION, current_token.text, type_token.line, type_token.col);
    strncpy(func->data_type, type_str, sizeof(func->data_type) - 1);
    func->data_type[sizeof(func->data_type) - 1] = '\0';
    free(type_str);
    advance();

    if (current_token.type != TOKEN_SYMBOL || strcmp(current_token.text, "(") != 0) {
        fprintf(stderr, "Error (L%d:%d): Expected '(' after function name '%s'\n", current_token.line, current_token.col, func->value);
        free_ast(func);
        return NULL;
    }
    advance();
    func->left = parse_parameters();

    if (current_token.type != TOKEN_SYMBOL || strcmp(current_token.text, "{") != 0) {
        fprintf(stderr, "Error (L%d:%d): Expected '{' to open function body for '%s'\n", current_token.line, current_token.col, func->value);
        free_ast(func);
        return NULL;
    }
    Token body_start_token = current_token;
    (void)body_start_token; // suppress unused variable warning
    advance();
    func->right = parse_block();
    if (!func->right) {
        fprintf(stderr, "Error (L%d:%d): Failed to parse function body for '%s'\n", body_start_token.line, body_start_token.col, func->value);
        free_ast(func);
        return NULL;
    }

    if (current_token.type != TOKEN_SYMBOL || strcmp(current_token.text, "}") != 0) {
        fprintf(stderr, "Error (L%d:%d): Expected '}' to close function body for '%s'. Got '%s'.\n", current_token.line, current_token.col, func->value, current_token.text);
        free_ast(func);
        return NULL;
    }
    advance();

    return func;
}


static ASTNode* parse_parameters() {
    ASTNode* head = NULL;
    ASTNode* tail = NULL;

    if (current_token.type == TOKEN_SYMBOL && strcmp(current_token.text, ")") == 0) {
        advance();
        return NULL;
    }

    while (current_token.type != TOKEN_EOF) {
        Token param_type_token = current_token;
        ASTNode* param_node = NULL;
        char inferred_type[64] = "any";

        if (is_builtin_type_keyword(current_token.text)) {
            // Typed parameter: <type> <name>
            char* param_type_str = strdup(current_token.text);
            advance();

            if (current_token.type != TOKEN_IDENTIFIER) {
                fprintf(stderr, "Error (L%d:%d): Expected parameter name after type '%s'\n", current_token.line, current_token.col, param_type_str);
                free(param_type_str);
                free_ast(head);
                return NULL;
            }
            param_node = create_node(AST_PARAMETER, current_token.text, param_type_token.line, param_type_token.col);
            strncpy(param_node->data_type, param_type_str, sizeof(param_node->data_type) - 1);
            param_node->data_type[sizeof(param_node->data_type) - 1] = '\0';
            free(param_type_str);
            advance();
        } else if (current_token.type == TOKEN_IDENTIFIER) {
            // Check for colon-style type annotation: paramName: type
            Token name_tok = current_token;
            Token next = peek_token();
            if (next.type == TOKEN_SYMBOL && strcmp(next.text, ":") == 0) {
                // Colon-style: name: type
                param_node = create_node(AST_PARAMETER, name_tok.text, param_type_token.line, param_type_token.col);
                advance(); // consume name
                advance(); // consume ':'
                
                if (!is_builtin_type_keyword(current_token.text) && current_token.type != TOKEN_IDENTIFIER) {
                    fprintf(stderr, "Error (L%d:%d): Expected type name after ':' in parameter.\n", current_token.line, current_token.col);
                    free_ast(param_node); free_ast(head); return NULL;
                }
                
                strncpy(param_node->data_type, current_token.text, sizeof(param_node->data_type) - 1);
                param_node->data_type[sizeof(param_node->data_type) - 1] = '\0';
                advance(); // consume type
            } else if (next.type == TOKEN_IDENTIFIER) {
                // Traditional style: type name
                char type_str[64];
                strncpy(type_str, name_tok.text, sizeof(type_str) - 1);
                type_str[sizeof(type_str) - 1] = '\0';
                advance(); // consume type name
                param_node = create_node(AST_PARAMETER, current_token.text, param_type_token.line, param_type_token.col);
                // Set data_type to the user-defined type
                strncpy(param_node->data_type, type_str, sizeof(param_node->data_type) - 1);
                param_node->data_type[sizeof(param_node->data_type) - 1] = '\0';
                advance(); // consume parameter name
            } else {
                // Untyped parameter: just a name; default type 'any'
                param_node = create_node(AST_PARAMETER, current_token.text, param_type_token.line, param_type_token.col);
                strncpy(param_node->data_type, inferred_type, sizeof(param_node->data_type) - 1);
                param_node->data_type[sizeof(param_node->data_type) - 1] = '\0';
                advance();
            }
        } else {
            fprintf(stderr, "Error (L%d:%d): Invalid token '%s' in parameter list\n", current_token.line, current_token.col, current_token.text);
            free_ast(head);
            return NULL;
        }

        // Check for array parameter type like: type name[]
        if (current_token.type == TOKEN_SYMBOL && strcmp(current_token.text, "[") == 0) {
            advance();
            if (current_token.type == TOKEN_SYMBOL && strcmp(current_token.text, "]") == 0) {
                advance();
                param_node->is_array = 1;
                strcat(param_node->data_type, "[]");
            }
            else {
                fprintf(stderr, "Error (L%d:%d): Expected ']' for array parameter '%s'.\n", current_token.line, current_token.col, param_node->value);
                free_ast(param_node); free_ast(head); return NULL;
            }
        }


        if (head == NULL) {
            head = tail = param_node;
        }
        else {
            tail->next = param_node;
            tail = param_node;
        }

        if (current_token.type == TOKEN_SYMBOL && strcmp(current_token.text, ")") == 0) {
            break;
        }

        if (current_token.type != TOKEN_SYMBOL || strcmp(current_token.text, ",") != 0) {
            fprintf(stderr, "Error (L%d:%d): Expected ',' or ')' in parameter list\n", current_token.line, current_token.col);
            free_ast(head);
            return NULL;
        }
        advance();
    }

    if (current_token.type == TOKEN_SYMBOL && strcmp(current_token.text, ")") == 0) {
        advance();
    }
    else {
        fprintf(stderr, "Error (L%d:%d): Expected ')' to close parameter list.\n", current_token.line, current_token.col);
        free_ast(head);
        return NULL;
    }
    return head;
}


static ASTNode* parse_struct_declaration() {
    Token struct_keyword_token = current_token;
    advance();
    if (current_token.type != TOKEN_IDENTIFIER) {
        fprintf(stderr, "Error (L%d:%d): Expected struct name\n", current_token.line, current_token.col);
        return NULL;
    }
    ASTNode* node = create_node(AST_STRUCT, current_token.text, struct_keyword_token.line, struct_keyword_token.col);
    advance();

    if (current_token.type != TOKEN_SYMBOL || strcmp(current_token.text, "{") != 0) {
        fprintf(stderr, "Error (L%d:%d): Expected '{' after struct name '%s'\n", current_token.line, current_token.col, node->value);
        free_ast(node);
        return NULL;
    }
    advance();

    ASTNode* members = NULL;
    ASTNode* last_member = NULL;
    while (current_token.type != TOKEN_EOF && !(current_token.type == TOKEN_SYMBOL && strcmp(current_token.text, "}") == 0)) {
        ASTNode* member = parse_typed_variable_declaration();
        if (member) {
            if (members == NULL) {
                members = last_member = member;
            }
            else {
                last_member->next = member;
                last_member = member;
            }
        }
        else {
            fprintf(stderr, "Error (L%d:%d): Failed to parse struct member in '%s'.\n", current_token.line, current_token.col, node->value);
            free_ast(node);
            free_ast(members);
            return NULL;
        }
    }
    node->left = members;

    if (current_token.type != TOKEN_SYMBOL || strcmp(current_token.text, "}") != 0) {
        fprintf(stderr, "Error (L%d:%d): Expected '}' to close struct definition '%s'.\n", current_token.line, current_token.col, node->value);
        free_ast(node);
        return NULL;
    }
    advance();
    return node;
}


static ASTNode* parse_class_declaration() {
    Token class_keyword_token = current_token;
    advance();
    if (current_token.type != TOKEN_IDENTIFIER) {
        fprintf(stderr, "Error (L%d:%d): Expected class name\n", current_token.line, current_token.col);
        return NULL;
    }
    ASTNode* node = create_node(AST_CLASS, current_token.text, class_keyword_token.line, class_keyword_token.col);
    advance();

    if (current_token.type == TOKEN_KEYWORD && strcmp(current_token.text, "extends") == 0) {
        advance();
        if (current_token.type != TOKEN_IDENTIFIER) {
            fprintf(stderr, "Error (L%d:%d): Expected base class name after 'extends' for class '%s'.\n", current_token.line, current_token.col, node->value);
            free_ast(node);
            return NULL;
        }
        node->right = create_node(AST_IDENTIFIER, current_token.text, current_token.line, current_token.col);
        advance();
    }


    if (current_token.type != TOKEN_SYMBOL || strcmp(current_token.text, "{") != 0) {
        fprintf(stderr, "Error (L%d:%d): Expected '{' after class name or inheritance specifier for '%s'\n", current_token.line, current_token.col, node->value);
        free_ast(node);
        return NULL;
    }
    advance();

    ASTNode* members = NULL;
    ASTNode* last_member = NULL;
    while (current_token.type != TOKEN_EOF && !(current_token.type == TOKEN_SYMBOL && strcmp(current_token.text, "}") == 0)) {
        Token member_start_token = current_token;
        ASTNode* member = parse_statement();

        if (member) {
            if (members == NULL) {
                members = last_member = member;
            }
            else {
                last_member->next = member;
                last_member = member;
            }
        }
        else {
            fprintf(stderr, "Error (L%d:%d): Failed to parse field or method in class '%s'.\n", member_start_token.line, member_start_token.col, node->value);
            if (current_token.type != TOKEN_EOF) advance(); else break;
        }
    }
    node->left = members;

    if (current_token.type != TOKEN_SYMBOL || strcmp(current_token.text, "}") != 0) {
        fprintf(stderr, "Error (L%d:%d): Expected '}' to close class definition '%s'.\n", current_token.line, current_token.col, node->value);
        free_ast(node);
        return NULL;
    }
    advance();
    return node;
}

// --- Expression Parsers ---

static ASTNode* parse_expression() {
    ASTNode* condition = NULL;
    ASTNode* left = parse_primary();
    if (!left) return NULL;
    condition = parse_binary_expression(left, 0);

    while (current_token.type == TOKEN_SYMBOL && strcmp(current_token.text, "?") == 0) {
        Token qtok = current_token;
        advance(); // consume '?'

        ASTNode* true_expr = parse_expression();
        if (!true_expr) { free_ast(condition); return NULL; }

        if (current_token.type != TOKEN_SYMBOL || strcmp(current_token.text, ":") != 0) {
            fprintf(stderr, "Error (L%d:%d): Expected ':' in ternary expression.\n", current_token.line, current_token.col);
            free_ast(condition); free_ast(true_expr); return NULL;
        }
        advance(); // consume ':'

        ASTNode* false_expr = parse_expression();
        if (!false_expr) { free_ast(condition); free_ast(true_expr); return NULL; }

        ASTNode* tern_node = create_node(AST_TERNARY, "?:", qtok.line, qtok.col);
        tern_node->left = condition; // condition
        tern_node->right = true_expr; // true branch
        tern_node->next = false_expr; // use next for false branch

        condition = tern_node; // continue in case of nested ternaries
    }
    return condition;
}

static ASTNode* parse_binary_expression(ASTNode* left, int min_precedence) {
    while (1) {
        Token op_token = current_token; // Save for potential operator
        int prec = 0;
        // Check if current token is a potential binary operator
        if (current_token.type == TOKEN_OPERATOR ||
            (current_token.type == TOKEN_SYMBOL && (strcmp(current_token.text, "=") == 0 ||
                strcmp(current_token.text, "<") == 0 ||
                strcmp(current_token.text, ">") == 0)
                )) {
            prec = get_precedence(current_token.text);
        }
        else {
            break; // Not a binary operator we handle here
        }

        if (prec <= min_precedence) {
            break;
        }

        advance(); // Consume the operator

        ASTNode* right = parse_primary(); // Parse RHS primary
        if (!right) { // Higher precedence ops bind tighter
            // If parse_primary fails, it's an error on RHS
            fprintf(stderr, "Error (L%d:%d): Expected expression for right-hand side of binary operator '%s'\n", op_token.line, op_token.col, op_token.text);
            free_ast(left);
            return NULL;
        }

        // Handle right-associativity or higher precedence on the right
        while (1) {
            Token next_op_token = current_token;
            int next_prec = 0;
            if (next_op_token.type == TOKEN_OPERATOR ||
                (next_op_token.type == TOKEN_SYMBOL && (strcmp(next_op_token.text, "=") == 0 ||
                    strcmp(next_op_token.text, "<") == 0 ||
                    strcmp(next_op_token.text, ">") == 0))) {
                next_prec = get_precedence(next_op_token.text);
            }
            else {
                break;
            }

            // For left-associative: if next_prec <= prec, break.
            // For right-associative (like '='): if next_prec < prec, break. (or handle with prec-1 for recursive call)
            if (strcmp(op_token.text, "=") == 0) { // Assignment is right-associative
                if (next_prec < prec) break; // For right-associative: recurse if same or higher precedence
                right = parse_binary_expression(right, prec - 1); // Pass (prec - 1) for right-associativity
            }
            else { // Left-associative
                if (next_prec <= prec) break;
                right = parse_binary_expression(right, next_prec);
            }

            if (!right) { free_ast(left); return NULL; }
        }

        ASTNode* new_left = create_node(AST_BINARY_OP, op_token.text, op_token.line, op_token.col);
        new_left->left = left;
        new_left->right = right;
        left = new_left;
    }
    return left;
}


static ASTNode* parse_primary() {
    ASTNode* node = NULL;
    Token start_token = current_token;

    // Handle anonymous inline function expressions like `func(x){ ... }` or `function(x){}`
    if (current_token.type == TOKEN_KEYWORD &&
        (strcmp(current_token.text, "func") == 0 || strcmp(current_token.text, "function") == 0)) {
        // Peek ahead: if the next token is '(', treat as anonymous function expression.
        Token peek_tok = peek_token();
        if (peek_tok.type == TOKEN_SYMBOL && strcmp(peek_tok.text, "(") == 0) {
            node = parse_anonymous_function();
            if (!node) return NULL;
        }
    }
    // Handle unary prefix operators
    if (!node) { // proceed with previous logic only if anonymous func didnt already parse
        if (current_token.type == TOKEN_OPERATOR &&
            (strcmp(current_token.text, "-") == 0 || strcmp(current_token.text, "+") == 0 || strcmp(current_token.text, "!") == 0 || strcmp(current_token.text, "++") == 0 || strcmp(current_token.text, "--") == 0)) {
            Token op_token = current_token;
            advance();
            // The operand of a unary operator should be parsed with a precedence higher than most binary operators.
            // parse_primary() itself or a specific parse_unary_operand() that handles high precedence (like member access) is needed.
            ASTNode* operand = parse_primary(); // Recursive call for chained unary or high-precedence constructs
            if (!operand) {
                fprintf(stderr, "Error (L%d:%d): Expected operand after unary operator '%s'.\n", op_token.line, op_token.col, op_token.text);
                return NULL;
            }
            node = create_node(AST_UNARY_OP, op_token.text, op_token.line, op_token.col);
            node->left = operand;
            // After parsing a unary expression, it can be the start of member access, etc.
            // So, fall through to the postfix operator loop.
        }
        else if (current_token.type == TOKEN_KEYWORD &&
            (strcmp(current_token.text, "true") == 0 || strcmp(current_token.text, "false") == 0)) {
            node = create_node(AST_LITERAL, current_token.text, start_token.line, start_token.col);
            strncpy(node->data_type, "bool", sizeof(node->data_type) - 1);
            node->data_type[sizeof(node->data_type) - 1] = '\0';
            advance();
        }
        else if (current_token.type == TOKEN_KEYWORD && strcmp(current_token.text, "null") == 0) {
            strncpy(node->data_type, "null", sizeof(node->data_type) - 1);
            node->data_type[sizeof(node->data_type) - 1] = '\0';
            advance();
        }
        else if (current_token.type == TOKEN_KEYWORD && strcmp(current_token.text, "this") == 0) {
            node = parse_this_reference();
        }
        else if (current_token.type == TOKEN_KEYWORD && strcmp(current_token.text, "super") == 0) {
            node = parse_super_reference();
        }
        else if (current_token.type == TOKEN_KEYWORD && strcmp(current_token.text, "new") == 0) {
            node = parse_new_expression();
        }
        else if (current_token.type == TOKEN_SYMBOL && strcmp(current_token.text, "(") == 0) {
            advance();
            node = parse_expression();
            if (!node) { return NULL; }
            if (current_token.type != TOKEN_SYMBOL || strcmp(current_token.text, ")") != 0) {
                fprintf(stderr, "Error (L%d:%d): Expected ')' after parenthesized expression.\n", start_token.line, start_token.col);
                free_ast(node); return NULL;
            }
            advance();
        }
        else if (current_token.type == TOKEN_SYMBOL && strcmp(current_token.text, "[") == 0) {
            node = parse_array_literal();
        }
        else if (current_token.type == TOKEN_SYMBOL && strcmp(current_token.text, "{") == 0) {
            // Distinguish map literal vs block: only parse map if key token follows
            Token next = peek_token();
            if (next.type == TOKEN_IDENTIFIER || next.type == TOKEN_STRING || next.type == TOKEN_NUMBER) {
                node = parse_map_literal();
            }
            // Otherwise leave '{' for block parsing in class/function
        }
        else if (strcmp(current_token.text, ".") == 0) {
            // member access should be handled as part of binary or primary, but parser handles this in eval
        }
        else {
            node = parse_literal_or_identifier(); // Handles numbers, strings, identifiers
        }
    }

    // Loop for postfix operators: member access '.', index '[]', function call '()'
    while (node != NULL) { // Condition ensures we don't loop if primary parsing failed
        if (current_token.type == TOKEN_SYMBOL && strcmp(current_token.text, ".") == 0) {
            node = parse_member_access(node); // Update node with the member access AST
            if (!node) return NULL; // Error in member access
        }
        else if (current_token.type == TOKEN_SYMBOL && strcmp(current_token.text, "[") == 0) {
            node = parse_member_access(node); // parse_member_access handles '[' for index
            if (!node) return NULL; // Error in index access
        }
        else if (current_token.type == TOKEN_SYMBOL && strcmp(current_token.text, "(") == 0) {
            // This is a function call where `node` is the function identifier/expression
            Token call_start_token = current_token; // For '('
            advance(); // consume '('
            ASTNode* args = NULL;
            ASTNode* last_arg = NULL;

            if (!(current_token.type == TOKEN_SYMBOL && strcmp(current_token.text, ")") == 0)) {
                while (1) {
                    ASTNode* arg = parse_expression();
                    if (!arg) {
                        fprintf(stderr, "Error (L%d:%d): Failed to parse function call argument for '%s'.\n", call_start_token.line, call_start_token.col, node->value);
                        free_ast(node); free_ast(args); return NULL;
                    }
                    if (!args) args = last_arg = arg;
                    else { last_arg->next = arg; last_arg = arg; }

                    if (current_token.type == TOKEN_SYMBOL && strcmp(current_token.text, ")") == 0) break;
                    if (current_token.type != TOKEN_SYMBOL || strcmp(current_token.text, ",") != 0) {
                        fprintf(stderr, "Error (L%d:%d): Expected ',' or ')' in argument list for '%s'.\n", current_token.line, current_token.col, node->value);
                        free_ast(node); free_ast(args); return NULL;
                    }
                    advance();
                }
            }
            if (current_token.type != TOKEN_SYMBOL || strcmp(current_token.text, ")") != 0) {
                fprintf(stderr, "Error (L%d:%d): Expected ')' to close argument list for '%s'.\n", current_token.line, current_token.col, node->value);
                free_ast(node); free_ast(args); return NULL;
            }
            advance();

            // Create AST_CALL node. `node` is the function being called.
            // If `node` was AST_MEMBER_ACCESS (obj.method), its value is "method", left is "obj".
            ASTNode* call_node = create_node(AST_CALL, node->value, node->line, node->col);
            call_node->left = args;
            if (node->type == AST_MEMBER_ACCESS) {
                call_node->right = node->left; // The object/class expression
                strncpy(call_node->value, node->value, sizeof(call_node->value) - 1); // The method name from member access
                // The original 'node' (which was AST_MEMBER_ACCESS) is now replaced by 'call_node'.
                // We need to free the old 'node' to avoid memory leak if it was heap allocated.
                // However, 'node' is just a pointer. If parse_member_access returned a new node,
                // the old 'target' was its child.
                // This part is tricky. Let's assume `node` points to the AST_MEMBER_ACCESS node.
                // We are effectively "upgrading" it to a call.
                // We need to ensure the original target (node->left) isn't lost if node itself is freed.
                ASTNode* target_for_call = node->left; // Save target from member access
                node->left = NULL; // Detach from old member access node before freeing it
                free_ast(node);    // Free the member access node
                call_node->right = target_for_call; // Set the target for the call
            }
            else {
                call_node->right = NULL; // Plain function call, no specific target object
            }
            node = call_node; // Update node to be the new AST_CALL node
        }
        else if (current_token.type == TOKEN_OPERATOR && (strcmp(current_token.text, "++") == 0 || strcmp(current_token.text, "--") == 0)) {
            Token post_op = current_token;
            advance();
            ASTNode* post_unary = create_node(AST_UNARY_OP, post_op.text, post_op.line, post_op.col);
            post_unary->left = node; // operand is the expression we've built so far
            node = post_unary; // the new expression becomes the operand with postfix operator
        }
        else {
            break; // Not a postfix operator we handle here, end loop
        }
    }
    return node;
}


// Simplified parse_member_access called by parse_primary's loop
// It handles ONE level of '.' or '[' access.
static ASTNode* parse_member_access(ASTNode* target) {
    Token op_token = current_token;

    if (current_token.type == TOKEN_SYMBOL && strcmp(current_token.text, ".") == 0) {
        advance();
        if (current_token.type != TOKEN_IDENTIFIER) {
            fprintf(stderr, "Error (L%d:%d): Expected identifier for member access after '.'.\n", op_token.line, op_token.col);
            free_ast(target);
            return NULL;
        }

        ASTNode* member_node = create_node(AST_MEMBER_ACCESS, current_token.text, op_token.line, op_token.col);
        member_node->left = target;
        advance();
        return member_node;

    }
    else if (current_token.type == TOKEN_SYMBOL && strcmp(current_token.text, "[") == 0) {
        advance();
        ASTNode* index_expr = parse_expression();
        if (!index_expr) {
            fprintf(stderr, "Error (L%d:%d): Expected expression for index access.\n", op_token.line, op_token.col);
            free_ast(target);
            return NULL;
        }
        ASTNode* index_node = create_node(AST_INDEX_ACCESS, "[]", op_token.line, op_token.col);
        index_node->left = target;
        index_node->right = index_expr;

        if (current_token.type != TOKEN_SYMBOL || strcmp(current_token.text, "]") != 0) {
            fprintf(stderr, "Error (L%d:%d): Expected ']'.\n", current_token.line, current_token.col);
            free_ast(target); free_ast(index_expr); free_ast(index_node);
            return NULL;
        }
        advance();
        return index_node;
    }
    return target; // Should not be reached if called correctly from parse_primary loop
}


static ASTNode* parse_literal_or_identifier() {
    ASTNodeType type;
    Token current_start_token = current_token;

    switch (current_token.type) {
    case TOKEN_NUMBER:
        type = AST_LITERAL;
        break;
    case TOKEN_STRING:
        type = AST_LITERAL;
        break;
    case TOKEN_BOOL:
        type = AST_LITERAL;
        break;
    case TOKEN_KEYWORD:
        if (strcmp(current_token.text, "null") == 0) {
            type = AST_LITERAL;
            break;
        }
        /* fall through */
    case TOKEN_IDENTIFIER:
        type = AST_IDENTIFIER;
        break;
    default:
        fprintf(stderr, "Error (L%d:%d): Expected literal or identifier, got '%s'.\n", current_start_token.line, current_start_token.col, current_start_token.text);
        return NULL;
    }
    ASTNode* node = create_node(type, current_token.text, current_start_token.line, current_start_token.col);
    if (type == AST_LITERAL) { // Set data_type for literals
        if (current_token.type == TOKEN_NUMBER) {
            strncpy(node->data_type, strchr(current_token.text, '.') ? "float" : "int", sizeof(node->data_type) - 1);
        }
        else if (current_token.type == TOKEN_STRING) {
            strncpy(node->data_type, "string", sizeof(node->data_type) - 1);
        }
        else if (current_token.type == TOKEN_BOOL) { // "true" or "false"
            strncpy(node->data_type, "bool", sizeof(node->data_type) - 1);
        }
        node->data_type[sizeof(node->data_type) - 1] = '\0';
    }
    advance();
    return node;
}


static ASTNode* parse_if_statement() {
    Token if_keyword_token = current_token;
    advance();

    if (current_token.type != TOKEN_SYMBOL || strcmp(current_token.text, "(") != 0) {
        fprintf(stderr, "Error (L%d:%d): Expected '(' after 'if'.\n", if_keyword_token.line, if_keyword_token.col);
        return NULL;
    }
    advance();

    ASTNode* condition = parse_expression();
    if (!condition) {
        // Error already reported by parse_expression
        return NULL;
    }

    if (current_token.type != TOKEN_SYMBOL || strcmp(current_token.text, ")") != 0) {
        fprintf(stderr, "Error (L%d:%d): Expected ')' after if-condition.\n", current_token.line, current_token.col);
        free_ast(condition); return NULL;
    }
    advance();

    ASTNode* then_block = NULL;
    if (current_token.type == TOKEN_SYMBOL && strcmp(current_token.text, "{") == 0) {
        Token then_body_start_token = current_token;
        advance();
        then_block = parse_block();
        if (!then_block) {
            fprintf(stderr, "Error (L%d:%d): Failed to parse 'then' block for if statement.\n", then_body_start_token.line, then_body_start_token.col);
            free_ast(condition); return NULL;
        }
        if (current_token.type != TOKEN_SYMBOL || strcmp(current_token.text, "}") != 0) {
            fprintf(stderr, "Error (L%d:%d): Expected '}' to close if-body. Got '%s'.\n", current_token.line, current_token.col, current_token.text);
            free_ast(condition); free_ast(then_block); return NULL;
        }
        advance();
    } else {
        then_block = parse_statement();
        if (!then_block) { free_ast(condition); return NULL; }
    }

    ASTNode* if_node = create_node(AST_IF, "if", if_keyword_token.line, if_keyword_token.col);
    if_node->left = condition;
    if_node->right = then_block;

    if (current_token.type == TOKEN_KEYWORD && strcmp(current_token.text, "else") == 0) {
        Token else_keyword_token = current_token;
        advance();

        ASTNode* else_node_content = NULL;
        if (current_token.type == TOKEN_KEYWORD && strcmp(current_token.text, "if") == 0) {
            else_node_content = parse_if_statement();
            if (!else_node_content) { free_ast(if_node); return NULL; }
        }
        else if (current_token.type == TOKEN_SYMBOL && strcmp(current_token.text, "{") == 0) {
            Token else_body_start_token = current_token;
            advance();
            else_node_content = parse_block();
            if (!else_node_content) {
                fprintf(stderr, "Error (L%d:%d): Failed to parse 'else' block.\n", else_body_start_token.line, else_body_start_token.col);
                free_ast(if_node); return NULL;
            }
            if (current_token.type != TOKEN_SYMBOL || strcmp(current_token.text, "}") != 0) {
                fprintf(stderr, "Error (L%d:%d): Expected '}' to close else-body. Got '%s'.\n", current_token.line, current_token.col, current_token.text);
                free_ast(if_node); free_ast(else_node_content); return NULL;
            }
            advance();
        } else {
            else_node_content = parse_statement();
            if (!else_node_content) { free_ast(if_node); return NULL; }
        }

        ASTNode* else_ast_node = create_node(AST_ELSE, "else", else_keyword_token.line, else_keyword_token.col);
        else_ast_node->left = else_node_content;
        if_node->next = else_ast_node;
    }
    return if_node;
}


static ASTNode* parse_while_statement() {
    Token while_keyword_token = current_token;
    advance();

    if (current_token.type != TOKEN_SYMBOL || strcmp(current_token.text, "(") != 0) {
        fprintf(stderr, "Error (L%d:%d): Expected '(' after 'while'.\n", while_keyword_token.line, while_keyword_token.col);
        return NULL;
    }
    advance();

    ASTNode* condition = parse_expression();
    if (!condition) { return NULL; }

    if (current_token.type != TOKEN_SYMBOL || strcmp(current_token.text, ")") != 0) {
        fprintf(stderr, "Error (L%d:%d): Expected ')' after while-condition.\n", current_token.line, current_token.col);
        free_ast(condition); return NULL;
    }
    advance();

    ASTNode* body = NULL;
    if (current_token.type == TOKEN_SYMBOL && strcmp(current_token.text, "{") == 0) {
        Token body_start_token = current_token;
        advance();
        body = parse_block();
        if (!body) {
            fprintf(stderr, "Error (L%d:%d): Failed to parse while-body.\n", body_start_token.line, body_start_token.col);
            free_ast(condition); return NULL;
        }
        if (current_token.type != TOKEN_SYMBOL || strcmp(current_token.text, "}") != 0) {
            fprintf(stderr, "Error (L%d:%d): Expected '}' to close while-body. Got '%s'.\n", current_token.line, current_token.col, current_token.text);
            free_ast(condition); free_ast(body); return NULL;
        }
        advance();
    } else {
        body = parse_statement();
        if (!body) { free_ast(condition); return NULL; }
    }

    ASTNode* while_node = create_node(AST_WHILE, "while", while_keyword_token.line, while_keyword_token.col);
    while_node->left = condition;
    while_node->right = body;
    return while_node;
}

static ASTNode* parse_for_statement() {
    Token for_keyword_token = current_token;
    advance();

    if (current_token.type != TOKEN_SYMBOL || strcmp(current_token.text, "(") != 0) {
        fprintf(stderr, "Error (L%d:%d): Expected '(' after 'for'.\n", for_keyword_token.line, for_keyword_token.col);
        return NULL;
    }
    advance();

    ASTNode* init_expr = NULL;
    int init_consumed_semicolon = 0; // Flag to indicate if the init part already consumed its ';'

    if (!(current_token.type == TOKEN_SYMBOL && strcmp(current_token.text, ";") == 0)) {
        Token before_init = current_token;

        // 1) Typed variable declaration (e.g., int i = 0)
        if (is_builtin_type_keyword(current_token.text)) {
            init_expr = parse_typed_variable_declaration();
            if (!init_expr) {
                fprintf(stderr, "Error (L%d:%d): Failed to parse for-loop typed initializer.\n", before_init.line, before_init.col);
                return NULL;
            }
            init_consumed_semicolon = 1; // parse_typed_variable_declaration consumes the ';'
        }
        // 2) 'let' or 'var' untyped declaration
        else if (current_token.type == TOKEN_KEYWORD && (strcmp(current_token.text, "let") == 0 || strcmp(current_token.text, "var") == 0)) {
            init_expr = parse_variable_declaration();
            if (!init_expr) {
                fprintf(stderr, "Error (L%d:%d): Failed to parse for-loop variable initializer.\n", before_init.line, before_init.col);
                return NULL;
            }
            init_consumed_semicolon = 1; // parse_variable_declaration consumes the ';'
        }
        // 3) General expression initializer
        else {
            init_expr = parse_expression();
            if (!init_expr) {
                fprintf(stderr, "Error (L%d:%d): Failed to parse for-loop initializer expression.\n", before_init.line, before_init.col);
                return NULL;
            }
        }
    }

    // If the initializer did NOT already consume a semicolon (expression form), expect and consume it now
    if (!init_consumed_semicolon) {
        if (current_token.type != TOKEN_SYMBOL || strcmp(current_token.text, ";") != 0) {
            fprintf(stderr, "Error (L%d:%d): Expected ';' after for-loop initializer.\n", current_token.line, current_token.col);
            free_ast(init_expr); return NULL;
        }
        advance();
    }

    ASTNode* cond_expr = NULL;
    if (!(current_token.type == TOKEN_SYMBOL && strcmp(current_token.text, ";") == 0)) {
        cond_expr = parse_expression();
        if (!cond_expr && strcmp(current_token.text, ";") != 0) {
            fprintf(stderr, "Error (L%d:%d): Failed to parse for-loop condition.\n", current_token.line, current_token.col);
            free_ast(init_expr); return NULL;
        }
    }
    if (current_token.type != TOKEN_SYMBOL || strcmp(current_token.text, ";") != 0) {
        fprintf(stderr, "Error (L%d:%d): Expected ';' after for-loop condition.\n", current_token.line, current_token.col);
        free_ast(init_expr); free_ast(cond_expr); return NULL;
    }
    advance();

    ASTNode* incr_expr = NULL;
    if (!(current_token.type == TOKEN_SYMBOL && strcmp(current_token.text, ")") == 0)) {
        incr_expr = parse_expression();
        if (!incr_expr && strcmp(current_token.text, ")") != 0) {
            fprintf(stderr, "Error (L%d:%d): Failed to parse for-loop increment.\n", current_token.line, current_token.col);
            free_ast(init_expr); free_ast(cond_expr); return NULL;
        }
    }
    if (current_token.type != TOKEN_SYMBOL || strcmp(current_token.text, ")") != 0) {
        fprintf(stderr, "Error (L%d:%d): Expected ')' after for-loop increment.\n", current_token.line, current_token.col);
        free_ast(init_expr); free_ast(cond_expr); free_ast(incr_expr); return NULL;
    }
    advance();

    ASTNode* body = NULL;
    if (current_token.type == TOKEN_SYMBOL && strcmp(current_token.text, "{") == 0) {
        Token body_start_token2 = current_token;
        advance();
        body = parse_block();
        if (!body) {
            fprintf(stderr, "Error (L%d:%d): Failed to parse for-body.\n", body_start_token2.line, body_start_token2.col);
            free_ast(init_expr); free_ast(cond_expr); free_ast(incr_expr); return NULL;
        }
        if (current_token.type != TOKEN_SYMBOL || strcmp(current_token.text, "}") != 0) {
            fprintf(stderr, "Error (L%d:%d): Expected '}' to close for-body. Got '%s'.\n", current_token.line, current_token.col, current_token.text);
            free_ast(init_expr); free_ast(cond_expr); free_ast(incr_expr); free_ast(body); return NULL;
        }
        advance();
    } else {
        body = parse_statement();
        if (!body) { free_ast(init_expr); free_ast(cond_expr); free_ast(incr_expr); return NULL; }
    }

    ASTNode* for_node = create_node(AST_FOR, "for", for_keyword_token.line, for_keyword_token.col);

    ASTNode* current_control = NULL;
    if (init_expr) {
        for_node->left = init_expr;
        current_control = init_expr;
    }
    if (cond_expr) {
        if (current_control) current_control->next = cond_expr;
        else for_node->left = cond_expr;
        current_control = cond_expr;
    }
    if (incr_expr) {
        if (current_control) current_control->next = incr_expr;
        else for_node->left = incr_expr;
    }
    if (current_control && !incr_expr && cond_expr) { // e.g. for(init; cond;)
        cond_expr->next = NULL;
    }
    else if (current_control && !cond_expr && init_expr) { // e.g. for(init;;incr) or for(init;;)
        init_expr->next = incr_expr; // incr_expr could be null here too
    }
    // If only init_expr: for_node->left = init_expr, init_expr->next = NULL (implicitly by create_node)
    // If no control parts, for_node->left is NULL.

    for_node->right = body;
    return for_node;
}


static ASTNode* parse_return_statement() {
    Token return_keyword_token = current_token;
    advance();
    ASTNode* node = create_node(AST_RETURN, "return", return_keyword_token.line, return_keyword_token.col);

    if (!(current_token.type == TOKEN_SYMBOL && strcmp(current_token.text, ";") == 0)) {
        node->left = parse_expression();
        if (!node->left && !(current_token.type == TOKEN_SYMBOL && strcmp(current_token.text, ";") == 0)) {
            fprintf(stderr, "Error (L%d:%d): Failed to parse return expression.\n", current_token.line, current_token.col);
            free_ast(node); return NULL;
        }
    }
    else {
        node->left = NULL;
    }

    if (current_token.type == TOKEN_SYMBOL && strcmp(current_token.text, ";") == 0) {
        advance();
    }
    else {
        fprintf(stderr, "Warning (L%d:%d): Missing semicolon after return statement.\n", return_keyword_token.line, return_keyword_token.col);
    }
    return node;
}

static ASTNode* parse_function() {
    Token func_keyword_token = current_token;
    advance();

    if (current_token.type != TOKEN_IDENTIFIER && 
        !(current_token.type == TOKEN_KEYWORD && strcmp(current_token.text, "new") == 0)) {
        fprintf(stderr, "Error (L%d:%d): Expected function name\n", func_keyword_token.line, func_keyword_token.col);
        return NULL;
    }

    ASTNodeType node_type = AST_FUNCTION;
    if (current_token.text[0] == 'm' && strcmp(current_token.text, "method") == 0) {
        node_type = AST_CLASS_METHOD;
    }

    ASTNode* func = create_node(node_type, current_token.text, func_keyword_token.line, func_keyword_token.col);
    Token func_name_token = current_token;
    advance();

    if (current_token.type != TOKEN_SYMBOL || strcmp(current_token.text, "(") != 0) {
        fprintf(stderr, "Error (L%d:%d): Expected '(' after function name '%s'\n", func_name_token.line, func_name_token.col, func_name_token.text);
        free_ast(func);
        return NULL;
    }
    advance();

    // **FIX 2: Use the robust `parse_parameters` function.**
    func->left = parse_parameters();
    // No need to check for ')' here, as parse_parameters consumes it or fails.

    if (current_token.type != TOKEN_SYMBOL || strcmp(current_token.text, "{") != 0) {
        fprintf(stderr, "Error (L%d:%d): Expected '{' to begin function body for '%s'\n", current_token.line, current_token.col, func_name_token.text);
        free_ast(func);
        return NULL;
    }
    Token body_start_token = current_token;
    advance();
    func->right = parse_block();
    if (!func->right) {
        fprintf(stderr, "Error (L%d:%d): Failed to parse function body for '%s'\n", body_start_token.line, body_start_token.col, func_name_token.text);
        free_ast(func);
        return NULL;
    }

    if (current_token.type != TOKEN_SYMBOL || strcmp(current_token.text, "}") != 0) {
        fprintf(stderr, "Error (L%d:%d): Expected '}' to close function body for '%s'. Got '%s'.\n", current_token.line, current_token.col, func_name_token.text, current_token.text);
        free_ast(func);
        return NULL;
    }
    advance();
    return func;
}

static ASTNode* parse_print_statement() {
    Token print_keyword_token = current_token;
    advance();

    if (current_token.type != TOKEN_SYMBOL || strcmp(current_token.text, "(") != 0) {
        fprintf(stderr, "Error (L%d:%d): Expected '(' after print.\n", print_keyword_token.line, print_keyword_token.col);
        return NULL;
    }
    advance();

    ASTNode* expr = parse_expression();
    if (!expr) {
        fprintf(stderr, "Error (L%d:%d): Expected expression in print statement.\n", current_token.line, current_token.col);
        return NULL;
    }

    if (current_token.type != TOKEN_SYMBOL || strcmp(current_token.text, ")") != 0) {
        fprintf(stderr, "Error (L%d:%d): Expected ')' after print argument\n", current_token.line, current_token.col);
        free_ast(expr); return NULL;
    }
    advance();

    if (current_token.type != TOKEN_SYMBOL || strcmp(current_token.text, ";") != 0) {
        fprintf(stderr, "Error (L%d:%d): Expected ';' after print statement.\n", current_token.line, current_token.col);
        free_ast(expr); return NULL;
    }
    advance();

    ASTNode* print_node = create_node(AST_PRINT, "print", print_keyword_token.line, print_keyword_token.col);
    print_node->left = expr;
    return print_node;
}

static ASTNode* parse_array_literal() {
    Token start_token = current_token; // '['
    advance();

    ASTNode* head_element = NULL;
    ASTNode* tail_element = NULL;

    if (!(current_token.type == TOKEN_SYMBOL && strcmp(current_token.text, "]") == 0)) {
        while (1) {
            ASTNode* elem_expr = parse_expression();
            if (!elem_expr) {
                fprintf(stderr, "Error (L%d:%d): Failed to parse array element.\n", current_token.line, current_token.col);
                free_ast(head_element); return NULL;
            }
            if (!head_element) head_element = tail_element = elem_expr;
            else { tail_element->next = elem_expr; tail_element = elem_expr; }

            /* After each element we must find either a comma (continue with next element)
               or a closing bracket (array terminator). This explicit branching also
               guarantees that a nested '[' which has already been consumed by
               parse_expression() does not trigger a false error here. */
            if (current_token.type == TOKEN_SYMBOL) {
                if (strcmp(current_token.text, ",") == 0) {
                    advance();           /* consume comma and parse next element */
                    continue;
                }
                if (strcmp(current_token.text, "]") == 0) {
                    break;               /* done – do NOT consume ']' here, handled below */
                }
            }
            fprintf(stderr, "Error (L%d:%d): Expected ',' or ']' in array literal.\n", current_token.line, current_token.col);
            free_ast(head_element); return NULL;
        }
    }

    if (current_token.type != TOKEN_SYMBOL || strcmp(current_token.text, "]") != 0) {
        fprintf(stderr, "Error (L%d:%d): Unterminated array literal, expected ']'.\n", start_token.line, start_token.col);
        free_ast(head_element); return NULL;
    }
    advance();

    ASTNode* arr_node = create_node(AST_ARRAY, "array_literal", start_token.line, start_token.col);
    arr_node->left = head_element;
    strncpy(arr_node->data_type, "array", sizeof(arr_node->data_type) - 1);
    arr_node->data_type[sizeof(arr_node->data_type) - 1] = '\0';
    return arr_node;
}


static ASTNode* parse_new_expression() {
    Token new_keyword_token = current_token;
    advance();
    if (current_token.type != TOKEN_IDENTIFIER) {
        fprintf(stderr, "Error (L%d:%d): Expected class name after 'new'.\n", new_keyword_token.line, new_keyword_token.col);
        return NULL;
    }
    ASTNode* node = create_node(AST_NEW, current_token.text, new_keyword_token.line, new_keyword_token.col);
    strncpy(node->data_type, current_token.text, sizeof(node->data_type) - 1);
    node->data_type[sizeof(node->data_type) - 1] = '\0';
    Token class_name_token = current_token;
    advance();

    if (current_token.type == TOKEN_SYMBOL && strcmp(current_token.text, "(") == 0) {
        advance();

        ASTNode* args = NULL;
        ASTNode* last_arg = NULL;

        if (!(current_token.type == TOKEN_SYMBOL && strcmp(current_token.text, ")") == 0)) {
            while (1) {
                ASTNode* arg = parse_expression();
                if (!arg) {
                    fprintf(stderr, "Error (L%d:%d): Failed to parse constructor argument for 'new %s'.\n", current_token.line, current_token.col, class_name_token.text);
                    free_ast(node); free_ast(args); return NULL;
                }
                if (args == NULL) args = last_arg = arg;
                else { last_arg->next = arg; last_arg = arg; }

                if (current_token.type == TOKEN_SYMBOL && strcmp(current_token.text, ")") == 0) break;
                if (current_token.type != TOKEN_SYMBOL || strcmp(current_token.text, ",") != 0) {
                    fprintf(stderr, "Error (L%d:%d): Expected ',' or ')' in constructor arguments for 'new %s'.\n", current_token.line, current_token.col, class_name_token.text);
                    free_ast(node); free_ast(args); return NULL;
                }
                advance();
            }
        }
        if (current_token.type != TOKEN_SYMBOL || strcmp(current_token.text, ")") != 0) {
            fprintf(stderr, "Error (L%d:%d): Expected ')' to close constructor arguments for 'new %s'.\n", current_token.line, current_token.col, class_name_token.text);
            free_ast(node); free_ast(args); return NULL;
        }
        advance();
        node->left = args;
    }
    else {
        node->left = NULL;
    }
    return node;
}

static ASTNode* parse_this_reference() {
    Token this_token = current_token;
    advance();
    ASTNode* node = create_node(AST_THIS, "this", this_token.line, this_token.col);
    return node;
}

static ASTNode* parse_super_reference() {
    Token super_token = current_token;
    advance();
    ASTNode* node = create_node(AST_SUPER, "super", super_token.line, super_token.col);
    return node;
}

static ASTNode* parse_import() {
    Token import_keyword_token = current_token;
    advance();

    if (current_token.type != TOKEN_STRING) {
        fprintf(stderr, "Error (L%d:%d): Expected string literal for module name after import\n", import_keyword_token.line, import_keyword_token.col);
        return NULL;
    }

    // Strip leading and trailing quotes from module name literal
    const char *raw = current_token.text;
    size_t len = strlen(raw);
    char mod_name[256];
    if (len >= 2 && raw[0] == '"' && raw[len-1] == '"') {
        size_t mod_len = len - 2;
        strncpy(mod_name, raw + 1, mod_len);
        mod_name[mod_len] = '\0';
    } else {
        strncpy(mod_name, raw, sizeof(mod_name) - 1);
        mod_name[sizeof(mod_name) - 1] = '\0';
    }
    ASTNode* import_node = create_node(AST_IMPORT, mod_name, import_keyword_token.line, import_keyword_token.col);
    advance();

    // Check for optional "as" alias
    if (current_token.type == TOKEN_KEYWORD && strcmp(current_token.text, "as") == 0) {
        advance();
        if (current_token.type != TOKEN_IDENTIFIER) {
            fprintf(stderr, "Error (L%d:%d): Expected identifier after 'as' in import statement\n", current_token.line, current_token.col);
            free_ast(import_node); return NULL;
        }
        // Store the alias in the left child
        import_node->left = create_node(AST_IDENTIFIER, current_token.text, current_token.line, current_token.col);
        advance();
    }

    if (current_token.type != TOKEN_SYMBOL || strcmp(current_token.text, ";") != 0) {
        fprintf(stderr, "Error (L%d:%d): Expected ';' after import statement\n", current_token.line, current_token.col);
        free_ast(import_node); return NULL;
    }
    advance();
    return import_node;
}

static ASTNode* parse_break_statement() {
    Token break_keyword_token = current_token;
    advance();
    ASTNode* node = create_node(AST_BREAK, "break", break_keyword_token.line, break_keyword_token.col);
    if (current_token.type == TOKEN_SYMBOL && strcmp(current_token.text, ";") == 0) {
        advance();
    }
    return node;
}

static ASTNode* parse_continue_statement() {
    Token continue_keyword_token = current_token;
    advance();
    ASTNode* node = create_node(AST_CONTINUE, "continue", continue_keyword_token.line, continue_keyword_token.col);
    if (current_token.type == TOKEN_SYMBOL && strcmp(current_token.text, ";") == 0) {
        advance();
    }
    return node;
}

static ASTNode* parse_map_literal() {
    Token start_tok = current_token; // should be '{'
    advance(); // consume '{'

    ASTNode* first_pair = NULL;
    ASTNode* last_pair = NULL;

    if (current_token.type == TOKEN_SYMBOL && strcmp(current_token.text, "}") == 0) {
        // empty map
        advance();
    } else {
        while (1) {
            // Parse key (identifier or string or number)
            ASTNode* key_node = NULL;
            if (current_token.type == TOKEN_IDENTIFIER || current_token.type == TOKEN_STRING || current_token.type == TOKEN_NUMBER) {
                key_node = parse_literal_or_identifier();
            } else {
                fprintf(stderr, "Error (L%d:%d): Expected map key identifier or literal.\n", current_token.line, current_token.col);
                free_ast(first_pair); return NULL;
            }

            if (current_token.type != TOKEN_SYMBOL || strcmp(current_token.text, ":") != 0) {
                fprintf(stderr, "Error (L%d:%d): Expected ':' after map key.\n", current_token.line, current_token.col);
                free_ast(first_pair); free_ast(key_node); return NULL;
            }
            Token colon_tok = current_token;
            advance(); // consume ':'

            ASTNode* value_expr = parse_expression();
            if (!value_expr) { free_ast(first_pair); free_ast(key_node); return NULL; }

            ASTNode* pair_node = create_node(AST_BINARY_OP, ":", colon_tok.line, colon_tok.col);
            pair_node->left = key_node;
            pair_node->right = value_expr;

            if (!first_pair) first_pair = last_pair = pair_node;
            else { last_pair->next = pair_node; last_pair = pair_node; }

            if (current_token.type == TOKEN_SYMBOL && strcmp(current_token.text, ",") == 0) {
                advance(); // consume ',' and continue
                continue;
            }
            else if (current_token.type == TOKEN_SYMBOL && strcmp(current_token.text, "}") == 0) {
                advance(); // consume '}' and break
                break;
            }
            else {
                fprintf(stderr, "Error (L%d:%d): Expected ',' or '}' in map literal.\n", current_token.line, current_token.col);
                free_ast(first_pair); return NULL;
            }
        }
    }

    ASTNode* map_node = create_node(AST_MAP, "map_literal", start_tok.line, start_tok.col);
    map_node->left = first_pair;
    return map_node;
}

// ----------------- Anonymous function (function expression) -----------------
static ASTNode* parse_anonymous_function() {
    Token func_keyword_token = current_token; // 'func' or 'function'
    advance();

    // Expect parameter list
    if (current_token.type != TOKEN_SYMBOL || strcmp(current_token.text, "(") != 0) {
        fprintf(stderr, "Error (L%d:%d): Expected '(' after anonymous function keyword.\n", current_token.line, current_token.col);
        return NULL;
    }
    advance();

    ASTNode* params = parse_parameters(); // This consumes the closing ')'

    if (current_token.type != TOKEN_SYMBOL || strcmp(current_token.text, "{") != 0) {
        fprintf(stderr, "Error (L%d:%d): Expected '{' to start anonymous function body.\n", current_token.line, current_token.col);
        free_ast(params);
        return NULL;
    }
    Token body_start_token = current_token;
    (void)body_start_token; // suppress unused variable warning
    advance();
    ASTNode* body_block = parse_block();
    if (!body_block) { free_ast(params); return NULL; }

    if (current_token.type != TOKEN_SYMBOL || strcmp(current_token.text, "}") != 0) {
        fprintf(stderr, "Error (L%d:%d): Expected '}' to close anonymous function body.\n", current_token.line, current_token.col);
        free_ast(params); free_ast(body_block); return NULL;
    }
    advance();

    static int anon_index = 0;
    char anon_name[32];
    snprintf(anon_name, sizeof(anon_name), "<anon_%d>", ++anon_index);

    ASTNode* func_node = create_node(AST_FUNCTION, anon_name, func_keyword_token.line, func_keyword_token.col);
    func_node->left = params;
    func_node->right = body_block;
    return func_node;
}
