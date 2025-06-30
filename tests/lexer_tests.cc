#include "ourolang/lexer.h"
#include "ourolang/arena.h"
#include <cassert>
#include <vector>

int main() {
    const std::string source = "let x: int = 42;";
    ouro::Arena arena;
    ouro::Lexer lexer(source, arena.get_resource());
    auto tokens = lexer.tokenize();

    std::vector<ouro::TokenType> expected{
        ouro::TokenType::LET,
        ouro::TokenType::IDENTIFIER,
        ouro::TokenType::COLON,
        ouro::TokenType::INT,
        ouro::TokenType::EQUALS,
        ouro::TokenType::NUMBER,
        ouro::TokenType::SEMICOLON,
        ouro::TokenType::EOF_TOKEN};

    assert(tokens.size() == expected.size());
    for (size_t i = 0; i < expected.size(); ++i) {
        assert(tokens[i].type == expected[i]);
    }
    assert(tokens[1].value == "x");
    assert(tokens[5].value == "42");
    return 0;
}
