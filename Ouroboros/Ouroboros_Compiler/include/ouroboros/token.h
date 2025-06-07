#ifndef OUROBOROS_TOKEN_H
#define OUROBOROS_TOKEN_H

#include <stddef.h>

// Token types
typedef enum {
    // Keywords
    TOKEN_PACKAGE,
    TOKEN_IMPORT,
    TOKEN_CLASS,
    TOKEN_INTERFACE,
    TOKEN_FUNCTION,
    TOKEN_VAR,
    TOKEN_CONST,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_WHILE,
    TOKEN_FOR,
    TOKEN_RETURN,
    TOKEN_BREAK,
    TOKEN_CONTINUE,
    TOKEN_NEW,
    TOKEN_THIS,
    TOKEN_SUPER,
    TOKEN_NULL,
    TOKEN_TRUE,
    TOKEN_FALSE,

    // Operators
    TOKEN_PLUS,           // +
    TOKEN_MINUS,          // -
    TOKEN_STAR,           // *
    TOKEN_SLASH,          // /
    TOKEN_PERCENT,        // %
    TOKEN_EQUAL,          // =
    TOKEN_EQUAL_EQUAL,    // ==
    TOKEN_BANG,           // !
    TOKEN_BANG_EQUAL,     // !=
    TOKEN_LESS,           // <
    TOKEN_LESS_EQUAL,     // <=
    TOKEN_GREATER,        // >
    TOKEN_GREATER_EQUAL,  // >=
    TOKEN_AND,            // &&
    TOKEN_OR,             // ||
    TOKEN_DOT,            // .
    TOKEN_COMMA,          // ,
    TOKEN_SEMICOLON,      // ;
    TOKEN_COLON,          // :
    TOKEN_QUESTION,       // ?
    TOKEN_ARROW,          // ->
    TOKEN_INCREMENT,      // ++
    TOKEN_DECREMENT,      // --

    // Brackets
    TOKEN_LEFT_PAREN,     // (
    TOKEN_RIGHT_PAREN,    // )
    TOKEN_LEFT_BRACE,     // {
    TOKEN_RIGHT_BRACE,    // }
    TOKEN_LEFT_BRACKET,   // [
    TOKEN_RIGHT_BRACKET,  // ]

    // Literals
    TOKEN_IDENTIFIER,
    TOKEN_STRING,
    TOKEN_NUMBER,
    TOKEN_CHARACTER,

    // Special
    TOKEN_ERROR,
    TOKEN_EOF
} TokenType;

// Token structure
typedef struct {
    TokenType type;
    const char* lexeme;
    size_t length;
    size_t line;
    size_t column;
} Token;

// Token functions
const char* token_type_to_string(TokenType type);
Token* token_create(TokenType type, const char* lexeme, size_t length, size_t line, size_t column);
void token_free(Token* token);

#endif // OUROBOROS_TOKEN_H 