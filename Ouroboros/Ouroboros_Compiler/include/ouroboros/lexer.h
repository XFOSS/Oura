#ifndef OUROBOROS_LEXER_H
#define OUROBOROS_LEXER_H

#include "ouroboros/token.h"
#include <stddef.h>

// Lexer structure
typedef struct {
    const char* source;
    size_t start;
    size_t current;
    size_t line;
    size_t column;
} Lexer;

// Lexer functions
Lexer* lexer_init(const char* source);
void lexer_free(Lexer* lexer);
Token* lexer_scan_token(Lexer* lexer);
bool lexer_is_at_end(Lexer* lexer);

#endif // OUROBOROS_LEXER_H 