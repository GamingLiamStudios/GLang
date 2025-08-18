#pragma once
#include <stdio.h>
#include <stddef.h>

struct glcpg_token
{
    enum
    {
        E_PGTOK_EOF = 0,
        E_PGTOK_EOL,

        E_PGTOK_IDENT,
        E_PGTOK_EQUAL,
    } type;

    union
    {
        const char *ident;
    } value;
};

/// Returns length of returned string, or negative for error.
/// Up to `maxlen - 1` chars will be written. Will still return calculated length if `string` is
/// NULL and `maxlen` is zero. Will still return the required length even if `maxlen - 1 > length`.
int glcpg_token_debug(char *restrict string, size_t maxlen, const struct glcpg_token token);

/// Returns number of tokens parsed, or negative for error
ptrdiff_t glcpg_lexer_file(struct glcpg_token *restrict *result, FILE *file);

void glcpg_token_free(struct glcpg_token *token);