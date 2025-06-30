#include "ourolang/lexer.h"
#include <cassert>
#include <vector>
#include <string>

int main() {
    ouro::Lexer lexer("let x = 42;");
    auto tokens = lexer.tokenize();
    assert(tokens.size() == 6);
    assert(tokens[0].type == ouro::TokenType::LET);
    assert(tokens[1].type == ouro::TokenType::IDENTIFIER && tokens[1].value == "x");
    assert(tokens[2].type == ouro::TokenType::EQUALS);
    assert(tokens[3].type == ouro::TokenType::NUMBER && tokens[3].value == "42");
    assert(tokens[4].type == ouro::TokenType::SEMICOLON);
    assert(tokens[5].type == ouro::TokenType::EOF_TOKEN);
    return 0;
}
