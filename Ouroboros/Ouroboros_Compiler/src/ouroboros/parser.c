// parser.c
// Implements the syntactic analysis (parsing) for Ouroboros source code,
// building an Abstract Syntax Tree (AST).

#include "ouroboros/parser.h"
#include "ouroboros/ast.h"
#include "ouroboros/token.h"
#include <stdio.h>   // For fprintf
#include <stdlib.h>  // For malloc, free, exit
#include <string.h>  // For strcmp

// --- Internal Helper Function Declarations (Parser State Management) ---
static Token* peek(Parser* parser);
static Token* previous(Parser* parser);
static Token* advance(Parser* parser);
static bool check(Parser* parser, TokenType type);
static bool match(Parser* parser, int num_types, ...); // Variadic for multiple token types
static bool is_at_end(Parser* parser);
static void synchronize(Parser* parser); // Error recovery
static void parser_error(Parser* parser, Token* token, const char* message);

// --- Forward Declarations for Parsing Rules (Recursive Descent) ---
// We define these here so they can call each other mutually recursively.
// Implementations will follow later.
static ASTNode* parse_declaration(Parser* parser);
static ASTNode* parse_class_declaration(Parser* parser, Token* access_modifier_token);
static FunctionDeclarationNode* parse_function_declaration(Parser* parser, Token* access_modifier_token, Token* static_modifier_token);
static ConstructorDeclarationNode* parse_constructor_declaration(Parser* parser, IdentifierNode* class_name_node);
static FieldDeclarationNode* parse_field_declaration(Parser* parser, Token* access_modifier_token, Token* static_modifier_token, Token* immutability_modifier_token);
static EnumDeclarationNode* parse_enum_declaration(Parser* parser, Token* access_modifier_token);
static PackageDeclarationNode* parse_package_declaration(Parser* parser);
static ImportDeclarationNode* parse_import_declaration(Parser* parser);

static StatementNode* parse_statement(Parser* parser);
static BlockStatementNode* parse_block_statement(Parser* parser);
static ExpressionStatementNode* parse_expression_statement(Parser* parser);
static VariableDeclarationStatementNode* parse_variable_declaration_statement(Parser* parser, Token* modifier);
static IfStatementNode* parse_if_statement(Parser* parser);
static WhileStatementNode* parse_while_statement(Parser* parser);
static ForStatementNode* parse_for_statement(Parser* parser);
static DoWhileStatementNode* parse_do_while_statement(Parser* parser);
static ForEachStatementNode* parse_foreach_statement(Parser* parser);
static JumpStatementNode* parse_jump_statement(Parser* parser, TokenType type); // For break/continue
static ReturnStatementNode* parse_return_statement(Parser* parser);
static ThrowStatementNode* parse_throw_statement(Parser* parser);
static TryCatchFinallyStatementNode* parse_try_catch_finally_statement(Parser* parser);

static ExpressionNode* parse_expression(Parser* parser); // Entry point for expressions
// Operator precedence parsing functions (from lowest to highest precedence)
static ExpressionNode* parse_assignment_expression(Parser* parser);
static ExpressionNode* parse_ternary_expression(Parser* parser);
static ExpressionNode* parse_logical_or_expression(Parser* parser);
static ExpressionNode* parse_logical_and_expression(Parser* parser);
static ExpressionNode* parse_bitwise_or_expression(Parser* parser);
static ExpressionNode* parse_bitwise_xor_expression(Parser* parser);
static ExpressionNode* parse_bitwise_and_expression(Parser* parser);
static ExpressionNode* parse_equality_expression(Parser* parser);
static ExpressionNode* parse_comparison_expression(Parser* parser);
static ExpressionNode* parse_shift_expression(Parser* parser);
static ExpressionNode* parse_additive_expression(Parser* parser);
static ExpressionNode* parse_multiplicative_expression(Parser* parser);
static ExpressionNode* parse_unary_expression(Parser* parser);
static ExpressionNode* parse_call_member_array_expression(Parser* parser); // Handles call, member access, array access
static ExpressionNode* parse_primary_expression(Parser* parser);

// Utility functions for parsing lists
static ParameterNode* parse_parameter(Parser* parser);
static TypeReferenceNode* parse_type_reference(Parser* parser);
static IdentifierNode* parse_dotted_identifier(Parser* parser); // For package/import names like 'com.example.MyClass'


// --- Public API Implementations ---

/**
 * Initializes a new Parser instance.
 * @param tokens A dynamically allocated array of Token pointers from the lexer.
 * The parser takes ownership of this array and its contents.
 * @param token_count The number of tokens in the array.
 * @param source_name The name of the source file.
 * @return A dynamically allocated Parser pointer, or NULL on allocation failure.
 */
Parser* parser_init(Token** tokens, size_t token_count, const char* source_name) {
    Parser* parser = (Parser*)malloc(sizeof(Parser));
    if (parser == NULL) {
        fprintf(stderr, "Error: Memory allocation failed for Parser.\n");
        return NULL;
    }
    parser->tokens = tokens;
    parser->token_count = token_count;
    parser->current_token_idx = 0;
    parser->source_name = source_name;
    return parser;
}

/**
 * Parses the stream of tokens and constructs the Abstract Syntax Tree (AST).
 * This is the main entry point for the parsing process.
 * @param parser A pointer to the Parser instance.
 * @return The root ASTNode of the parsed program (a ProgramNode), or NULL if parsing fails.
 * The caller is responsible for freeing the AST using ast_free_program().
 */
ASTNode* parser_parse(Parser* parser) {
    printf("--- Starting Parsing ---\n");
    ProgramNode* program_node = NULL;
    ASTNode** declarations = NULL;
    size_t declaration_count = 0;
    size_t declarations_capacity = 8; // Initial capacity for declarations

    declarations = (ASTNode**)ast_safe_malloc_array(declarations_capacity, sizeof(ASTNode*));

    // Consume optional package declaration
    if (check(parser, PACKAGE)) {
        Token* package_token = advance(parser); // Consume 'package'
        PackageDeclarationNode* pkg_decl = parse_package_declaration(parser);
        if (pkg_decl == NULL) {
             // Error occurred in package declaration, parser_error already called.
             // Attempt to recover or return NULL.
             free(declarations); // Clean up if we're aborting
             return NULL;
        }
        declarations[declaration_count++] = (ASTNode*)pkg_decl;
        // Optionally resize array if needed for more declarations
        if (declaration_count == declarations_capacity) {
            declarations_capacity *= 2;
            declarations = (ASTNode**)realloc(declarations, declarations_capacity * sizeof(ASTNode*));
            if (declarations == NULL) {
                fprintf(stderr, "Fatal Error: Failed to reallocate memory for declarations array.\n");
                exit(EXIT_FAILURE);
            }
        }
    }

    // Consume import declarations
    while (check(parser, IMPORT)) {
        Token* import_token = advance(parser); // Consume 'import'
        ImportDeclarationNode* imp_decl = parse_import_declaration(parser);
        if (imp_decl == NULL) {
             // Error occurred in import declaration
             free(declarations);
             return NULL;
        }
        declarations[declaration_count++] = (ASTNode*)imp_decl;
        if (declaration_count == declarations_capacity) {
            declarations_capacity *= 2;
            declarations = (ASTNode**)realloc(declarations, declarations_capacity * sizeof(ASTNode*));
            if (declarations == NULL) {
                fprintf(stderr, "Fatal Error: Failed to reallocate memory for declarations array.\n");
                exit(EXIT_FAILURE);
            }
        }
    }


    // Main loop to parse top-level declarations (classes, functions, enums)
    while (!is_at_end(parser)) {
        ASTNode* decl = parse_declaration(parser);
        if (decl == NULL) {
            // Error occurred during declaration parsing.
            // synchronize() attempts to move past the error.
            synchronize(parser);
            if (is_at_end(parser)) { // If sync didn't find anything usable
                break;
            }
            continue; // Try parsing next declaration
        }

        // Add parsed declaration to the list
        declarations[declaration_count++] = decl;

        // Resize array if capacity is reached
        if (declaration_count == declarations_capacity) {
            declarations_capacity *= 2;
            declarations = (ASTNode**)realloc(declarations, declarations_capacity * sizeof(ASTNode*));
            if (declarations == NULL) {
                fprintf(stderr, "Fatal Error: Failed to reallocate memory for declarations array.\n");
                exit(EXIT_FAILURE);
            }
        }
    }

    // Create the final ProgramNode
    // Pass a token for the program node, perhaps the first token or a dummy token
    Token* program_token = (parser->token_count > 0) ? parser->tokens[0] : NULL;
    program_node = ast_create_program_node(declarations, declaration_count, program_token);

    // Free the dynamically allocated declarations array itself if it's no longer needed after program_node creation,
    // assuming program_node duplicates it or takes ownership.
    // In our ast_create_program_node, it duplicates, so we free the temp array:
    free(declarations);

    printf("--- Parsing Complete ---\n");
    return (ASTNode*)program_node;
}


/**
 * Frees the memory associated with a Parser instance and the tokens it owns.
 * Note: This function will free the tokens array itself, and each individual Token*
 * within it using free_token().
 * @param parser A pointer to the Parser instance to free.
 */
void parser_free(Parser* parser) {
    if (parser) {
        if (parser->tokens) {
            // Free individual tokens first
            for (size_t i = 0; i < parser->token_count; ++i) {
                if (parser->tokens[i]) {
                    free_token(parser->tokens[i]);
                }
            }
            // Then free the array of token pointers
            free(parser->tokens);
        }
        free(parser);
    }
}


// --- Internal Helper Function Implementations (Parser State Management) ---

/**
 * Returns the current token without consuming it.
 */
static Token* peek(Parser* parser) {
    if (is_at_end(parser)) {
        return parser->tokens[parser->token_count - 1]; // Return EOF_TOKEN
    }
    return parser->tokens[parser->current_token_idx];
}

/**
 * Returns the previously consumed token.
 */
static Token* previous(Parser* parser) {
    if (parser->current_token_idx == 0) {
        return NULL; // No previous token
    }
    return parser->tokens[parser->current_token_idx - 1];
}

/**
 * Consumes the current token and returns it.
 */
static Token* advance(Parser* parser) {
    if (!is_at_end(parser)) {
        parser->current_token_idx++;
    }
    return previous(parser); // Return the token that *was* current
}

/**
 * Checks if the current token is of the expected type.
 */
static bool check(Parser* parser, TokenType type) {
    if (is_at_end(parser)) {
        return false;
    }
    return peek(parser)->type == type;
}

/**
 * Checks if the current token is one of the given types. If it is, consumes the token
 * and returns true. Otherwise, leaves the token and returns false.
 */
static bool match(Parser* parser, int num_types, ...) {
    va_list args;
    va_start(args, num_types);

    for (int i = 0; i < num_types; ++i) {
        TokenType type = va_arg(args, TokenType);
        if (check(parser, type)) {
            advance(parser);
            va_end(args);
            return true;
        }
    }
    va_end(args);
    return false;
}

/**
 * Returns true if the parser has reached the end of the token stream (EOF_TOKEN).
 */
static bool is_at_end(Parser* parser) {
    return peek(parser)->type == EOF_TOKEN;
}

/**
 * Reports a parsing error.
 * In a real compiler, this might record errors and continue. For simplicity,
 * we'll print to stderr.
 */
static void parser_error(Parser* parser, Token* token, const char* message) {
    fprintf(stderr, "Syntax Error at %s Line: %d, Column: %d: %s",
            token->source_name ? token->source_name : "UnknownSource",
            token->line, token->column, message);
    if (token->type != EOF_TOKEN) {
        fprintf(stderr, " (found: '%s' of type %s)", token->lexeme, "UnknownType"); // Need token type to string here
    }
    fprintf(stderr, "\n");
    // Optionally set a flag in the parser to indicate an error occurred
}

/**
 * Attempts to recover from a parsing error by discarding tokens until a likely
 * synchronization point (e.g., end of a statement, start of a new declaration).
 */
static void synchronize(Parser* parser) {
    advance(parser); // Discard the erroneous token

    while (!is_at_end(parser)) {
        // If the previous token was a semicolon, we might be at the end of a statement.
        if (previous(parser)->type == SEMICOLON) {
            return;
        }

        // Look for keywords that typically start new declarations or statements.
        switch (peek(parser)->type) {
            case CLASS:
            case FUN: // Placeholder for function keyword if not method (might be 'fn' etc.)
            case VAR:
            case LET:
            case CONST:
            case FINAL:
            case IF:
            case WHILE:
            case FOR:
            case RETURN:
                return; // Found a good synchronization point
            default:
                // Keep advancing past tokens until a potential sync point is found.
                break;
        }
        advance(parser);
    }
}

// --- Parsing Rule Implementations ---

/**
 * Parses a top-level declaration (class, function, field, enum, package, import).
 * Handles access modifiers and static/immutability modifiers.
 * @param parser The parser instance.
 * @return An ASTNode* representing the parsed declaration, or NULL on error.
 */
static ASTNode* parse_declaration(Parser* parser) {
    // Handle package and import declarations (already done in parser_parse)
    // Here, we focus on actual code declarations.

    Token* access_modifier_token = NULL;
    if (match(parser, 4, PUBLIC, PRIVATE, PROTECTED, INTERNAL)) {
        access_modifier_token = previous(parser);
    }

    Token* static_modifier_token = NULL;
    if (match(parser, 1, STATIC)) {
        static_modifier_token = previous(parser);
    }

    Token* immutability_modifier_token = NULL;
    if (match(parser, 2, CONST, FINAL)) {
        immutability_modifier_token = previous(parser);
    }

    // Order matters for disambiguation:
    // class/interface/enum come first if 'static' is not allowed there
    // If 'static' modifies class, this logic needs adjustment.
    // Assuming 'static' only applies to functions/fields for now.

    if (match(parser, 1, CLASS)) {
        // Class Declaration
        return (ASTNode*)parse_class_declaration(parser, access_modifier_token);
    }
    // TODO: Add INTERFACE, ENUM parsing here.
    // For now, if modifiers are present and it's not a class, assume it's a function or field.

    // If it's not a class/interface/enum, it's likely a function or a field.
    // We need to look for a return type (or void) and then an identifier.
    // This is a common disambiguation point.
    TypeReferenceNode* return_type_or_field_type = parse_type_reference(parser);
    if (return_type_or_field_type != NULL) {
        // If we have a type, the next must be an identifier (function name or field name)
        if (check(parser, IDENTIFIER)) {
            Token* name_token = peek(parser); // Don't advance yet, wait for disambiguation
            // Heuristic for function vs. field: look for '('
            if (check(parser, LPARENT)) { // It's likely a function or constructor
                // TODO: Need to handle constructors (name matches class name)
                IdentifierNode* func_name_node = ast_create_identifier_node(name_token, name_token->lexeme);
                advance(parser); // Consume function name token
                return (ASTNode*)parse_function_declaration(parser, access_modifier_token, static_modifier_token);
            } else {
                // It's a field declaration
                // In this case, previous() after consume would be the field name.
                // We've already advanced type_ref token by calling parse_type_reference.
                // So, current_token_idx points to IDENTIFIER (name_token).
                IdentifierNode* field_name_node = ast_create_identifier_node(name_token, name_token->lexeme);
                advance(parser); // Consume field name token
                return (ASTNode*)parse_field_declaration(parser, access_modifier_token, static_modifier_token, immutability_modifier_token);
            }
        }
    }

    // If no declaration matched, report an error.
    parser_error(parser, peek(parser), "Expected a declaration.");
    return NULL; // Return NULL on error, parser_parse will synchronize
}

/**
 * Parses a class declaration.
 * @param parser The parser instance.
 * @param access_modifier_token Optional access modifier (PUBLIC, PRIVATE, etc.).
 * @return A ClassInterfaceDeclarationNode* or NULL on error.
 */
static ASTNode* parse_class_declaration(Parser* parser, Token* access_modifier_token) {
    // Current token should be 'class' (already consumed in parse_declaration)
    // Next token should be the class name
    if (!check(parser, IDENTIFIER)) {
        parser_error(parser, peek(parser), "Expected class name after 'class'.");
        return NULL;
    }
    Token* class_name_token = advance(parser);
    IdentifierNode* class_name_node = ast_create_identifier_node(class_name_token, class_name_token->lexeme);

    // Parse 'extends' clause (optional)
    TypeReferenceNode** super_classes = NULL;
    size_t super_class_count = 0;
    if (match(parser, 1, EXTENDS)) {
        super_classes = (TypeReferenceNode**)ast_safe_malloc_array(1, sizeof(TypeReferenceNode*));
        super_classes[0] = parse_type_reference(parser);
        if (super_classes[0] == NULL) { /* handle error */ }
        super_class_count = 1;
    }

    // Parse 'implements' clause (optional, can be multiple)
    TypeReferenceNode** implemented_interfaces = NULL;
    size_t implemented_interface_count = 0;
    size_t interface_capacity = 2; // Initial capacity
    if (match(parser, 1, IMPLEMENTS)) {
        implemented_interfaces = (TypeReferenceNode**)ast_safe_malloc_array(interface_capacity, sizeof(TypeReferenceNode*));
        do {
            TypeReferenceNode* interface_type = parse_type_reference(parser);
            if (interface_type == NULL) { /* handle error */ break; }
            implemented_interfaces[implemented_interface_count++] = interface_type;
            if (implemented_interface_count == interface_capacity) {
                interface_capacity *= 2;
                implemented_interfaces = (TypeReferenceNode**)realloc(implemented_interfaces, interface_capacity * sizeof(TypeReferenceNode*));
                if (implemented_interfaces == NULL) { /* Fatal Error */ exit(EXIT_FAILURE); }
            }
        } while (match(parser, 1, COMMA));
    }


    // Expect opening brace for class body
    if (!match(parser, 1, LBRACE)) {
        parser_error(parser, peek(parser), "Expected '{' before class body.");
        // Free previously allocated nodes
        ast_free_node((ASTNode*)class_name_node);
        // ... free super_classes and implemented_interfaces arrays/contents
        return NULL;
    }
    Token* class_body_lbrace_token = previous(parser); // Token for '{'

    // Parse class members (fields, methods, constructors)
    ASTNode** members = NULL;
    size_t member_count = 0;
    size_t members_capacity = 8;
    members = (ASTNode**)ast_safe_malloc_array(members_capacity, sizeof(ASTNode*));

    while (!check(parser, RBRACE) && !is_at_end(parser)) {
        // Parse access modifiers for members
        Token* member_access_mod = NULL;
        if (match(parser, 4, PUBLIC, PRIVATE, PROTECTED, INTERNAL)) {
            member_access_mod = previous(parser);
        }
        Token* member_static_mod = NULL;
        if (match(parser, 1, STATIC)) {
            member_static_mod = previous(parser);
        }
        Token* member_immutability_mod = NULL;
        if (match(parser, 2, CONST, FINAL)) {
            member_immutability_mod = previous(parser);
        }

        // Determine if it's a field, method, or constructor
        // This requires looking ahead or more complex disambiguation
        // For simplicity, let's assume methods first, then fields.
        // A better approach involves trying to parse function, then field.

        // Try to parse a method/constructor
        // Check for return type or identifier (which could be constructor name)
        TypeReferenceNode* member_return_type = NULL;
        bool is_constructor = false;

        // If next token is Identifier and matches class name, it's a constructor
        // Or if it's 'void' or a type for a regular method
        if (check(parser, VOID) || check(parser, IDENTIFIER)) { // Could be void func() or Type func()
            if (check(parser, IDENTIFIER) && strcmp(peek(parser)->lexeme, class_name_node->name) == 0) {
                // Potential constructor
                is_constructor = true;
            } else {
                // Potential method
                member_return_type = parse_type_reference(parser); // Will advance
                if (member_return_type == NULL) {
                    // This was not a type, maybe a field declaration or another error.
                    parser_error(parser, peek(parser), "Expected a type or method/constructor name.");
                    synchronize(parser);
                    continue; // Skip to next iteration
                }
            }
        }


        if (is_constructor) {
            ConstructorDeclarationNode* ctor_decl = parse_constructor_declaration(parser, class_name_node);
            if (ctor_decl) {
                members[member_count++] = (ASTNode*)ctor_decl;
            } else {
                // Error in constructor parsing
                synchronize(parser);
            }
        } else if (check(parser, IDENTIFIER)) { // After return type, expect method/field name
            Token* member_name_token = peek(parser);
            if (check(parser, LPARENT)) { // It's a method
                // We've already parsed member_return_type and found an identifier.
                // Now confirm it's a method by seeing '('
                // Then parse the function.
                FunctionDeclarationNode* func_decl = parse_function_declaration(parser, member_access_mod, member_static_mod);
                if (func_decl) {
                    members[member_count++] = (ASTNode*)func_decl;
                } else {
                    synchronize(parser);
                }
            } else { // It's a field
                // This is a bit tricky, as parse_field_declaration expects modifiers and type.
                // Need to re-think how modifiers are passed if type is already parsed.
                // For now, let's simplify and re-parse types/modifiers *inside* field/func parse.
                // This is a simplification; a full parser would manage this.
                parser_error(parser, peek(parser), "Expected '(' for method or end of field declaration.");
                synchronize(parser);
            }
        } else {
            parser_error(parser, peek(parser), "Expected a class member declaration (field, method, or constructor).");
            synchronize(parser);
        }

        // Resize array if capacity is reached
        if (member_count == members_capacity) {
            members_capacity *= 2;
            members = (ASTNode**)realloc(members, members_capacity * sizeof(ASTNode*));
            if (members == NULL) { /* Fatal Error */ exit(EXIT_FAILURE); }
        }
    }

    if (!match(parser, 1, RBRACE)) {
        parser_error(parser, peek(parser), "Expected '}' after class body.");
        // Free partially built AST
        return NULL;
    }

    ClassInterfaceDeclarationNode* class_node = ast_create_class_interface_decl_node(
        DECL_CLASS, access_modifier_token, class_name_node,
        super_classes, super_class_count,
        implemented_interfaces, implemented_interface_count,
        members, member_count,
        class_body_lbrace_token // Use the LBRACE token for line/column of class
    );

    // Free the temporary arrays for super_classes, implemented_interfaces, and members,
    // as ast_create_class_interface_decl_node duplicates them.
    if (super_classes) free(super_classes);
    if (implemented_interfaces) free(implemented_interfaces);
    if (members) free(members);

    return (ASTNode*)class_node;
}


// --- Main Parsing Rule Implementations (Basic) ---

/**
 * Parses a package declaration.
 * Assumes 'package' token has already been consumed.
 * @return A PackageDeclarationNode* or NULL on error.
 */
static PackageDeclarationNode* parse_package_declaration(Parser* parser) {
    Token* package_token = previous(parser); // The 'package' keyword token

    IdentifierNode* package_name = parse_dotted_identifier(parser);
    if (package_name == NULL) {
        parser_error(parser, peek(parser), "Expected package name after 'package'.");
        return NULL;
    }

    if (!match(parser, 1, SEMICOLON)) {
        parser_error(parser, peek(parser), "Expected ';' after package declaration.");
        ast_free_node((ASTNode*)package_name);
        return NULL;
    }

    return ast_create_package_decl_node(package_name, package_token);
}

/**
 * Parses an import declaration.
 * Assumes 'import' token has already been consumed.
 * @return An ImportDeclarationNode* or NULL on error.
 */
static ImportDeclarationNode* parse_import_declaration(Parser* parser) {
    Token* import_token = previous(parser); // The 'import' keyword token

    IdentifierNode* imported_name = parse_dotted_identifier(parser);
    if (imported_name == NULL) {
        parser_error(parser, peek(parser), "Expected import path after 'import'.");
        return NULL;
    }

    bool is_wildcard = false;
    if (match(parser, 1, MULTIPLY)) { // Check for '*' for wildcard import (e.g., import my.package.*;)
        is_wildcard = true;
    }

    if (!match(parser, 1, SEMICOLON)) {
        parser_error(parser, peek(parser), "Expected ';' after import declaration.");
        ast_free_node((ASTNode*)imported_name);
        return NULL;
    }

    return ast_create_import_decl_node(imported_name, is_wildcard, import_token);
}


/**
 * Parses a dotted identifier (e.g., `java.lang.String`).
 * Used for package names, import paths, fully qualified names.
 * @return An IdentifierNode* representing the full dotted name, or NULL on error.
 */
static IdentifierNode* parse_dotted_identifier(Parser* parser) {
    if (!check(parser, IDENTIFIER)) {
        parser_error(parser, peek(parser), "Expected identifier.");
        return NULL;
    }

    Token* first_id_token = advance(parser);
    char* full_name = ast_safe_strdup(first_id_token->lexeme);
    size_t current_len = strlen(full_name);
    size_t capacity = current_len + 16; // Initial extra capacity

    while (match(parser, 1, PERIOD)) {
        if (!check(parser, IDENTIFIER)) {
            parser_error(parser, peek(parser), "Expected identifier after '.'.");
            free(full_name);
            return NULL;
        }
        Token* next_id_token = advance(parser);

        // Resize buffer if needed
        size_t id_len = strlen(next_id_token->lexeme);
        if (current_len + 1 + id_len + 1 > capacity) {
            capacity = current_len + 1 + id_len + 1 + 16; // Grow capacity
            full_name = (char*)realloc(full_name, capacity);
            if (full_name == NULL) {
                fprintf(stderr, "Fatal Error: Failed to reallocate memory for dotted identifier.\n");
                exit(EXIT_FAILURE);
            }
        }
        strcat(full_name, ".");
        strcat(full_name, next_id_token->lexeme);
        current_len = strlen(full_name);
    }

    IdentifierNode* node = ast_create_identifier_node(first_id_token, full_name);
    free(full_name); // Free the temporary buffer
    return node;
}


// Placeholder for parse_type_reference
static TypeReferenceNode* parse_type_reference(Parser* parser) {
    // For now, only supports simple type names like 'int', 'String', 'MyClass'
    if (match(parser, 7, CHAR_TYPE, SHORT_TYPE, INT_TYPE, LONG_TYPE, FLOAT_TYPE, DOUBLE_TYPE, BOOLEAN_TYPE, STRING_TYPE) ||
        check(parser, IDENTIFIER)) { // Could be a custom class name
        Token* type_token = previous(parser); // If matched a type keyword
        if (type_token == NULL) { // If it was an IDENTIFIER
             type_token = advance(parser);
        }

        // Handle array types: e.g., int[] or String[][]
        // while (match(parser, 1, LBRACKET)) {
        //     if (!match(parser, 1, RBRACKET)) {
        //         parser_error(parser, peek(parser), "Expected ']' for array type.");
        //         // Error recovery: skip until RBRACKET or semicolon
        //     }
        //     // Append [] to name, or create a specific ArrayTypeReferenceNode
        // }

        return ast_create_type_reference_node(type_token, type_token->lexeme);
    }
    parser_error(parser, peek(parser), "Expected a type name.");
    return NULL;
}


// --- Placeholder Implementations for other parsing rules ---
// These will be implemented in detail in future steps

static FunctionDeclarationNode* parse_function_declaration(Parser* parser, Token* access_modifier_token, Token* static_modifier_token) {
    parser_error(parser, peek(parser), "Function declaration parsing not yet implemented.");
    synchronize(parser);
    return NULL;
}

static ConstructorDeclarationNode* parse_constructor_declaration(Parser* parser, IdentifierNode* class_name_node) {
    parser_error(parser, peek(parser), "Constructor declaration parsing not yet implemented.");
    synchronize(parser);
    return NULL;
}

static FieldDeclarationNode* parse_field_declaration(Parser* parser, Token* access_modifier_token, Token* static_modifier_token, Token* immutability_modifier_token) {
    parser_error(parser, peek(parser), "Field declaration parsing not yet implemented.");
    synchronize(parser);
    return NULL;
}

static EnumDeclarationNode* parse_enum_declaration(Parser* parser, Token* access_modifier_token) {
    parser_error(parser, peek(parser), "Enum declaration parsing not yet implemented.");
    synchronize(parser);
    return NULL;
}

static StatementNode* parse_statement(Parser* parser) {
    parser_error(parser, peek(parser), "Statement parsing not yet implemented.");
    synchronize(parser);
    return NULL;
}

static BlockStatementNode* parse_block_statement(Parser* parser) {
    parser_error(parser, peek(parser), "Block statement parsing not yet implemented.");
    synchronize(parser);
    return NULL;
}

static ExpressionStatementNode* parse_expression_statement(Parser* parser) {
    parser_error(parser, peek(parser), "Expression statement parsing not yet implemented.");
    synchronize(parser);
    return NULL;
}

static VariableDeclarationStatementNode* parse_variable_declaration_statement(Parser* parser, Token* modifier) {
    parser_error(parser, peek(parser), "Variable declaration statement parsing not yet implemented.");
    synchronize(parser);
    return NULL;
}

static IfStatementNode* parse_if_statement(Parser* parser) {
    parser_error(parser, peek(parser), "If statement parsing not yet implemented.");
    synchronize(parser);
    return NULL;
}

static WhileStatementNode* parse_while_statement(Parser* parser) {
    parser_error(parser, peek(parser), "While statement parsing not yet implemented.");
    synchronize(parser);
    return NULL;
}

static ForStatementNode* parse_for_statement(Parser* parser) {
    parser_error(parser, peek(parser), "For statement parsing not yet implemented.");
    synchronize(parser);
    return NULL;
}

static DoWhileStatementNode* parse_do_while_statement(Parser* parser) {
    parser_error(parser, peek(parser), "Do-While statement parsing not yet implemented.");
    synchronize(parser);
    return NULL;
}

static ForEachStatementNode* parse_foreach_statement(Parser* parser) {
    parser_error(parser, peek(parser), "For-Each statement parsing not yet implemented.");
    synchronize(parser);
    return NULL;
}

static JumpStatementNode* parse_jump_statement(Parser* parser, TokenType type) {
    parser_error(parser, peek(parser), "Jump statement parsing not yet implemented.");
    synchronize(parser);
    return NULL;
}

static ReturnStatementNode* parse_return_statement(Parser* parser) {
    parser_error(parser, peek(parser), "Return statement parsing not yet implemented.");
    synchronize(parser);
    return NULL;
}

static ThrowStatementNode* parse_throw_statement(Parser* parser) {
    parser_error(parser, peek(parser), "Throw statement parsing not yet implemented.");
    synchronize(parser);
    return NULL;
}

static TryCatchFinallyStatementNode* parse_try_catch_finally_statement(Parser* parser) {
    parser_error(parser, peek(parser), "Try-Catch-Finally statement parsing not yet implemented.");
    synchronize(parser);
    return NULL;
}

static ExpressionNode* parse_expression(Parser* parser) {
    // This will eventually call the lowest precedence expression parser.
    return parse_assignment_expression(parser); // Start with lowest precedence
}

static ExpressionNode* parse_assignment_expression(Parser* parser) {
    // This will eventually parse expressions like 'a = b', 'x += y'
    parser_error(parser, peek(parser), "Assignment expression parsing not yet implemented.");
    synchronize(parser);
    return NULL;
}

static ExpressionNode* parse_ternary_expression(Parser* parser) {
    parser_error(parser, peek(parser), "Ternary expression parsing not yet implemented.");
    synchronize(parser);
    return NULL;
}

static ExpressionNode* parse_logical_or_expression(Parser* parser) {
    parser_error(parser, peek(parser), "Logical OR expression parsing not yet implemented.");
    synchronize(parser);
    return NULL;
}

static ExpressionNode* parse_logical_and_expression(Parser* parser) {
    parser_error(parser, peek(parser), "Logical AND expression parsing not yet implemented.");
    synchronize(parser);
    return NULL;
}

static ExpressionNode* parse_bitwise_or_expression(Parser* parser) {
    parser_error(parser, peek(parser), "Bitwise OR expression parsing not yet implemented.");
    synchronize(parser);
    return NULL;
}

static ExpressionNode* parse_bitwise_xor_expression(Parser* parser) {
    parser_error(parser, peek(parser), "Bitwise XOR expression parsing not yet implemented.");
    synchronize(parser);
    return NULL;
}

static ExpressionNode* parse_bitwise_and_expression(Parser* parser) {
    parser_error(parser, peek(parser), "Bitwise AND expression parsing not yet implemented.");
    synchronize(parser);
    return NULL;
}

static ExpressionNode* parse_equality_expression(Parser* parser) {
    parser_error(parser, peek(parser), "Equality expression parsing not yet implemented.");
    synchronize(parser);
    return NULL;
}

static ExpressionNode* parse_comparison_expression(Parser* parser) {
    parser_error(parser, peek(parser), "Comparison expression parsing not yet implemented.");
    synchronize(parser);
    return NULL;
}

static ExpressionNode* parse_shift_expression(Parser* parser) {
    parser_error(parser, peek(parser), "Shift expression parsing not yet implemented.");
    synchronize(parser);
    return NULL;
}

static ExpressionNode* parse_additive_expression(Parser* parser) {
    parser_error(parser, peek(parser), "Additive expression parsing not yet implemented.");
    synchronize(parser);
    return NULL;
}

static ExpressionNode* parse_multiplicative_expression(Parser* parser) {
    parser_error(parser, peek(parser), "Multiplicative expression parsing not yet implemented.");
    synchronize(parser);
    return NULL;
}

static ExpressionNode* parse_unary_expression(Parser* parser) {
    parser_error(parser, peek(parser), "Unary expression parsing not yet implemented.");
    synchronize(parser);
    return NULL;
}

static ExpressionNode* parse_call_member_array_expression(Parser* parser) {
    parser_error(parser, peek(parser), "Call, member, array access expression parsing not yet implemented.");
    synchronize(parser);
    return NULL;
}

static ExpressionNode* parse_primary_expression(Parser* parser) {
    // Handles literals, identifiers, parentheses expressions
    parser_error(parser, peek(parser), "Primary expression parsing not yet implemented.");
    synchronize(parser);
    return NULL;
}

static ParameterNode* parse_parameter(Parser* parser) {
    parser_error(parser, peek(parser), "Parameter parsing not yet implemented.");
    synchronize(parser);
    return NULL;
}

