#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>

typedef enum {
    TOKEN_IDENTIFIER,
    TOKEN_KEYWORD,
    TOKEN_NUMBER,    // Can be int or float
    TOKEN_STRING,
    TOKEN_BOOL,      // true, false
    TOKEN_OPERATOR,  // +, -, *, /, =, ==, !=, <, >, <=, >=, &&, ||, !, etc.
    TOKEN_SYMBOL,    // (, ), {, }, [, ], ;, ,, .
    TOKEN_EOF,
    TOKEN_UNKNOWN    // For unrecognized characters, helps parser skip
} TokenType;

typedef struct Token {
    TokenType type;
    char text[256]; // Increased buffer size for longer identifiers/strings
    int line;
    int col;
} Token;

// If lexing from a file stream
Token next_token_from_file(FILE *file, int *line, int *col); // Example if needed

// Main lexing function from a source string
Token* lex(const char* source);

#endif // LEXER_H
