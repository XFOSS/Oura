#pragma once
#include "token.h"
#include "arena.h"
#include <string>
#include <memory_resource>
#include <vector>
#include <cctype>
#include <stdexcept>

namespace ouro {

class Lexer {
    std::string source;
    size_t pos = 0;
    int line = 1;
    std::pmr::memory_resource* mem;

public:
    explicit Lexer(const std::string& src, std::pmr::memory_resource* r = std::pmr::get_default_resource())
        : source(src), mem(r) {}

    std::vector<Token> tokenize() {
        std::pmr::vector<Token> tokens(mem);
        while (pos < source.size()) {
            char c = source[pos];
            if (std::isspace(static_cast<unsigned char>(c))) {
                if (c == '\n') line++;
                pos++;
                continue;
            }
            if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
                tokens.push_back(parse_identifier());
            } else if (std::isdigit(static_cast<unsigned char>(c)) || c == '.') {
                tokens.push_back(parse_number());
            } else if (c == '"') {
                tokens.push_back(parse_string());
            } else {
                tokens.push_back(parse_symbol());
            }
        }
        tokens.push_back({TokenType::EOF_TOKEN, "", line});
        return std::vector<Token>(tokens.begin(), tokens.end());
    }

private:
    Token parse_identifier() {
        std::string value;
        while (pos < source.size() && (std::isalnum(static_cast<unsigned char>(source[pos])) || source[pos] == '_')) {
            value += source[pos++];
        }
        if (value == "let") return {TokenType::LET, value, line};
        if (value == "fn") return {TokenType::FN, value, line};
        if (value == "if") return {TokenType::IF, value, line};
        if (value == "else") return {TokenType::ELSE, value, line};
        if (value == "return") return {TokenType::RETURN, value, line};
        if (value == "for") return {TokenType::FOR, value, line};
        if (value == "in") return {TokenType::IN, value, line};
        if (value == "async") return {TokenType::ASYNC, value, line};
        if (value == "await") return {TokenType::AWAIT, value, line};
        if (value == "gpu") return {TokenType::GPU, value, line};
        if (value == "int") return {TokenType::INT, value, line};
        if (value == "float") return {TokenType::FLOAT, value, line};
        if (value == "string") return {TokenType::STRING, value, line};
        return {TokenType::IDENTIFIER, value, line};
    }

    Token parse_number() {
        std::string value;
        bool has_dot = false;
        while (pos < source.size() && (std::isdigit(static_cast<unsigned char>(source[pos])) || source[pos] == '.')) {
            if (source[pos] == '.') has_dot = true;
            value += source[pos++];
        }
        return {TokenType::NUMBER, value, line};
    }

    Token parse_string() {
        std::string value;
        pos++; // Skip opening quote
        while (pos < source.size() && source[pos] != '"') {
            value += source[pos++];
        }
        pos++; // Skip closing quote
        return {TokenType::STRING_LITERAL, value, line};
    }

    Token parse_symbol() {
        char c = source[pos++];
        switch (c) {
            case ':': return {TokenType::COLON, ":", line};
            case '=': return {TokenType::EQUALS, "=", line};
            case '(': return {TokenType::LPAREN, "(", line};
            case ')': return {TokenType::RPAREN, ")", line};
            case '{': return {TokenType::LBRACE, "{", line};
            case '}': return {TokenType::RBRACE, "}", line};
            case ';': return {TokenType::SEMICOLON, ";", line};
            case ',': return {TokenType::COMMA, ",", line};
            case '+': return {TokenType::PLUS, "+", line};
            case '-':
                if (pos < source.size() && source[pos] == '>') {
                    pos++;
                    return {TokenType::ARROW, "->", line};
                }
                return {TokenType::MINUS, "-", line};
            case '*': return {TokenType::MUL, "*", line};
            case '/': return {TokenType::DIV, "/", line};
            case '>': return {TokenType::GT, ">", line};
            case '.':
                if (pos < source.size() && source[pos] == '.') {
                    pos++;
                    return {TokenType::DOTDOT, "..", line};
                }
                break;
        }
        throw std::runtime_error("Unknown symbol at line " + std::to_string(line));
    }
};

} // namespace ouro
