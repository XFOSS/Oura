#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <cstdint>

namespace ouro {

enum class TokenType {
    LET, FN, IF, ELSE, RETURN, FOR, IN, ASYNC, AWAIT, GPU,
    INT, FLOAT, STRING, IDENTIFIER, NUMBER, STRING_LITERAL,
    COLON, EQUALS, LPAREN, RPAREN, LBRACE, RBRACE, SEMICOLON, COMMA,
    PLUS, MINUS, MUL, DIV, GT, DOTDOT, ARROW, EOF_TOKEN
};

struct Token {
    TokenType type;
    std::string value;
    int line;
};

} // namespace ouro
