// token.h
// Defines the TokenType enumeration and the Token structure for the Ouroboros Lexer.

#ifndef OUROBOROS_TOKEN_H
#define OUROBOROS_TOKEN_H

// Required for common C types like size_t for string lengths, if needed
#include <stdio.h> // For FILE and basic I/O (though not strictly for token defs)
#include <stdlib.h> // For malloc, free
#include <string.h> // For strlen, strcpy

// Enum defining all possible types of tokens in Ouroboros.
// This mirrors the comprehensive TokenType enum we defined in the Java Token.java.
typedef enum {
    // --- Single-character delimiters ---
    LPARENT, RPARENT, LBRACE, RBRACE, LBRACKET, RBRACKET,
    COMMA, SEMICOLON, COLON, PERIOD, QUESTION_MARK,

    // --- Operators (single character) ---
    PLUS, MINUS, MULTIPLY, DIVIDE, MODULO, BIT_NOT, BIT_AND, BIT_OR, BIT_XOR, NOT,
    ASSIGN, LT, GT,

    // --- Operators (two or three characters) ---
    PLUS_EQUALS, MINUS_EQUALS, MULTIPLY_EQUALS, DIVIDE_EQUALS, MODULO_EQUALS,
    INCREMENT, DECREMENT, // ++, --

    EQ, NE, LE, GE, // ==, !=, <=, >=
    AND, OR, // &&, ||
    BIT_AND_EQUALS, BIT_OR_EQUALS, BIT_XOR_EQUALS, // &=, |=, ^=
    LEFT_SHIFT, RIGHT_SHIFT, LEFT_SHIFT_EQUALS, RIGHT_SHIFT_EQUALS, // <<, >>, <<=, >>=
    UNSIGNED_RIGHT_SHIFT, UNSIGNED_RIGHT_SHIFT_EQUALS, // >>>, >>>=

    RANGE, EXCLUSIVE_RANGE, // ..., ..<

    // --- Keywords (control flow, types, modifiers, object-oriented) ---
    IF, ELSE, WHILE, FOR, DO,
    RETURN, BREAK, CONTINUE,
    CLASS, INTERFACE, EXTENDS, IMPLEMENTS,
    THIS, SUPER, NEW, STATIC,
    PUBLIC, PRIVATE, PROTECTED, INTERNAL,
    VAR, LET, CONST, FINAL,
    TRY, CATCH, FINALLY, THROW, THROWS,
    PACKAGE, IMPORT,
    REF, OUT, // Parameter passing modifiers
    VOID,
    FOREACH,
    ENUM,

    // --- Primitive Types (Explicit Keywords, distinct from literals) ---
    CHAR_TYPE, SHORT_TYPE, INT_TYPE, LONG_TYPE, FLOAT_TYPE, DOUBLE_TYPE,
    BOOLEAN_TYPE, STRING_TYPE,

    // --- Literals ---
    IDENTIFIER,
    INTEGER_LITERAL,
    FLOATING_POINT_LITERAL,
    STRING_LITERAL,
    CHARACTER_LITERAL,
    BOOLEAN_LITERAL, // true, false
    NULL_LITERAL, // null

    // --- Special ---
    EOF_TOKEN, // Renamed to avoid conflict with standard EOF macro
    ERROR_TOKEN // Renamed to avoid conflict with potential macros
} TokenType;

// A union to hold different types of literal values.
// In C, you typically need to manage memory for strings manually.
typedef union {
    long long integer_val;     // For INTEGER_LITERAL
    double float_val;          // For FLOATING_POINT_LITERAL
    char char_val;             // For CHARACTER_LITERAL
    int boolean_val;           // For BOOLEAN_LITERAL (0 for false, 1 for true)
    char* string_val;          // For STRING_LITERAL (dynamically allocated)
    // NULL_LITERAL doesn't need a value, its presence as a token type is enough
} LiteralValue;

// Represents a single token identified by the lexer.
typedef struct {
    TokenType type;
    char* lexeme;       // The raw text matched by the token (dynamically allocated)
    LiteralValue literal; // The parsed literal value, if applicable
    int line;           // Line number where the token was found
    int column;         // Column number where the token starts
    char* source_name;  // Name of the source file (could be a const char*)
    char* error_message; // Specific error message for ERROR_TOKEN (dynamically allocated)
    int has_literal;    // Flag to indicate if the literal union is active
} Token;

// Function to create a new token (forward declaration)
// char* lexeme should be duplicated inside this function if it's a temporary buffer
Token* create_token(TokenType type, const char* lexeme, int line, int column, const char* source_name);

// Function to create a new token with a literal value (forward declaration)
Token* create_token_with_literal(TokenType type, const char* lexeme, LiteralValue literal, int line, int column, const char* source_name);

// Function to create an error token (forward declaration)
Token* create_error_token(const char* lexeme, const char* error_message, int line, int column, const char* source_name);

// Function to free memory associated with a token (forward declaration)
void free_token(Token* token);

#endif // OUROBOROS_TOKEN_H
