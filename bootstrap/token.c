#include "token.h"
#include "log.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <errno.h>

#define TOKENSTREAM_CAPACITY     256
#define TOKENSTREAM_GROWTHFACTOR 2

#define TOKENBUFFER_SIZE 1024

void token_stream_free(struct token_stream *tokens)
{
    free(tokens->tokens);

    tokens->capacity = 0;
    tokens->size     = 0;
    tokens->tokens   = NULL;
}

int token_stream_expand(struct token_stream *tokens)
{
    tokens->capacity *= TOKENSTREAM_GROWTHFACTOR;

    struct token *resized = realloc(tokens->tokens, sizeof(struct token) * tokens->capacity);
    if (resized == NULL)
    {
        token_stream_free(tokens);
        return E_MEMORYERROR;
    }

    return 0;
}

int tokenize_file(struct token_stream *tokens, FILE *file)
{
    // Create initial token stream
    tokens->tokens   = calloc(TOKENSTREAM_CAPACITY, sizeof(struct token));
    tokens->capacity = TOKENSTREAM_CAPACITY;
    tokens->size     = 0;

    char *token_buffer = malloc(TOKENBUFFER_SIZE);
    memset(token_buffer, 0, TOKENBUFFER_SIZE);
    size_t buffer_len = 0;

    struct debug_info debug_info = {
        .line   = 1,
        .column = 0,
    };

    char c;
    while ((c = fgetc(file)) != EOF)
    {
        debug_info.column += 1;
        if (c == '\n')
        {
            debug_info.line += 1;
            debug_info.column = 0;
        }

        // TODO: Support (ignore) Comments

        if (buffer_len > 0)
        {
            // Continue along adding to token_buffer
            if (tokens->tokens[tokens->size].value == E_TOKEN_STRING)
            {
                if (c == '"')
                {
                    if (buffer_len >= 1 && token_buffer[buffer_len - 1] == '\\')
                    {
                        // Store unescaped string
                        token_buffer[buffer_len - 1] = '"';
                    }
                    else
                    {
                        // Finish current token

                        char *string = malloc(buffer_len);
                        memcpy(string, token_buffer + 1, buffer_len);

                        tokens->tokens[tokens->size++].data.string = string;
                        memset(token_buffer, 0, buffer_len);
                        buffer_len = 0;
                    }
                    continue;
                }

                token_buffer[buffer_len++] = c;
                continue;
            }

            if (tokens->tokens[tokens->size].value == E_TOKEN_INTEGER)
            {
                // TODO: Support Hex integer constants
                if (!isdigit(c))
                {
                    // Finish current token

                    // Parse string as integer
                    char         *last_value = NULL;
                    unsigned long value      = strtoul(token_buffer, &last_value, 10);
                    if (value == ULONG_MAX && errno == ERANGE)
                    {
                        struct debug_info info = tokens->tokens[tokens->size++].debug_info;
                        glc_log(
                          E_WARN,
                          "Value %s at %d:%d is too long; Clamped to %lu\n",
                          token_buffer,
                          info.line,
                          info.column,
                          ULONG_MAX);
                    }
                    tokens->tokens[tokens->size++].data.integer = value;

                    memset(token_buffer, 0, buffer_len);
                    buffer_len = 0;
                }
                else
                {
                    token_buffer[buffer_len++] = c;
                    continue;
                }
            }
            else
            {
                if (!(isalnum(c) || c == '_'))
                {
                    // Finish current token

                    // Check against keywords
                    if (strncmp(token_buffer, "pub", buffer_len) == 0)
                    {
                        tokens->tokens[tokens->size++].value = E_TOKEN_PUBLIC;
                    }
                    else if (strncmp(token_buffer, "const", buffer_len) == 0)
                    {
                        tokens->tokens[tokens->size++].value = E_TOKEN_CONST;
                    }
                    else if (strncmp(token_buffer, "fn", buffer_len) == 0)
                    {
                        tokens->tokens[tokens->size++].value = E_TOKEN_FUNCTION;
                    }
                    else if (strncmp(token_buffer, "struct", buffer_len) == 0)
                    {
                        tokens->tokens[tokens->size++].value = E_TOKEN_STRUCT;
                    }
                    else if (strncmp(token_buffer, "trait", buffer_len) == 0)
                    {
                        tokens->tokens[tokens->size++].value = E_TOKEN_TRAIT;
                    }
                    else if (strncmp(token_buffer, "enum", buffer_len) == 0)
                    {
                        tokens->tokens[tokens->size++].value = E_TOKEN_ENUM;
                    }
                    else if (strncmp(token_buffer, "impl", buffer_len) == 0)
                    {
                        tokens->tokens[tokens->size++].value = E_TOKEN_IMPLEMENTS;
                    }
                    else if (strncmp(token_buffer, "extern", buffer_len) == 0)
                    {
                        tokens->tokens[tokens->size++].value = E_TOKEN_EXTERN;
                    }
                    else if (strncmp(token_buffer, "let", buffer_len) == 0)
                    {
                        tokens->tokens[tokens->size++].value = E_TOKEN_LET;
                    }
                    else if (strncmp(token_buffer, "as", buffer_len) == 0)
                    {
                        tokens->tokens[tokens->size++].value = E_TOKEN_AS;
                    }
                    else if (strncmp(token_buffer, "if", buffer_len) == 0)
                    {
                        tokens->tokens[tokens->size++].value = E_TOKEN_IF;
                    }
                    else if (strncmp(token_buffer, "match", buffer_len) == 0)
                    {
                        tokens->tokens[tokens->size++].value = E_TOKEN_MATCH;
                    }
                    else
                    {
                        char *string = malloc(buffer_len + 1);
                        memcpy(string, token_buffer, buffer_len + 1);

                        tokens->tokens[tokens->size++].data.string = string;
                    }

                    memset(token_buffer, 0, buffer_len);
                    buffer_len = 0;
                }
                else
                {
                    token_buffer[buffer_len++] = c;
                    continue;
                }
            }
        }

        if (tokens->capacity == tokens->size)
        {
            int result = token_stream_expand(tokens);
            if (result < 0) { return result; }
        }

        tokens->tokens[tokens->size].debug_info = debug_info;

        // Start new token
        if (isdigit(c))
        {
            tokens->tokens[tokens->size].value = E_TOKEN_INTEGER;
            token_buffer[buffer_len++]         = c;
            continue;
        }
        if (isalpha(c))
        {
            tokens->tokens[tokens->size].value = E_TOKEN_IDENTIFIER;
            token_buffer[buffer_len++]         = c;
            continue;
        }
        if (c == '"')
        {
            tokens->tokens[tokens->size].value = E_TOKEN_STRING;
            token_buffer[buffer_len++]         = c;
            continue;
        }

        if (isspace(c)) { continue; }

        // Unknown token
        tokens->tokens[tokens->size].value = c;
        tokens->size++;
    }
    free(token_buffer);

    return 0;
}