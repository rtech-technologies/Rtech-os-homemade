#ifndef LEXER_H
#define LEXER_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    TOK_EOF = 0,
    TOK_IDENTIFIER,
    TOK_NUMBER,
    TOK_STRING,

    // Keywords
    TOK_ENTITY,
    TOK_ON,
    TOK_IF,

    // Punctuation & Operators
    TOK_COLON,
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_LBRACKET,
    TOK_RBRACKET,
    TOK_ASSIGN,
    TOK_PLUS,
    TOK_MINUS,
    TOK_DOT,
    TOK_COMMA,

    // Structural
    TOK_NEWLINE,
    TOK_INDENT,
    TOK_DEDENT,

    // Error
    TOK_ERROR
} TokenType;

typedef struct {
    TokenType type;
    const char *start;
    int length;
    int line;
} Token;

void lexer_init(const char *source);
Token lexer_next_token(void);

// Helper for debugging
const char* token_type_name(TokenType type);

#endif
