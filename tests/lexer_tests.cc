#include "ourolang/lexer.h"
#include <cassert>

int main() {
    using namespace ouro;
    Lexer lex("let x = 42;");
    auto tokens = lex.tokenize();

    assert(tokens.size() == 6); // includes EOF token
    assert(tokens[0].type == TokenType::LET);
    assert(tokens[1].type == TokenType::IDENTIFIER && tokens[1].value == "x");
    assert(tokens[2].type == TokenType::EQUALS);
    assert(tokens[3].type == TokenType::NUMBER && tokens[3].value == "42");
    assert(tokens[4].type == TokenType::SEMICOLON);
    assert(tokens[5].type == TokenType::EOF_TOKEN);
    return 0;
}
