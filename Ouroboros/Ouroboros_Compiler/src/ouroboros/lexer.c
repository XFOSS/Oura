// lexer.c
// Implements the lexical analysis (scanning) for Ouroboros source code.

#include <lexer.h>
#include <token.h>
#include <stdio.h> // For fprintf, EOF
#include <stdlib.h> // For malloc, free, exit, strtol, strtod
#include <string.h> // For strlen, strncmp, strcpy
#include <ctype.h>  // For isalpha, isdigit, isalnum, isspace

// --- Static helper function declarations ---
static char lexer_peek(Lexer* lexer);
static char lexer_peek_next(Lexer* lexer);
static char lexer_advance(Lexer* lexer);
static int lexer_match(Lexer* lexer, char expected);
static void lexer_skip_whitespace(Lexer* lexer); // Now strictly for whitespace and comments
static Token* lexer_make_token(Lexer* lexer, TokenType type);
static Token* lexer_make_literal_token(Lexer* lexer, TokenType type, LiteralValue literal);
static Token* lexer_make_error_token(Lexer* lexer, const char* message);
static Token* lexer_scan_string_literal(Lexer* lexer); // Returns Token*
static Token* lexer_scan_char_literal(Lexer* lexer);  // Returns Token*
static Token* lexer_scan_number_literal(Lexer* lexer); // Returns Token*
static Token* lexer_scan_identifier_or_keyword(Lexer* lexer); // Returns Token*

// --- Keyword mapping ---
// A simple array of structs and linear search for simplicity.
typedef struct {
    const char* lexeme;
    TokenType type;
} KeywordMapping;

static KeywordMapping keywords[] = {
    {"if", IF}, {"else", ELSE}, {"while", WHILE}, {"for", FOR}, {"do", DO},
    {"return", RETURN}, {"break", BREAK}, {"continue", CONTINUE},
    {"class", CLASS}, {"interface", INTERFACE}, {"extends", EXTENDS}, {"implements", IMPLEMENTS},
    {"this", THIS}, {"super", SUPER}, {"new", NEW}, {"static", STATIC},
    {"public", PUBLIC}, {"private", PRIVATE}, {"protected", PROTECTED}, {"internal", INTERNAL},
    {"var", VAR}, {"let", LET}, {"const", CONST}, {"final", FINAL},
    {"try", TRY}, {"catch", CATCH}, {"finally", FINALLY}, {"throw", THROW}, {"throws", THROWS},
    {"package", PACKAGE}, {"import", IMPORT},
    {"ref", REF}, {"out", OUT},
    {"void", VOID}, {"foreach", FOREACH}, {"enum", ENUM},
    {"char", CHAR_TYPE}, {"short", SHORT_TYPE}, {"int", INT_TYPE}, {"long", LONG_TYPE},
    {"float", FLOAT_TYPE}, {"double", DOUBLE_TYPE}, {"boolean", BOOLEAN_TYPE}, {"String", STRING_TYPE}, // Note: "String" as a type keyword
    {"true", BOOLEAN_LITERAL}, {"false", BOOLEAN_LITERAL}, {"null", NULL_LITERAL},
    {NULL, ERROR_TOKEN} // Sentinel to mark the end of the array
};


// --- Public API Implementations ---

/**
 * Initializes a new Lexer instance.
 * @param source_code The entire source code string to lex. This string should persist
 * for the lifetime of the lexer (e.g., loaded from a file into a buffer).
 * @param source_name A descriptive name for the source (e.g., file path or "stdin").
 * @return A dynamically allocated Lexer pointer, or NULL on allocation failure.
 */
Lexer* lexer_init(const char* source_code, const char* source_name) {
    Lexer* lexer = (Lexer*)malloc(sizeof(Lexer));
    if (lexer == NULL) {
        fprintf(stderr, "Error: Memory allocation failed for Lexer.\n");
        return NULL;
    }
    lexer->source_code = source_code;
    lexer->source_length = (int)strlen(source_code);
    lexer->current_pos = 0;
    lexer->start_pos = 0;
    lexer->current_line = 1;
    lexer->current_column = 1;
    lexer->source_name = source_name; // source_name is assumed to be a persistent string (e.g., a literal or path)

    return lexer;
}

/**
 * Scans and returns the next token from the source code.
 * Responsibility: Caller must free the returned Token* using free_token().
 * @param lexer A pointer to the Lexer instance.
 * @return A dynamically allocated Token pointer, or NULL if an unrecoverable error occurs.
 */
Token* lexer_scan_token(Lexer* lexer) {
    lexer_skip_whitespace(lexer); // Skip any leading whitespace or comments

    lexer->start_pos = lexer->current_pos; // Mark the beginning of the new token

    if (lexer->current_pos >= lexer->source_length) {
        return lexer_make_token(lexer, EOF_TOKEN);
    }

    char c = lexer_advance(lexer); // Consume the current character

    switch (c) {
        // --- Single-character tokens ---
        case '(': return lexer_make_token(lexer, LPARENT);
        case ')': return lexer_make_token(lexer, RPARENT);
        case '{': return lexer_make_token(lexer, LBRACE);
        case '}': return lexer_make_token(lexer, RBRACE);
        case '[': return lexer_make_token(lexer, LBRACKET);
        case ']': return lexer_make_token(lexer, RBRACKET);
        case ',': return lexer_make_token(lexer, COMMA);
        case ';': return lexer_make_token(lexer, SEMICOLON);
        case ':': return lexer_make_token(lexer, COLON);
        case '?': return lexer_make_token(lexer, QUESTION_MARK);
        case '~': return lexer_make_token(lexer, BIT_NOT);

        // --- Multi-character operators and period/range ---
        case '.':
            if (lexer_match(lexer, '.')) {
                if (lexer_match(lexer, '.')) return lexer_make_token(lexer, RANGE); // ...
                if (lexer_match(lexer, '<')) return lexer_make_token(lexer, EXCLUSIVE_RANGE); // ..<
            }
            return lexer_make_token(lexer, PERIOD);

        case '+':
            if (lexer_match(lexer, '+')) return lexer_make_token(lexer, INCREMENT);
            if (lexer_match(lexer, '=')) return lexer_make_token(lexer, PLUS_EQUALS);
            return lexer_make_token(lexer, PLUS);
        case '-':
            if (lexer_match(lexer, '-')) return lexer_make_token(lexer, DECREMENT);
            if (lexer_match(lexer, '=')) return lexer_make_token(lexer, MINUS_EQUALS);
            return lexer_make_token(lexer, MINUS);
        case '*':
            if (lexer_match(lexer, '=')) return lexer_make_token(lexer, MULTIPLY_EQUALS);
            return lexer_make_token(lexer, MULTIPLY);
        case '/':
            if (lexer_match(lexer, '=')) return lexer_make_token(lexer, DIVIDE_EQUALS);
            // Comments are handled in lexer_skip_whitespace, so this '/' is always division here.
            return lexer_make_token(lexer, DIVIDE);
        case '%':
            if (lexer_match(lexer, '=')) return lexer_make_token(lexer, MODULO_EQUALS);
            return lexer_make_token(lexer, MODULO);
        case '!':
            if (lexer_match(lexer, '=')) return lexer_make_token(lexer, NE);
            return lexer_make_token(lexer, NOT);
        case '=':
            if (lexer_match(lexer, '=')) return lexer_make_token(lexer, EQ);
            return lexer_make_token(lexer, ASSIGN);
        case '<':
            if (lexer_match(lexer, '=')) return lexer_make_token(lexer, LE);
            if (lexer_match(lexer, '<')) {
                if (lexer_match(lexer, '=')) return lexer_make_token(lexer, LEFT_SHIFT_EQUALS);
                return lexer_make_token(lexer, LEFT_SHIFT);
            }
            return lexer_make_token(lexer, LT);
        case '>':
            if (lexer_match(lexer, '=')) return lexer_make_token(lexer, GE);
            if (lexer_match(lexer, '>')) {
                if (lexer_match(lexer, '=')) return lexer_make_token(lexer, RIGHT_SHIFT_EQUALS);
                if (lexer_match(lexer, '>')) { // >>>
                    if (lexer_match(lexer, '=')) return lexer_make_token(lexer, UNSIGNED_RIGHT_SHIFT_EQUALS); // >>>=
                    return lexer_make_token(lexer, UNSIGNED_RIGHT_SHIFT);
                }
                return lexer_make_token(lexer, RIGHT_SHIFT);
            }
            return lexer_make_token(lexer, GT);
        case '&':
            if (lexer_match(lexer, '&')) return lexer_make_token(lexer, AND);
            if (lexer_match(lexer, '=')) return lexer_make_token(lexer, BIT_AND_EQUALS);
            return lexer_make_token(lexer, BIT_AND);
        case '|':
            if (lexer_match(lexer, '|')) return lexer_make_token(lexer, OR);
            if (lexer_match(lexer, '=')) return lexer_make_token(lexer, BIT_OR_EQUALS);
            return lexer_make_token(lexer, BIT_OR);
        case '^':
            if (lexer_match(lexer, '=')) return lexer_make_token(lexer, BIT_XOR_EQUALS);
            return lexer_make_token(lexer, BIT_XOR);

        // --- Literals ---
        case '"': return lexer_scan_string_literal(lexer);
        case '\'': return lexer_scan_char_literal(lexer);

        default:
            if (isdigit(c)) {
                return lexer_scan_number_literal(lexer);
            } else if (isalpha(c) || c == '_') {
                return lexer_scan_identifier_or_keyword(lexer);
            } else {
                // If we reach here, it's an unrecognized character
                return lexer_make_error_token(lexer, "Unrecognized character.");
            }
    }
}

/**
 * Frees the memory associated with a Lexer instance.
 * Note: This does NOT free the source_code string, as it is assumed to be owned externally.
 * @param lexer A pointer to the Lexer instance to free.
 */
void lexer_free(Lexer* lexer) {
    if (lexer) {
        free(lexer);
    }
}


// --- Static Helper Function Implementations ---

/**
 * Returns the character at the current position without advancing.
 * Returns EOF if at the end of the source.
 */
static char lexer_peek(Lexer* lexer) {
    if (lexer->current_pos >= lexer->source_length) {
        return EOF;
    }
    return lexer->source_code[lexer->current_pos];
}

/**
 * Returns the character immediately after the current position without advancing.
 * Returns EOF if not enough characters remain.
 */
static char lexer_peek_next(Lexer* lexer) {
    if (lexer->current_pos + 1 >= lexer->source_length) {
        return EOF;
    }
    return lexer->source_code[lexer->current_pos + 1];
}

/**
 * Consumes the current character and advances the position.
 * Also updates line and column numbers.
 * @return The character that was advanced past.
 */
static char lexer_advance(Lexer* lexer) {
    char c = lexer->source_code[lexer->current_pos++];
    if (c == '\n') {
        lexer->current_line++;
        lexer->current_column = 1; // Reset column for new line
    } else {
        lexer->current_column++;
    }
    return c;
}

/**
 * Checks if the current character matches the expected character.
 * If it matches, consumes it and returns true. Otherwise, returns false.
 */
static int lexer_match(Lexer* lexer, char expected) {
    if (lexer->current_pos >= lexer->source_length) return 0;
    if (lexer->source_code[lexer->current_pos] != expected) return 0;

    lexer_advance(lexer); // Consume the matched character
    return 1;
}

/**
 * Skips whitespace and comments (single-line and multi-line).
 * Modifies lexer's current_pos, current_line, current_column.
 */
static void lexer_skip_whitespace(Lexer* lexer) {
    for (;;) {
        char c = lexer_peek(lexer);
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
                lexer_advance(lexer);
                break;
            case '\n':
                lexer_advance(lexer);
                break;
            case '/':
                if (lexer_peek_next(lexer) == '/') {
                    // Single-line comment: skip until end of line
                    while (lexer_peek(lexer) != '\n' && lexer_peek(lexer) != EOF) {
                        lexer_advance(lexer);
                    }
                } else if (lexer_peek_next(lexer) == '*') {
                    // Multi-line comment: skip until */
                    lexer_advance(lexer); // Consume '/'
                    lexer_advance(lexer); // Consume '*'
                    while (lexer_peek(lexer) != EOF) {
                        if (lexer_peek(lexer) == '*' && lexer_peek_next(lexer) == '/') {
                            lexer_advance(lexer); // Consume '*'
                            lexer_advance(lexer); // Consume '/'
                            break; // Exit comment loop
                        }
                        lexer_advance(lexer);
                    }
                    if (lexer_peek(lexer) == EOF) {
                        // This indicates an unterminated comment.
                        // For simplicity, the main lexer_scan_token will now treat
                        // whatever's left as an error if it hits EOF without finding a token.
                        // Or, we could return an error token directly from here.
                        // For now, we allow the main scanner to handle it.
                        return;
                    }
                } else {
                    // Not a comment, but a division operator.
                    // Stop skipping and let the main switch handle it.
                    return;
                }
                break;
            default:
                // Not whitespace or a comment start, so return
                return;
        }
    }
}

/**
 * Creates a basic token with the current lexeme range.
 * Automatically extracts the lexeme string and duplicates it.
 * @param lexer The lexer instance.
 * @param type The TokenType.
 * @return A new Token*. Caller must free.
 */
static Token* lexer_make_token(Lexer* lexer, TokenType type) {
    int lexeme_length = lexer->current_pos - lexer->start_pos;
    char* lexeme_buf = (char*)malloc(lexeme_length + 1);
    if (lexeme_buf == NULL) {
        fprintf(stderr, "Error: Memory allocation failed for lexeme_buf in make_token.\n");
        exit(EXIT_FAILURE);
    }
    strncpy(lexeme_buf, lexer->source_code + lexer->start_pos, lexeme_length);
    lexeme_buf[lexeme_length] = '\0';

    int token_start_column = lexer->current_column - lexeme_length;
    // Special handling for tokens that might start right after a newline, resetting column.
    if (lexer->source_code[lexer->start_pos] == '\n' || lexer->start_pos == 0) {
        token_start_column = 1;
    }

    Token* token = create_token(type, lexeme_buf, lexer->current_line, token_start_column, lexer->source_name);
    free(lexeme_buf); // create_token duplicates it, so free this temp buffer
    return token;
}

/**
 * Creates a token with a literal value from the current lexeme range.
 * @param lexer The lexer instance.
 * @param type The TokenType (must be a literal type).
 * @param literal The LiteralValue union containing the parsed value.
 * @return A new Token*. Caller must free.
 */
static Token* lexer_make_literal_token(Lexer* lexer, TokenType type, LiteralValue literal) {
    int lexeme_length = lexer->current_pos - lexer->start_pos;
    char* lexeme_buf = (char*)malloc(lexeme_length + 1);
    if (lexeme_buf == NULL) {
        fprintf(stderr, "Error: Memory allocation failed for lexeme_buf in make_literal_token.\n");
        exit(EXIT_FAILURE);
    }
    strncpy(lexeme_buf, lexer->source_code + lexer->start_pos, lexeme_length);
    lexeme_buf[lexeme_length] = '\0';

    int token_start_column = lexer->current_column - lexeme_length;
     if (lexer->source_code[lexer->start_pos] == '\n' || lexer->start_pos == 0) {
        token_start_column = 1;
    }

    Token* token = create_token_with_literal(type, lexeme_buf, literal, lexer->current_line, token_start_column, lexer->source_name);
    free(lexeme_buf);
    return token;
}

/**
 * Creates an error token from the current lexeme range.
 * @param lexer The lexer instance.
 * @param message The specific error message.
 * @return A new Token*. Caller must free.
 */
static Token* lexer_make_error_token(Lexer* lexer, const char* message) {
    int lexeme_length = lexer->current_pos - lexer->start_pos;
    char* lexeme_buf = (char*)malloc(lexeme_length + 1);
    if (lexeme_buf == NULL) {
        fprintf(stderr, "Error: Memory allocation failed for lexeme_buf in make_error_token.\n");
        exit(EXIT_FAILURE);
    }
    strncpy(lexeme_buf, lexer->source_code + lexer->start_pos, lexeme_length);
    lexeme_buf[lexeme_length] = '\0';

    int token_start_column = lexer->current_column - lexeme_length;
     if (lexer->source_code[lexer->start_pos] == '\n' || lexer->start_pos == 0) {
        token_start_column = 1;
    }

    Token* token = create_error_token(lexeme_buf, message, lexer->current_line, token_start_column, lexer->source_name);
    free(lexeme_buf);
    return token;
}

/**
 * Scans a string literal (e.g., "hello world").
 * The initial '"' should already be consumed by lexer_advance().
 * @param lexer The lexer instance.
 * @return A STRING_LITERAL token or an ERROR_TOKEN if unterminated/invalid.
 */
static Token* lexer_scan_string_literal(Lexer* lexer) {
    int literal_start_pos = lexer->current_pos; // Start after the opening '"'

    while (lexer_peek(lexer) != '"' && lexer_peek(lexer) != EOF) {
        if (lexer_peek(lexer) == '\\') { // Handle escape sequences within string
            lexer_advance(lexer); // Consume '\'
            // For now, simply consume the next char. Real compiler needs full escape sequence logic.
            if (lexer_peek(lexer) != EOF) lexer_advance(lexer);
        } else {
            lexer_advance(lexer);
        }
    }

    if (lexer_peek(lexer) == EOF) {
        return lexer_make_error_token(lexer, "Unterminated string literal.");
    }

    // Extract the content *before* consuming the closing quote
    int string_content_length = lexer->current_pos - literal_start_pos;
    char* string_content = (char*)malloc(string_content_length + 1);
    if (string_content == NULL) {
        fprintf(stderr, "Error: Memory allocation failed for string_content.\n");
        exit(EXIT_FAILURE);
    }
    strncpy(string_content, lexer->source_code + literal_start_pos, string_content_length);
    string_content[string_content_length] = '\0';

    lexer_advance(lexer); // Consume the closing '"'

    LiteralValue lit_val;
    lit_val.string_val = string_content; // create_token_with_literal will duplicate this
    Token* token = lexer_make_literal_token(lexer, STRING_LITERAL, lit_val);
    free(string_content); // Free the temporary buffer, as create_token_with_literal duplicates it
    return token;
}

/**
 * Scans a character literal (e.g., 'a', '\n').
 * The initial "'" should already be consumed by lexer_advance().
 * @param lexer The lexer instance.
 * @return A CHARACTER_LITERAL token or an ERROR_TOKEN if unterminated/invalid.
 */
static Token* lexer_scan_char_literal(Lexer* lexer) {
    char char_val;

    if (lexer_peek(lexer) == EOF) {
        return lexer_make_error_token(lexer, "Unterminated character literal.");
    }

    if (lexer_peek(lexer) == '\\') { // Handle escape sequences
        lexer_advance(lexer); // Consume '\'
        if (lexer_peek(lexer) == EOF) {
            return lexer_make_error_token(lexer, "Incomplete escape sequence in character literal.");
        }
        char escaped_char = lexer_advance(lexer);
        switch (escaped_char) {
            case 'n': char_val = '\n'; break;
            case 't': char_val = '\t'; break;
            case 'r': char_val = '\r'; break;
            case 'b': char_val = '\b'; break;
            case 'f': char_val = '\f'; break;
            case '\\': char_val = '\\'; break;
            case '\'': char_val = '\''; break;
            case '"': char_val = '"'; break;
            // Add more escape sequences as needed (e.g., \u for unicode, \x for hex)
            default:
                return lexer_make_error_token(lexer, "Unknown escape sequence in character literal.");
        }
    } else {
        char_val = lexer_advance(lexer); // Consume the character
    }

    if (lexer_peek(lexer) != '\'') {
        return lexer_make_error_token(lexer, "Unterminated or invalid character literal. Expected single character followed by closing quote.");
    }
    lexer_advance(lexer); // Consume the closing "'"

    LiteralValue lit_val;
    lit_val.char_val = char_val;
    return lexer_make_literal_token(lexer, CHARACTER_LITERAL, lit_val);
}

/**
 * Scans an integer or floating-point number literal.
 * The first digit should already be consumed by lexer_advance().
 * @param lexer The lexer instance.
 * @return An INTEGER_LITERAL or FLOATING_POINT_LITERAL token, or an ERROR_TOKEN.
 */
static Token* lexer_scan_number_literal(Lexer* lexer) {
    // Consume digits before a potential decimal point
    while (isdigit(lexer_peek(lexer))) {
        lexer_advance(lexer);
    }

    // Check for a fractional part
    int is_float = 0;
    if (lexer_peek(lexer) == '.' && isdigit(lexer_peek_next(lexer))) {
        is_float = 1;
        lexer_advance(lexer); // Consume '.'
        while (isdigit(lexer_peek(lexer))) {
            lexer_advance(lexer); // Consume digits after decimal
        }
    }

    // Check for exponent part (e.g., 1.2e-5)
    if ((lexer_peek(lexer) == 'e' || lexer_peek(lexer) == 'E') &&
        (isdigit(lexer_peek_next(lexer)) ||
         ((lexer_peek_next(lexer) == '+' || lexer_peek_next(lexer) == '-') && isdigit(lexer->source_code[lexer->current_pos + 2])))) {
        is_float = 1;
        lexer_advance(lexer); // Consume 'e' or 'E'
        if (lexer_peek(lexer) == '+' || lexer_peek(lexer) == '-') {
            lexer_advance(lexer); // Consume '+' or '-'
        }
        while (isdigit(lexer_peek(lexer))) {
            lexer_advance(lexer); // Consume exponent digits
        }
    }

    // Extract the numeric lexeme string
    int lexeme_length = lexer->current_pos - lexer->start_pos;
    char* num_str = (char*)malloc(lexeme_length + 1);
    if (num_str == NULL) {
        fprintf(stderr, "Error: Memory allocation failed for number string in scan_number_literal.\n");
        exit(EXIT_FAILURE);
    }
    strncpy(num_str, lexer->source_code + lexer->start_pos, lexeme_length);
    num_str[lexeme_length] = '\0';

    LiteralValue lit_val;
    Token* token = NULL;

    if (is_float) {
        char *endptr;
        lit_val.float_val = strtod(num_str, &endptr);
        if (*endptr != '\0') {
            token = lexer_make_error_token(lexer, "Invalid floating point literal.");
        } else {
            token = lexer_make_literal_token(lexer, FLOATING_POINT_LITERAL, lit_val);
        }
    } else {
        char *endptr;
        lit_val.integer_val = strtoll(num_str, &endptr, 10); // Base 10
        if (*endptr != '\0') {
             token = lexer_make_error_token(lexer, "Invalid integer literal.");
        } else {
            token = lexer_make_literal_token(lexer, INTEGER_LITERAL, lit_val);
        }
    }
    free(num_str); // Free the temporary buffer
    return token;
}

/**
 * Scans an identifier or a keyword.
 * The first character (alpha or underscore) should already be consumed.
 * @param lexer The lexer instance.
 * @return An IDENTIFIER token or a keyword TokenType token.
 */
static Token* lexer_scan_identifier_or_keyword(Lexer* lexer) {
    while (isalnum(lexer_peek(lexer)) || lexer_peek(lexer) == '_') {
        lexer_advance(lexer);
    }

    int lexeme_length = lexer->current_pos - lexer->start_pos;
    char* text = (char*)malloc(lexeme_length + 1);
    if (text == NULL) {
        fprintf(stderr, "Error: Memory allocation failed for identifier/keyword text.\n");
        exit(EXIT_FAILURE);
    }
    strncpy(text, lexer->source_code + lexer->start_pos, lexeme_length);
    text[lexeme_length] = '\0';

    TokenType type = IDENTIFIER; // Assume it's an identifier by default

    // Check if it's a reserved keyword
    for (int i = 0; keywords[i].lexeme != NULL; i++) {
        if (strcmp(text, keywords[i].lexeme) == 0) {
            type = keywords[i].type;
            break;
        }
    }
    
    // For boolean and null literals, the lexeme is also the literal value
    LiteralValue lit_val;
    if (type == BOOLEAN_LITERAL) {
        lit_val.boolean_val = (strcmp(text, "true") == 0);
        Token* token = lexer_make_literal_token(lexer, type, lit_val);
        free(text);
        return token;
    } else if (type == NULL_LITERAL) {
        // NULL_LITERAL typically doesn't hold a value in the union, just its type is enough
        Token* token = lexer_make_token(lexer, type); // Use make_token as no literal value for union
        free(text);
        return token;
    } else {
        Token* token = lexer_make_token(lexer, type);
        free(text); // Free the temporary text buffer
        return token;
    }
}
