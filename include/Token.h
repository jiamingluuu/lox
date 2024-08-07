#pragma once

#include <any>
#include <string>

enum TokenType {
    // Single-character tokens.
    LEFT_PAREN, RIGHT_PAREN, LEFT_BRACE, RIGHT_BRACE, COMMA, DOT, MINUS, PLUS, SEMICOLON, SLASH, STAR,

    // One or two character tokens.
    BANG, BANG_EQUAL, EQUAL, EQUAL_EQUAL, GREATER, GREATER_EQUAL, LESS, LESS_EQUAL,

    // Literals.
    IDENTIFIER, STRING, NUMBER,

    // Keywords.
    AND, CLASS, ELSE, FALSE, FUN, FOR, IF, NIL, OR, PRINT, RETURN, SUPER, THIS, TRUE, VAR, WHILE,

    END_OF_FILE
};

class Token {
 public:
    Token(TokenType type, std::string lexeme, std::any literal, int line);
    friend std::ostream& operator<<(std::ostream& os, const Token& token);

    std::string tokenTypeToString(TokenType type);
    TokenType type;
    std::string lexeme;
    std::any literal;
    int line;
};
