#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include "lexer.h"

// --- Globals for string lexing ---
static const char* current_source_string = NULL;
static int current_string_pos = 0;
static int current_line_lex = 1;
static int current_col_lex = 1;
// ---

static int string_getc_lex() {
    if (!current_source_string || current_source_string[current_string_pos] == '\0')
        return EOF;
    return current_source_string[current_string_pos++];
}

static void string_ungetc_lex() {
    if (current_string_pos > 0)
        current_string_pos--;
}

static int string_peek_lex() {
    if (!current_source_string || current_source_string[current_string_pos] == '\0')
        return EOF;
    return current_source_string[current_string_pos];
}

static void skip_whitespace_and_comments_string() {
    int c;
    while ((c = string_getc_lex()) != EOF) {
        if (c == ' ' || c == '\t' || c == '\r') { // Added \r
            current_col_lex++;
        } else if (c == '\n') {
            current_line_lex++;
            current_col_lex = 1;
        } else if (c == '/' && string_peek_lex() == '/') { // Single-line comment
            current_col_lex += 2;
            while ((c = string_getc_lex()) != '\n' && c != EOF) { current_col_lex++; }
            if (c == '\n') { // Consumed the newline
                current_line_lex++;
                current_col_lex = 1;
            }
        } else if (c == '/' && string_peek_lex() == '*') { // Multi-line comment
            string_getc_lex(); // consume '*'
            current_col_lex += 2; 
            while ((c = string_getc_lex()) != EOF) {
                if (c == '*') {
                    if (string_peek_lex() == '/') {
                        string_getc_lex(); // consume '/'
                        current_col_lex += 2;
                        break; 
                    } else { current_col_lex++; }
                } else if (c == '\n') {
                    current_line_lex++;
                    current_col_lex = 1;
                } else {
                    current_col_lex++;
                }
            }
             if (c == EOF) { /* Unterminated comment, error reported by parser usually */ }
        } else {
            string_ungetc_lex(); // Put back non-whitespace/comment char
            break;
        }
    }
}


static int is_lexer_symbol(int c) {
    return strchr("(){}[];,:.<>?", c) != NULL;
}

static int is_lexer_operator_char_start(int c) { // Chars that can start an operator
    return strchr("+-*/%=&|!<>", c) != NULL;
}


static const char *keywords[] = {
    "let", "const", "var", "function", "return", "if", "else", "while", "for", 
    "true", "false", "null",
    "class", "new", "this", "extends", "static", 
    "super", "fn", 
    "break", "continue",
    "public", "private", // "protected" could be added
    "import", "print", 
    "struct",
    "constructor", // Added constructor keyword
    // Built-in types are also keywords to the lexer to distinguish from identifiers
    "int", "long", "float", "double", "bool", "string", "char", "void", "any", "array", "object", "map", 
    "as", "in", "is",
    "func", // Alias for function keyword
};

static int is_lexer_keyword(const char *text) {
    for (size_t i = 0; i < sizeof(keywords)/sizeof(keywords[0]); i++) {
        if (strcmp(text, keywords[i]) == 0) return 1;
    }
    return 0;
}

static Token get_next_token_from_string() {
    skip_whitespace_and_comments_string();

    Token tok = { TOKEN_EOF, "", current_line_lex, current_col_lex };
    int c = string_getc_lex();

    if (c == EOF) return tok;

    tok.line = current_line_lex; // Capture start line/col for this token
    tok.col = current_col_lex;

    if (isalpha(c) || c == '_') { // Identifiers or Keywords
        int i = 0;
        tok.text[i++] = c;
        current_col_lex++;
        while ((c = string_getc_lex()) != EOF && (isalnum(c) || c == '_')) {
            if (i < (int)sizeof(tok.text) - 1) tok.text[i++] = c;
            current_col_lex++;
        }
        tok.text[i] = '\0';
        if (c != EOF) string_ungetc_lex();
        
        if (is_lexer_keyword(tok.text)) {
            tok.type = TOKEN_KEYWORD;
            if (strcmp(tok.text, "true") == 0 || strcmp(tok.text, "false") == 0) {
                tok.type = TOKEN_BOOL; // Specific type for bool literals
            }
        } else {
            tok.type = TOKEN_IDENTIFIER;
        }
    } else if (isdigit(c) || (c == '.' && isdigit(string_peek_lex()))) { // Numbers (int or float, or starting with .)
        int i = 0;
        int has_decimal = 0;
        if (c == '.') { // Starts with '.', e.g. .5
            tok.text[i++] = '0'; // Prepend 0 for standard float format if desired, or keep as is
            tok.text[i++] = c;
            has_decimal = 1;
            current_col_lex++;
        } else {
            tok.text[i++] = c;
            current_col_lex++;
        }

        while ((c = string_getc_lex()) != EOF) {
            if (isdigit(c)) {
                if (i < (int)sizeof(tok.text) - 1) tok.text[i++] = c;
                current_col_lex++;
            } else if (c == '.' && !has_decimal) { // Only one decimal point allowed
                if (i < (int)sizeof(tok.text) - 1) tok.text[i++] = c;
                has_decimal = 1;
                current_col_lex++;
            } else if ((c == 'e' || c == 'E') && i > 0 && isdigit(tok.text[i-1])) { // Scientific notation
                if (i < (int)sizeof(tok.text) - 2) { // Need space for 'e' and at least one digit/sign
                    tok.text[i++] = c;
                    current_col_lex++;
                    c = string_peek_lex();
                    if (c == '+' || c == '-') {
                        tok.text[i++] = string_getc_lex();
                        current_col_lex++;
                    }
                    if (!isdigit(string_peek_lex())) { // Must be followed by digits
                        fprintf(stderr, "Lexer Error (L%d:%d): Malformed exponent in number.\n", tok.line, tok.col + i);
                        tok.type = TOKEN_UNKNOWN; // Or error
                        // Unget 'e' and potentially sign if they were part of an identifier
                        // This part is tricky for error recovery.
                        break; 
                    }
                } else break; // Not enough space for exponent
            }
            else {
                if (c != EOF) string_ungetc_lex();
                break;
            }
        }
        tok.text[i] = '\0';
        tok.type = TOKEN_NUMBER;

    } else if (c == '"') { // String literals
        int i = 0;
        current_col_lex++; // For opening quote
        while ((c = string_getc_lex()) != EOF) {
            current_col_lex++;
            if (c == '"') break; // End of string
            if (c == '\\') { // Escape sequence
                int next_char = string_getc_lex();
                current_col_lex++;
                if (next_char == EOF) { /* Unterminated escape */ break; }
                switch (next_char) {
                    case 'n': tok.text[i++] = '\n'; break;
                    case 't': tok.text[i++] = '\t'; break;
                    case 'r': tok.text[i++] = '\r'; break;
                    case '\\': tok.text[i++] = '\\'; break;
                    case '"': tok.text[i++] = '"'; break;
                    default: tok.text[i++] = next_char; break; // Store as is
                }
            } else {
                if (i < (int)sizeof(tok.text) - 1) tok.text[i++] = c;
            }
            if (i >= (int)sizeof(tok.text) -1) { /* String too long */ break;}
        }
        tok.text[i] = '\0';
        if (c != '"') { /* Unterminated string */ }
        tok.type = TOKEN_STRING;
    } else if (c == '\'') { // Character literals
        int i = 0;
        current_col_lex++; // For opening quote
        c = string_getc_lex();
        if (c == EOF) {
            fprintf(stderr, "Lexer Error (L%d:%d): Unterminated character literal.\n", tok.line, tok.col);
            tok.type = TOKEN_UNKNOWN;
            return tok;
        }
        current_col_lex++;
        if (c == '\\') { // Escape sequence
            int next_char = string_getc_lex();
            current_col_lex++;
            if (next_char == EOF) {
                fprintf(stderr, "Lexer Error (L%d:%d): Unterminated escape in character literal.\n", tok.line, tok.col);
                tok.type = TOKEN_UNKNOWN;
                return tok;
            }
            switch (next_char) {
                case 'n': tok.text[i++] = '\n'; break;
                case 't': tok.text[i++] = '\t'; break;
                case 'r': tok.text[i++] = '\r'; break;
                case '\\': tok.text[i++] = '\\'; break;
                case '\'': tok.text[i++] = '\''; break;
                default: tok.text[i++] = next_char; break;
            }
        } else {
            tok.text[i++] = c;
        }
        tok.text[i] = '\0';
        
        c = string_getc_lex();
        if (c != '\'') {
            fprintf(stderr, "Lexer Error (L%d:%d): Expected closing single quote for character literal.\n", tok.line, tok.col);
            tok.type = TOKEN_UNKNOWN;
            return tok;
        }
        current_col_lex++;
        tok.type = TOKEN_STRING; // We'll use TOKEN_STRING for char literals too
    } else if (is_lexer_operator_char_start(c)) { // Operators
        int i = 0;
        tok.text[i++] = c;
        current_col_lex++;
        /* Extended multi-character operator support */
        char next_c = string_peek_lex();
        char next2_c = '\0';
        if (next_c != EOF) {
            /* Temporarily consume to peek two chars ahead */
            string_getc_lex();
            next2_c = string_peek_lex();
            string_ungetc_lex();
        }

        int consumed_additional = 0;

        /* 3-character operators – currently only >>> */
        if (c == '>' && next_c == '>' && next2_c == '>') {
            if (i < (int)sizeof(tok.text) - 1) { tok.text[i++] = string_getc_lex(); current_col_lex++; }
            if (i < (int)sizeof(tok.text) - 1) { tok.text[i++] = string_getc_lex(); current_col_lex++; }
            consumed_additional = 1;
        }

        /* 2-character operators */
        if (!consumed_additional) {
            if ((c == '+' && (next_c == '+' || next_c == '=')) ||
                (c == '-' && (next_c == '-' || next_c == '=')) ||
                (c == '*' && next_c == '=') ||
                (c == '/' && next_c == '=') ||
                (c == '%' && next_c == '=') ||
                (c == '=' && next_c == '=') ||
                (c == '!' && next_c == '=') ||
                (c == '<' && (next_c == '=' || next_c == '<')) ||
                (c == '>' && (next_c == '=' || next_c == '>')) ||
                (c == '&' && next_c == '&') ||
                (c == '|' && next_c == '|')) {
                if (i < (int)sizeof(tok.text) - 1) { tok.text[i++] = string_getc_lex(); current_col_lex++; }
            }
        }
        tok.text[i] = '\0';
        tok.type = TOKEN_OPERATOR;
    } else if (is_lexer_symbol(c)) { // Single character symbols
        tok.text[0] = c;
        tok.text[1] = '\0';
        current_col_lex++;
        tok.type = TOKEN_SYMBOL;
    } else { // Unknown character
        tok.text[0] = c;
        tok.text[1] = '\0';
        current_col_lex++;
        tok.type = TOKEN_UNKNOWN; // Mark as unknown
        fprintf(stderr, "Lexer Warning (L%d:%d): Unknown character '%c' (ASCII %d).\n", tok.line, tok.col, c, c);
    }
    return tok;
}


Token* lex(const char* source) {
    current_source_string = source;
    current_string_pos = 0;
    current_line_lex = 1;
    current_col_lex = 1;
    
    int capacity = 256; // Initial capacity
    Token* tokens_list = (Token*)malloc(capacity * sizeof(Token));
    if (!tokens_list) {
        fprintf(stderr, "Lexer Error: Memory allocation failed for tokens list.\n");
        return NULL;
    }
    
    int count = 0;
    while (1) {
        if (count >= capacity) {
            capacity *= 2;
            Token* new_tokens_list = (Token*)realloc(tokens_list, capacity * sizeof(Token));
            if (!new_tokens_list) {
                fprintf(stderr, "Lexer Error: Memory reallocation failed for tokens list.\n");
                free(tokens_list);
                return NULL;
            }
            tokens_list = new_tokens_list;
        }
        
        tokens_list[count] = get_next_token_from_string();
        if (tokens_list[count].type == TOKEN_EOF) {
            // Do not increment count for the final EOF if we want count to be actual number of non-EOF tokens
            // But parser expects EOF at tokens[count], so include it.
            break; 
        }
        count++;
    }
    // The loop breaks when EOF is current token, so tokens_list[count] is EOF.
    // No need to add another sentinel if the loop condition is `while(1)` and break on EOF.
    return tokens_list;
}
