// token.c
// Implements functions for the Token structure and operations.

#include "ouroboros/token.h"
#include <stdlib.h> // For malloc, free
#include <string.h> // For strdup

/**
 * Creates a new Token instance.
 * @param type The TokenType of the token.
 * @param lexeme The string representation of the token as it appeared in source.
 * @param literal A union containing the literal value if the token is a literal (e.g., integer, string).
 * @param line The line number where the token starts.
 * @param column The column number where the token starts.
 * @param source_name The name of the source file.
 * @return A dynamically allocated Token pointer. The caller is responsible for freeing it.
 */
Token* create_token(TokenType type, const char* lexeme, LiteralValue literal,
                    int line, int column, const char* source_name) {
    Token* token = (Token*)malloc(sizeof(Token));
    if (token == NULL) {
        fprintf(stderr, "Error: Memory allocation failed for Token.\n");
        exit(EXIT_FAILURE); // Fatal error for now
    }
    token->type = type;
    token->lexeme = strdup(lexeme); // Duplicate the lexeme string
    if (token->lexeme == NULL) {
        fprintf(stderr, "Error: Memory allocation failed for token lexeme.\n");
        free(token);
        exit(EXIT_FAILURE);
    }

    // Copy literal value based on type
    if (type == STRING_LITERAL && literal.string_val != NULL) {
        token->literal.string_val = strdup(literal.string_val);
        if (token->literal.string_val == NULL) {
            fprintf(stderr, "Error: Memory allocation failed for string literal.\n");
            free(token->lexeme);
            free(token);
            exit(EXIT_FAILURE);
        }
    } else {
        token->literal = literal; // For other literal types, direct copy is fine
    }

    token->line = line;
    token->column = column;
    token->source_name = source_name; // Reference to source name (not owned by token)
    token->error_message = NULL; // Initially no error
    return token;
}

/**
 * Creates a new Token instance for an ERROR token.
 * @param type The TokenType, should be ERROR.
 * @param lexeme The lexeme that caused the error.
 * @param error_message The specific error message.
 * @param line The line number.
 * @param column The column number.
 * @param source_name The source file name.
 * @return A dynamically allocated Token pointer.
 */
Token* create_error_token(const char* lexeme, const char* error_message,
                          int line, int column, const char* source_name) {
    Token* token = (Token*)malloc(sizeof(Token));
    if (token == NULL) {
        fprintf(stderr, "Error: Memory allocation failed for Error Token.\n");
        exit(EXIT_FAILURE);
    }
    token->type = ERROR_TOKEN;
    token->lexeme = strdup(lexeme);
    if (token->lexeme == NULL) {
        fprintf(stderr, "Error: Memory allocation failed for error token lexeme.\n");
        free(token);
        exit(EXIT_FAILURE);
    }
    token->literal.integer_val = 0; // Default or irrelevant for error
    token->line = line;
    token->column = column;
    token->source_name = source_name;
    token->error_message = strdup(error_message);
    if (token->error_message == NULL) {
        fprintf(stderr, "Error: Memory allocation failed for error message.\n");
        free(token->lexeme);
        free(token);
        exit(EXIT_FAILURE);
    }
    return token;
}


/**
 * Frees the memory associated with a Token instance.
 * @param token The Token pointer to free.
 */
void free_token(Token* token) {
    if (token) {
        free(token->lexeme);
        if (token->type == STRING_LITERAL && token->literal.string_val != NULL) {
            free(token->literal.string_val);
        }
        if (token->error_message) {
            free(token->error_message);
        }
        free(token);
    }
}

/**
 * Returns a human-readable string representation of the TokenType.
 * @param type The TokenType.
 * @return A static string literal for the token type.
 */
const char* token_type_to_string(TokenType type) {
    switch (type) {
        // Single-character delimiters
        case LPARENT: return "LPARENT";
        case RPARENT: return "RPARENT";
        case LBRACE: return "LBRACE";
        case RBRACE: return "RBRACE";
        case LBRACKET: return "LBRACKET";
        case RBRACKET: return "RBRACKET";
        case COMMA: return "COMMA";
        case SEMICOLON: return "SEMICOLON";
        case COLON: return "COLON";
        case PERIOD: return "PERIOD";

        // Operators (single character)
        case PLUS: return "PLUS";
        case MINUS: return "MINUS";
        case MULTIPLY: return "MULTIPLY";
        case DIVIDE: return "DIVIDE";
        case MODULO: return "MODULO";
        case BIT_NOT: return "BIT_NOT";
        case BIT_AND: return "BIT_AND";
        case BIT_OR: return "BIT_OR";
        case BIT_XOR: return "BIT_XOR";
        case NOT: return "NOT";
        case ASSIGN: return "ASSIGN";
        case LT: return "LT";
        case GT: return "GT";
        case QUESTION_MARK: return "QUESTION_MARK";

        // Operators (two or three characters)
        case PLUS_EQUALS: return "PLUS_EQUALS";
        case MINUS_EQUALS: return "MINUS_EQUALS";
        case MULTIPLY_EQUALS: return "MULTIPLY_EQUALS";
        case DIVIDE_EQUALS: return "DIVIDE_EQUALS";
        case MODULO_EQUALS: return "MODULO_EQUALS";
        case INCREMENT: return "INCREMENT";
        case DECREMENT: return "DECREMENT";
        case EQ: return "EQ";
        case NE: return "NE";
        case LE: return "LE";
        case GE: return "GE";
        case AND: return "AND";
        case OR: return "OR";
        case BIT_AND_EQUALS: return "BIT_AND_EQUALS";
        case BIT_OR_EQUALS: return "BIT_OR_EQUALS";
        case BIT_XOR_EQUALS: return "BIT_XOR_EQUALS";
        case LEFT_SHIFT: return "LEFT_SHIFT";
        case RIGHT_SHIFT: return "RIGHT_SHIFT";
        case LEFT_SHIFT_EQUALS: return "LEFT_SHIFT_EQUALS";
        case RIGHT_SHIFT_EQUALS: return "RIGHT_SHIFT_EQUALS";
        case UNSIGNED_RIGHT_SHIFT: return "UNSIGNED_RIGHT_SHIFT";
        case UNSIGNED_RIGHT_SHIFT_EQUALS: return "UNSIGNED_RIGHT_SHIFT_EQUALS";
        case RANGE: return "RANGE";
        case EXCLUSIVE_RANGE: return "EXCLUSIVE_RANGE";
        case OPTIONAL_EQUALS: return "OPTIONAL_EQUALS";
        case ARROW: return "ARROW"; // ->
        case FAT_ARROW: return "FAT_ARROW"; // =>

        // Keywords
        case IF: return "IF";
        case ELSE: return "ELSE";
        case WHILE: return "WHILE";
        case FOR: return "FOR";
        case RETURN: return "RETURN";
        case BREAK: return "BREAK";
        case CONTINUE: return "CONTINUE";
        case CLASS: return "CLASS";
        case INTERFACE: return "INTERFACE";
        case ENUM: return "ENUM";
        case VOID: return "VOID";
        case VAR: return "VAR";
        case LET: return "LET";
        case CONST: return "CONST";
        case FINAL: return "FINAL";
        case DO: return "DO";
        case SWITCH: return "SWITCH";
        case CASE: return "CASE";
        case DEFAULT: return "DEFAULT";
        case TRY: return "TRY";
        case CATCH: return "CATCH";
        case FINALLY: return "FINALLY";
        case THROW: return "THROW";
        case NEW: return "NEW";
        case THIS: return "THIS";
        case SUPER: return "SUPER";
        case STATIC: return "STATIC";
        case PUBLIC: return "PUBLIC";
        case PRIVATE: return "PRIVATE";
        case PROTECTED: return "PROTECTED";
        case INTERNAL: return "INTERNAL";
        case IMPORT: return "IMPORT";
        case PACKAGE: return "PACKAGE";
        case EXTENDS: return "EXTENDS";
        case IMPLEMENTS: return "IMPLEMENTS";
        case CHAR_TYPE: return "CHAR_TYPE";
        case SHORT_TYPE: return "SHORT_TYPE";
        case INT_TYPE: return "INT_TYPE";
        case LONG_TYPE: return "LONG_TYPE";
        case FLOAT_TYPE: return "FLOAT_TYPE";
        case DOUBLE_TYPE: return "DOUBLE_TYPE";
        case BOOLEAN_TYPE: return "BOOLEAN_TYPE";
        case STRING_TYPE: return "STRING_TYPE";
        case FOREACH: return "FOREACH";
        case IN: return "IN";


        // Literals
        case IDENTIFIER: return "IDENTIFIER";
        case INTEGER_LITERAL: return "INTEGER_LITERAL";
        case FLOATING_POINT_LITERAL: return "FLOATING_POINT_LITERAL";
        case STRING_LITERAL: return "STRING_LITERAL";
        case CHARACTER_LITERAL: return "CHARACTER_LITERAL";
        case BOOLEAN_LITERAL: return "BOOLEAN_LITERAL";
        case NULL_LITERAL: return "NULL_LITERAL";

        // End of File
        case EOF_TOKEN: return "EOF_TOKEN";

        // Error Token
        case ERROR_TOKEN: return "ERROR_TOKEN";

        default: return "UNKNOWN_TOKEN_TYPE";
    }
}

/**
 * Prints a token's details to the console.
 * @param token The Token pointer to print.
 */
void print_token(const Token* token) {
    if (!token) {
        printf("NULL Token\n");
        return;
    }
    printf("Token: %-20s Lexeme: '%-15s' Line: %-4d Col: %-4d Source: %s",
           token_type_to_string(token->type),
           token->lexeme,
           token->line,
           token->column,
           token->source_name ? token->source_name : "N/A");

    if (token->type == INTEGER_LITERAL) {
        printf(" Literal: %lld", token->literal.integer_val);
    } else if (token->type == FLOATING_POINT_LITERAL) {
        printf(" Literal: %f", token->literal.float_val);
    } else if (token->type == STRING_LITERAL) {
        printf(" Literal: \"%s\"", token->literal.string_val);
    } else if (token->type == CHARACTER_LITERAL) {
        printf(" Literal: '%c'", token->literal.char_val);
    } else if (token->type == BOOLEAN_LITERAL) {
        printf(" Literal: %s", token->literal.boolean_val ? "true" : "false");
    } else if (token->type == ERROR_TOKEN) {
        printf(" Error: %s", token->error_message ? token->error_message : "Unknown error");
    }
    printf("\n");
}