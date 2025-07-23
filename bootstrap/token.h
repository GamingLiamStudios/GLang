#pragma once
#include <stdio.h>

struct debug_info
{
    int line;
    int column;
};

struct token
{
    enum
    {
        E_TOKEN_EOF = -255,

        E_TOKEN_INTEGER,
        E_TOKEN_STRING,
        E_TOKEN_IDENTIFIER,

        E_TOKEN_PUBLIC,    // pub
        E_TOKEN_CONST,     // const

        E_TOKEN_FUNCTION,    // fn
        E_TOKEN_STRUCT,      // struct
        E_TOKEN_TRAIT,       // trait
        E_TOKEN_ENUM,        // enum

        E_TOKEN_IMPLEMENTS,    // impl
        E_TOKEN_EXTERN,        // extern

        E_TOKEN_LET,    // let
        E_TOKEN_AS,     // as

        E_TOKEN_IF,       // if
        E_TOKEN_ELSE,     // else
        E_TOKEN_MATCH,    // match
    } value;              // For unknown ASCII chars; will be value of char

    union
    {
        unsigned long integer;
        const char   *string;
    } data;

    struct debug_info debug_info;
};

struct token_stream
{
    struct token *tokens;

    size_t capacity;
    size_t size;
};

enum token_error
{
    E_MEMORYERROR = -255,
    E_IOERROR,
};

int  tokenize_file(struct token_stream *tokens, FILE *file);
void token_stream_free(struct token_stream *tokens);