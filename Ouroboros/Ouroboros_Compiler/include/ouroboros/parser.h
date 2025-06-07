#ifndef OUROBOROS_PARSER_H
#define OUROBOROS_PARSER_H

#include "ouroboros/ast.h"
#include "ouroboros/lexer.h"
#include <stdbool.h>

// Parser structure
typedef struct {
    Lexer* lexer;
    Token* current;
    Token* previous;
    bool had_error;
} Parser;

// Parser functions
Parser* parser_init(Lexer* lexer);
void parser_free(Parser* parser);
ASTNode* parser_parse(Parser* parser);
bool parser_had_error(Parser* parser);

#endif // OUROBOROS_PARSER_H 