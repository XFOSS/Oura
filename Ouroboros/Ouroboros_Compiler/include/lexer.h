// lexer.h
// Defines the Lexer structure and declares public functions for lexical analysis.

#ifndef OUROBOROS_LEXER_H
#define OUROBOROS_LEXER_H

#include <stdio.h>    // For FILE*
#include "ouroboros/token.h" // For Token and TokenType definitions

// The Lexer structure holds the state of the lexer.
typedef struct {
    const char* source_code; // The input source code string
    int source_length;       // Total length of the source code
    int current_pos;         // Current reading position in the source code
    int start_pos;           // Starting position of the current token being scanned
    int current_line;        // Current line number
    int current_column;      // Current column number
    const char* source_name; // Name of the source file being lexed
} Lexer;

// Function to initialize a new Lexer instance.
// @param source_code The entire source code string to lex. This string should persist
//                    for the lifetime of the lexer.
// @param source_name A descriptive name for the source (e.g., file path).
// @return A dynamically allocated Lexer pointer, or NULL on allocation failure.
Lexer* lexer_init(const char* source_code, const char* source_name);

// Function to scan and return the next token from the source code.
// Responsibility: Caller must free the returned Token* using free_token().
// @param lexer A pointer to the Lexer instance.
// @return A dynamically allocated Token pointer, or NULL if an unrecoverable error occurs.
Token* lexer_scan_token(Lexer* lexer);

// Function to free the memory associated with a Lexer instance.
// Note: This does NOT free the source_code string, as it is assumed to be owned externally.
// @param lexer A pointer to the Lexer instance to free.
void lexer_free(Lexer* lexer);

#endif // OUROBOROS_LEXER_H
