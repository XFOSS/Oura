// parser.h
// Defines the Parser structure and declares public functions for syntactic analysis.

#ifndef OUROBOROS_PARSER_H
#define OUROBOROS_PARSER_H

#include "ouroboros/token.h" // For Token and TokenType definitions
#include <stdlib.h> // For size_t

// Forward declaration for ASTNode (full definition will be in ast.h later)
typedef struct ASTNode ASTNode;

// The Parser structure holds the state of the parser.
typedef struct {
    Token** tokens;       // Array of tokens from the lexer
    size_t token_count;   // Total number of tokens
    int current_token_idx; // Index of the current token being processed
    const char* source_name; // Name of the source file (from lexer)
    // Add fields for error reporting, symbol table, etc., as needed later
} Parser;

/**
 * Initializes a new Parser instance.
 * @param tokens A dynamically allocated array of Token pointers from the lexer.
 * The parser takes ownership of this array and its contents.
 * @param token_count The number of tokens in the array.
 * @param source_name The name of the source file.
 * @return A dynamically allocated Parser pointer, or NULL on allocation failure.
 */
Parser* parser_init(Token** tokens, size_t token_count, const char* source_name);

/**
 * Parses the stream of tokens and constructs the Abstract Syntax Tree (AST).
 * This will be the main entry point for the parsing process.
 * @param parser A pointer to the Parser instance.
 * @return The root ASTNode of the parsed program, or NULL if parsing fails.
 * The caller is responsible for freeing the AST.
 */
ASTNode* parser_parse(Parser* parser);

/**
 * Frees the memory associated with a Parser instance and the tokens it owns.
 * Note: This function will free the tokens array itself, and each individual Token*
 * within it using free_token().
 * @param parser A pointer to the Parser instance to free.
 */
void parser_free(Parser* parser);

#endif // OUROBOROS_PARSER_H
