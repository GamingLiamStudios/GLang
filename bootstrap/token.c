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
    if (tokens == NULL || tokens->tokens == NULL) { return; }
    for (size_t i = 0; i < tokens->size; i++)
    {
        struct token token = tokens->tokens[i];
        switch (token.value)
        {
        case E_TOKEN_IDENT:
        case E_TOKEN_STRING: free((char *) token.data.string);

        default: continue;
        }
    }

    free(tokens->tokens);

    tokens->capacity = 0;
    tokens->size     = 0;
    tokens->tokens   = NULL;
}

int token_stream_expand(struct token_stream *tokens)
{
    if (tokens == NULL) { return E_TOK_INVALIDINPUT; }
    tokens->capacity *= TOKENSTREAM_GROWTHFACTOR;

    struct token *resized = realloc(tokens->tokens, sizeof(struct token) * tokens->capacity);
    if (resized == NULL)
    {
        token_stream_free(tokens);
        return E_TOK_MEMORYERROR;
    }

    return 0;
}

void token_debug(FILE *restrict file, struct token *restrict tok)
{
    if (file == NULL || tok == NULL) { return; }
    switch (tok->value)
    {
    case E_TOKEN_INTEGER: printf("Integer(%lu)", tok->data.integer); return;
    case E_TOKEN_STRING: printf("String(\"%s\")", tok->data.string); return;
    case E_TOKEN_IDENT: printf("Ident(\"%s\")", tok->data.string); return;
    // TODO: too lazy to impl this rn lol
    case E_TOKEN_EOF: return;
    }

    printf("Unknown(%d)", tok->value);
}

int tokenize_file(struct token_stream *tokens, FILE *file)
{
    if (tokens == NULL) { return E_TOK_INVALIDINPUT; }

    char             *token_buffer;
    size_t            buffer_len;
    struct debug_info debug_info;
    char              c, prev;

    // Create initial token stream
    tokens->tokens   = calloc(TOKENSTREAM_CAPACITY, sizeof(struct token));
    tokens->capacity = TOKENSTREAM_CAPACITY;
    tokens->size     = 0;

    token_buffer = calloc(TOKENBUFFER_SIZE, sizeof(char));
    buffer_len   = 0;

    debug_info = (struct debug_info) {
        .line   = 1,
        .column = 0,
    };

    c = prev = ' ';

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

                        char *string = calloc(buffer_len + 1, sizeof(char));
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
                    // ...hashmap maybe?
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
                        tokens->tokens[tokens->size++].value = E_TOKEN_FN;
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
                        tokens->tokens[tokens->size++].value = E_TOKEN_IMPL;
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
                    else if (strncmp(token_buffer, "else", buffer_len) == 0)
                    {
                        tokens->tokens[tokens->size++].value = E_TOKEN_ELSE;
                    }
                    else if (strncmp(token_buffer, "match", buffer_len) == 0)
                    {
                        tokens->tokens[tokens->size++].value = E_TOKEN_MATCH;
                    }
                    else if (strncmp(token_buffer, "while", buffer_len) == 0)
                    {
                        tokens->tokens[tokens->size++].value = E_TOKEN_WHILE;
                    }
                    else if (strncmp(token_buffer, "loop", buffer_len) == 0)
                    {
                        tokens->tokens[tokens->size++].value = E_TOKEN_LOOP;
                    }
                    else if (strncmp(token_buffer, "break", buffer_len) == 0)
                    {
                        tokens->tokens[tokens->size++].value = E_TOKEN_BREAK;
                    }
                    else if (strncmp(token_buffer, "continue", buffer_len) == 0)
                    {
                        tokens->tokens[tokens->size++].value = E_TOKEN_CONTINUE;
                    }
                    else
                    {
                        char *string = calloc(buffer_len + 1, sizeof(char));
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
        if (isalpha(c) || c == '_')
        {
            tokens->tokens[tokens->size].value = E_TOKEN_IDENT;
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

        // Check against known tokens
        int result = c;
        switch (c)
        {
        case '(': result = E_TOKEN_LPAREN; break;
        case ')': result = E_TOKEN_RPAREN; break;
        case '{': result = E_TOKEN_LBRACE; break;
        case '}': result = E_TOKEN_RBRACE; break;
        case '[': result = E_TOKEN_LBRACK; break;
        case ']': result = E_TOKEN_RBRACK; break;

        case '+': result = E_TOKEN_PLUS; break;
        case '-': result = E_TOKEN_MINUS; break;
        case '*': result = E_TOKEN_STAR; break;
        case '/': result = E_TOKEN_SLASH; break;

        case ';': result = E_TOKEN_SEMICOLON; break;
        case ',': result = E_TOKEN_COMMA; break;
        case '.': result = E_TOKEN_POINT; break;
        case ':': result = E_TOKEN_COLON; break;

        case '?': result = E_TOKEN_QUESTION; break;
        case '~': result = E_TOKEN_TILDE; break;
        case '^': result = E_TOKEN_CARET; break;

        case '<': result = E_TOKEN_LT; break;
        case '!': result = E_TOKEN_EXMARK; break;

        case '>':
            if (prev == '-')
            {
                tokens->size--;
                result = E_TOKEN_IS;
            }
            else { result = E_TOKEN_GT; }
            break;
        case '&':
            if (prev == '&')
            {
                tokens->size--;
                result = E_TOKEN_ANDAND;
            }
            else { result = E_TOKEN_AND; }
            break;
        case '|':
            if (prev == '|')
            {
                tokens->size--;
                result = E_TOKEN_BARBAR;
            }
            else { result = E_TOKEN_BAR; }
            break;

        case '=':
            if (prev == '=')
            {
                tokens->size--;
                result = E_TOKEN_EQEQ;
            }
            else if (prev == '!')
            {
                tokens->size--;
                result = E_TOKEN_NEQ;
            }
            else if (prev == '<')
            {
                tokens->size--;
                result = E_TOKEN_LEQ;
            }
            else if (prev == '!')
            {
                tokens->size--;
                result = E_TOKEN_GEQ;
            }
            else { result = E_TOKEN_EQUAL; }
            break;
        }

        // Unknown token
        tokens->tokens[tokens->size].value = result;
        tokens->size++;

        prev = c;
    }

    if (buffer_len != 0)
    {
        glc_log(
          E_WARN,
          "EOF found at %d:%d; %s still in parsing buffer\n",
          debug_info.line,
          debug_info.column,
          token_buffer);
    }

    free(token_buffer);

    if (tokens->capacity == tokens->size)
    {
        int result = token_stream_expand(tokens);
        if (result < 0) { return result; }
    }

    tokens->tokens[tokens->size++].value = E_TOKEN_EOF;
    return 0;
}