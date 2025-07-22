#include "token.h"

#include <stdlib.h>

#define TOKENSTREAM_CAPACITY 256

void token_stream_free(struct token_stream *tokens)
{
    free(tokens->tokens);

    tokens->capacity = 0;
    tokens->size     = 0;
    tokens->tokens   = NULL;
}

enum token_error token_stream_expand(struct token_stream *tokens)
{
    tokens->capacity *= 2;

    struct token *resized = realloc(tokens->tokens, sizeof(struct token) * tokens->capacity);
    if (resized == NULL)
    {
        token_stream_free(tokens);
        return E_MEMORYERROR;
    }
}

enum token_error tokenize_file(struct token_stream *tokens, FILE *file)
{
    // Create initial token stream
    tokens->tokens   = calloc(TOKENSTREAM_CAPACITY, sizeof(struct token));
    tokens->capacity = TOKENSTREAM_CAPACITY;
    tokens->size     = 0;
}