// main.c
// Main entry point for the Ouroboros compiler.
// This program currently implements the lexical analysis phase.

#include <stdio.h>  // For printf, fprintf, FILE*, fopen, fread, fclose
#include <stdlib.h> // For EXIT_SUCCESS, EXIT_FAILURE, malloc, free
#include <string.h> // For strlen, strerror
#include <errno.h>  // For errno

#include "ouroboros/lexer.h" // Include the Lexer header
#include "ouroboros/token.h" // Include the Token header

// Function to read the entire content of a file into a dynamically allocated string.
// @param file_path The path to the file.
// @param out_buffer A pointer to a char* that will store the allocated string.
// @param out_length A pointer to an int that will store the length of the string.
// @return 0 on success, -1 on failure.
static int read_file_to_string(const char* file_path, char** out_buffer, int* out_length) {
    FILE* file = fopen(file_path, "rb"); // Open in binary mode for cross-platform
    if (file == NULL) {
        fprintf(stderr, "Error: Could not open file '%s': %s\n", file_path, strerror(errno));
        return -1;
    }

    // Determine file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file); // Go back to the beginning of the file

    if (file_size == -1) {
        fprintf(stderr, "Error: Could not determine file size for '%s': %s\n", file_path, strerror(errno));
        fclose(file);
        return -1;
    }

    // Allocate buffer for file content + null terminator
    *out_buffer = (char*)malloc(file_size + 1);
    if (*out_buffer == NULL) {
        fprintf(stderr, "Error: Memory allocation failed for file buffer.\n");
        fclose(file);
        return -1;
    }

    // Read file into buffer
    size_t bytes_read = fread(*out_buffer, 1, file_size, file);
    if (bytes_read != (size_t)file_size) {
        fprintf(stderr, "Error: Failed to read entire file '%s'. Read %zu bytes, expected %ld.\n", file_path, bytes_read, file_size);
        free(*out_buffer);
        *out_buffer = NULL;
        fclose(file);
        return -1;
    }

    (*out_buffer)[bytes_read] = '\0'; // Null-terminate the string
    *out_length = (int)bytes_read;

    fclose(file);
    return 0; // Success
}

// Helper function to print token information.
static void print_token(Token* token) {
    // A mapping from TokenType to string for better output.
    // This could be made into a separate function in token.c if desired for reuse.
    const char* token_type_to_string(TokenType type) {
        switch (type) {
            case LPARENT: return "LPARENT"; case RPARENT: return "RPARENT";
            case LBRACE: return "LBRACE"; case RBRACE: return "RBRACE";
            case LBRACKET: return "LBRACKET"; case RBRACKET: return "RBRACKET";
            case COMMA: return "COMMA"; case SEMICOLON: return "SEMICOLON";
            case COLON: return "COLON"; case PERIOD: return "PERIOD";
            case QUESTION_MARK: return "QUESTION_MARK";
            case PLUS: return "PLUS"; case MINUS: return "MINUS"; case MULTIPLY: return "MULTIPLY";
            case DIVIDE: return "DIVIDE"; case MODULO: return "MODULO"; case BIT_NOT: return "BIT_NOT";
            case BIT_AND: return "BIT_AND"; case BIT_OR: return "BIT_OR"; case BIT_XOR: return "BIT_XOR";
            case NOT: return "NOT"; case ASSIGN: return "ASSIGN"; case LT: return "LT"; case GT: return "GT";
            case PLUS_EQUALS: return "PLUS_EQUALS"; case MINUS_EQUALS: return "MINUS_EQUALS";
            case MULTIPLY_EQUALS: return "MULTIPLY_EQUALS"; case DIVIDE_EQUALS: return "DIVIDE_EQUALS";
            case MODULO_EQUALS: return "MODULO_EQUALS"; case INCREMENT: return "INCREMENT";
            case DECREMENT: return "DECREMENT"; case EQ: return "EQ"; case NE: return "NE";
            case LE: return "LE"; case GE: return "GE"; case AND: return "AND"; case OR: return "OR";
            case BIT_AND_EQUALS: return "BIT_AND_EQUALS"; case BIT_OR_EQUALS: return "BIT_OR_EQUALS";
            case BIT_XOR_EQUALS: return "BIT_XOR_EQUALS"; case LEFT_SHIFT: return "LEFT_SHIFT";
            case RIGHT_SHIFT: return "RIGHT_SHIFT"; case LEFT_SHIFT_EQUALS: return "LEFT_SHIFT_EQUALS";
            case RIGHT_SHIFT_EQUALS: return "RIGHT_SHIFT_EQUALS"; case UNSIGNED_RIGHT_SHIFT: return "UNSIGNED_RIGHT_SHIFT";
            case UNSIGNED_RIGHT_SHIFT_EQUALS: return "UNSIGNED_RIGHT_SHIFT_EQUALS";
            case RANGE: return "RANGE"; case EXCLUSIVE_RANGE: return "EXCLUSIVE_RANGE";
            case IF: return "IF"; case ELSE: return "ELSE"; case WHILE: return "WHILE";
            case FOR: return "FOR"; case DO: return "DO"; case RETURN: return "RETURN";
            case BREAK: return "BREAK"; case CONTINUE: return "CONTINUE"; case CLASS: return "CLASS";
            case INTERFACE: return "INTERFACE"; case EXTENDS: return "EXTENDS"; case IMPLEMENTS: return "IMPLEMENTS";
            case THIS: return "THIS"; case SUPER: return "SUPER"; case NEW: return "NEW";
            case STATIC: return "STATIC"; case PUBLIC: return "PUBLIC"; case PRIVATE: return "PRIVATE";
            case PROTECTED: return "PROTECTED"; case INTERNAL: return "INTERNAL"; case VAR: return "VAR";
            case LET: return "LET"; case CONST: return "CONST"; case FINAL: return "FINAL";
            case TRY: return "TRY"; case CATCH: return "CATCH"; case FINALLY: return "FINALLY";
            case THROW: return "THROW"; case THROWS: return "THROWS"; case PACKAGE: return "PACKAGE";
            case IMPORT: return "IMPORT"; case REF: return "REF"; case OUT: return "OUT";
            case VOID: return "VOID"; case FOREACH: return "FOREACH"; case ENUM: return "ENUM";
            case CHAR_TYPE: return "CHAR_TYPE"; case SHORT_TYPE: return "SHORT_TYPE";
            case INT_TYPE: return "INT_TYPE"; case LONG_TYPE: return "LONG_TYPE";
            case FLOAT_TYPE: return "FLOAT_TYPE"; case DOUBLE_TYPE: return "DOUBLE_TYPE";
            case BOOLEAN_TYPE: return "BOOLEAN_TYPE"; case STRING_TYPE: return "STRING_TYPE";
            case IDENTIFIER: return "IDENTIFIER";
            case INTEGER_LITERAL: return "INTEGER_LITERAL";
            case FLOATING_POINT_LITERAL: return "FLOATING_POINT_LITERAL";
            case STRING_LITERAL: return "STRING_LITERAL";
            case CHARACTER_LITERAL: return "CHARACTER_LITERAL";
            case BOOLEAN_LITERAL: return "BOOLEAN_LITERAL"; case NULL_LITERAL: return "NULL_LITERAL";
            case EOF_TOKEN: return "EOF_TOKEN"; case ERROR_TOKEN: return "ERROR_TOKEN";
            default: return "UNKNOWN_TOKEN_TYPE";
        }
    }

    printf("Type: %s (Lexeme: '%s') at %s Line: %d, Column: %d",
           token_type_to_string(token->type),
           token->lexeme ? token->lexeme : "N/A", // Handle NULL lexeme
           token->source_name ? token->source_name : "UnknownSource",
           token->line,
           token->column);

    if (token->has_literal) {
        printf(" [Literal: ");
        switch (token->type) {
            case INTEGER_LITERAL: printf("Int(%lld)", token->literal.integer_val); break;
            case FLOATING_POINT_LITERAL: printf("Float(%f)", token->literal.float_val); break;
            case CHARACTER_LITERAL: printf("Char('%c')", token->literal.char_val); break;
            case BOOLEAN_LITERAL: printf("Bool(%s)", token->literal.boolean_val ? "true" : "false"); break;
            case STRING_LITERAL: printf("String(\"%s\")", token->literal.string_val ? token->literal.string_val : ""); break;
            default: printf("N/A"); break;
        }
        printf("]");
    }

    if (token->error_message) {
        printf(" [Error: %s]", token->error_message);
    }
    printf("\n");
}


int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <ouroboros_source_file.ouro>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char* file_path = argv[1];
    char* source_code = NULL;
    int source_length = 0;

    // Read the source file
    if (read_file_to_string(file_path, &source_code, &source_length) != 0) {
        return EXIT_FAILURE; // Error already printed by read_file_to_string
    }

    printf("--- Source Code Loaded (%s) ---\n%s\n--------------------------\n\n", file_path, source_code);
    printf("--- Starting Lexical Analysis ---\n");

    // Initialize the lexer
    Lexer* lexer = lexer_init(source_code, file_path);
    if (lexer == NULL) {
        free(source_code);
        return EXIT_FAILURE;
    }

    // Scan and print tokens
    Token* token = NULL;
    do {
        token = lexer_scan_token(lexer);
        if (token == NULL) {
            fprintf(stderr, "Error: Lexer returned NULL token (unrecoverable error).\n");
            break; // Exit loop on critical error
        }
        print_token(token);

        if (token->type == ERROR_TOKEN) {
            // For now, stop on first lexical error. A real compiler might try to recover.
            free_token(token);
            break;
        }
        
        // Break if it's the end of file token
        if (token->type == EOF_TOKEN) {
            free_token(token);
            break;
        }

        free_token(token); // Free the token after printing, as lexer_scan_token returns a new one each time.

    } while (1); // Loop indefinitely until break condition (EOF or ERROR)

    printf("\n--- Lexical Analysis Complete ---\n");

    // Cleanup
    lexer_free(lexer);
    free(source_code);

    return EXIT_SUCCESS;
}
