#pragma once
#include <stdio.h>

struct token_constant
{
    enum
    {
        E_CONST_INTEGER,
        E_CONST_DECIMAL,
        E_CONST_STRING,
    } type;

    union
    {
        long integer;
        struct
        {
            long          integer;
            unsigned long fractional;
        } decimal;
        const char *string;
    } value;
};

struct token_identifier
{
    // TODO: Include Namespace information
    const char *identifier;
};

enum token_keyword
{
    E_KEYWORD_PUBLIC,
    E_KEYWORD_CONST,

    E_KEYWORD_FUNCTION,
    E_KEYWORD_STRUCT,
    E_KEYWORD_TRAIT,
    E_KEYWORD_ENUM,

    E_KEYWORD_IMPLEMENTS,
    E_KEYWORD_EXTERN,

    E_KEYWORD_LET,
    E_KEYWORD_AS,

    E_KEYWORD_IF,
    E_KEYWORD_MATCH,
};

struct token
{
    enum
    {
        E_TOKEN_CONSTANT   = -1,
        E_TOKEN_IDENTIFIER = -2,
        E_TOKEN_KEYWORD    = -3,
    } value;    // For unknown ASCII chars; will be value of char

    union
    {
        struct token_constant   constant;
        struct token_identifier identifier;
        enum token_keyword      keyword;
    } data;
};

struct token_stream
{
    struct token *tokens;

    size_t capacity;
    size_t size;
};

enum token_error
{
    E_INVALID_IDENTIFIER = -1,
    E_MEMORYERROR        = -2,
    E_IOERROR            = -3,
};

enum token_error tokenize_file(struct token_stream *tokens, FILE *file);
void             token_stream_free(struct token_stream *tokens);