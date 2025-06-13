#ifndef PARSER_H
#define PARSER_H

#include <stdio.h>
#include "lexer.h"
#include "ast_types.h"

// Functions
// ASTNode* parse_program(FILE *file); // If reading directly from file stream
ASTNode* parse(Token *tokens); // Takes array of tokens

// Expose global program AST root if other modules need it AFTER parsing
extern ASTNode *program; 

#endif // PARSER_H
