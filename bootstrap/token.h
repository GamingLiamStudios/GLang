#pragma once
#include <stdio.h>
#include <stddef.h>

struct debug_info
{
    int line;
    int column;
};

struct glcpg_token
{
    enum
    {
        E_TOKEN_EOF = -127,

        // Values
        E_TOKEN_INTEGER,
        E_TOKEN_STRING,
        E_TOKEN_IDENT,

        // Keywords
        E_TOKEN_PUBLIC,    // pub
        E_TOKEN_CONST,     // const

        E_TOKEN_FN,        // fn
        E_TOKEN_STRUCT,    // struct
        E_TOKEN_TRAIT,     // trait
        E_TOKEN_ENUM,      // enum

        E_TOKEN_IMPL,      // impl
        E_TOKEN_EXTERN,    // extern

        E_TOKEN_LET,    // let
        E_TOKEN_AS,     // as

        E_TOKEN_IF,       // if
        E_TOKEN_ELSE,     // else
        E_TOKEN_MATCH,    // match

        E_TOKEN_WHILE,       // while
        E_TOKEN_LOOP,        // loop
        E_TOKEN_BREAK,       // break
        E_TOKEN_CONTINUE,    // continue

        // Single-char Tokens
        E_TOKEN_LPAREN,    // (
        E_TOKEN_RPAREN,    // )
        E_TOKEN_LBRACE,    // {
        E_TOKEN_RBRACE,    // }
        E_TOKEN_LBRACK,    // [
        E_TOKEN_RBRACK,    // ]

        E_TOKEN_SEMICOLON,    // ;
        E_TOKEN_COMMA,        // ,
        E_TOKEN_EQUAL,        // =
        E_TOKEN_POINT,        // .
        E_TOKEN_COLON,        // :

        E_TOKEN_PLUS,     // +
        E_TOKEN_MINUS,    // -
        E_TOKEN_STAR,     // *
        E_TOKEN_SLASH,    // /

        E_TOKEN_EXMARK,      // !
        E_TOKEN_AND,         // &
        E_TOKEN_BAR,         // |
        E_TOKEN_QUESTION,    // ?
        E_TOKEN_TILDE,       // ~
        E_TOKEN_CARET,       // ^

        // Multi-char Tokens
        E_TOKEN_EQEQ,      // ==
        E_TOKEN_NEQ,       // !=
        E_TOKEN_LT,        // <
        E_TOKEN_LEQ,       // <=
        E_TOKEN_GT,        // >
        E_TOKEN_GEQ,       // >=
        E_TOKEN_BARBAR,    // ||
        E_TOKEN_ANDAND,    // &&

        E_TOKEN_PLUSEQUAL,     // +=
        E_TOKEN_MINUSEQUAL,    // -=
        E_TOKEN_STAREQUAL,     // *=
        E_TOKEN_SLASHEQUAL,    // /=

        E_TOKEN_BAREQUAL,    // |=
        E_TOKEN_ANDEQUAL,    // &=

        E_TOKEN_IS,    // ->
    } value;           // For unknown ASCII chars; will be value of char

    union
    {
        unsigned long integer;
        const char   *string;
    } data;

    struct debug_info debug_info;
};

enum token_error
{
    E_TOK_MEMORYERROR = -255,
    E_TOK_IOERROR,
    E_TOK_INVALIDINPUT,
};

void token_debug(FILE *file, struct glcpg_token *tok);
int  token_debug_str(char *string, size_t length, struct glcpg_token *tok);

ptrdiff_t tokenize_file(struct glcpg_token **tokens, FILE *file);
void      token_stream_free(struct glcpg_token **tokens);

size_t token_stream_len(struct glcpg_token *tokens);