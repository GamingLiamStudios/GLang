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

void token_stream_free(struct glcpg_token **stream)
{
    struct glcpg_token *next;

    if (stream == NULL || *stream == NULL) { return; }

    next = *stream;
    while (next->value != E_TOKEN_EOF)
    {
        switch (next->value)
        {
        case E_TOKEN_IDENT:
        case E_TOKEN_STRING: free((char *) next->data.string);

        default: next += 1;
        }
    }

    free(*stream);
    *stream = NULL;
}

void token_debug(FILE *restrict file, struct glcpg_token *restrict tok)
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

int token_debug_str(char *restrict string, size_t length, struct glcpg_token *restrict tok)
{
    if (string == NULL || tok == NULL || length == 0) { return 0; }
    switch (tok->value)
    {
    case E_TOKEN_INTEGER: return snprintf(string, length, "Integer(%lu)", tok->data.integer);
    case E_TOKEN_STRING: return snprintf(string, length, "String(\"%s\")", tok->data.string);
    case E_TOKEN_IDENT: return snprintf(string, length, "Ident(\"%s\")", tok->data.string);
    // TODO: too lazy to impl this rn lol
    case E_TOKEN_EOF: return 0;
    }

    snprintf(string, length, "Unknown(%d)", tok->value);
}

size_t token_stream_len(struct glcpg_token *tokens)
{
    struct glcpg_token *start;

    if (tokens == NULL) { return 0; }

    start = tokens;
    while ((tokens++)->value != E_TOKEN_EOF) { }
    return tokens - start;
}

ptrdiff_t tokenize_file(struct glcpg_token **result, FILE *file)
{
    if (result == NULL) { return E_TOK_INVALIDINPUT; }

    char             *token_buffer;
    size_t            buffer_len;
    struct debug_info debug_info;
    char              c, prev;

    struct glcpg_token *stream;
    size_t              capacity;

    // Create initial token stream
    *result = calloc(TOKENSTREAM_CAPACITY, sizeof(struct glcpg_token));

    stream   = *result;
    capacity = TOKENSTREAM_CAPACITY;

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
            if (stream->value == E_TOKEN_STRING)
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

                        (stream++)->data.string = string;
                        memset(token_buffer, 0, buffer_len);
                        buffer_len = 0;
                    }
                    continue;
                }

                token_buffer[buffer_len++] = c;
                continue;
            }

            if (stream->value == E_TOKEN_INTEGER)
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
                        struct debug_info info = stream->debug_info;
                        glc_log(
                          E_WARN,
                          "Value %s at %d:%d is too long; Clamped to %lu\n",
                          token_buffer,
                          info.line,
                          info.column,
                          ULONG_MAX);
                    }
                    (stream++)->data.integer = value;

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
                        (stream++)->value = E_TOKEN_PUBLIC;
                    }
                    else if (strncmp(token_buffer, "const", buffer_len) == 0)
                    {
                        (stream++)->value = E_TOKEN_CONST;
                    }
                    else if (strncmp(token_buffer, "fn", buffer_len) == 0)
                    {
                        (stream++)->value = E_TOKEN_FN;
                    }
                    else if (strncmp(token_buffer, "struct", buffer_len) == 0)
                    {
                        (stream++)->value = E_TOKEN_STRUCT;
                    }
                    else if (strncmp(token_buffer, "trait", buffer_len) == 0)
                    {
                        (stream++)->value = E_TOKEN_TRAIT;
                    }
                    else if (strncmp(token_buffer, "enum", buffer_len) == 0)
                    {
                        (stream++)->value = E_TOKEN_ENUM;
                    }
                    else if (strncmp(token_buffer, "impl", buffer_len) == 0)
                    {
                        (stream++)->value = E_TOKEN_IMPL;
                    }
                    else if (strncmp(token_buffer, "extern", buffer_len) == 0)
                    {
                        (stream++)->value = E_TOKEN_EXTERN;
                    }
                    else if (strncmp(token_buffer, "let", buffer_len) == 0)
                    {
                        (stream++)->value = E_TOKEN_LET;
                    }
                    else if (strncmp(token_buffer, "as", buffer_len) == 0)
                    {
                        (stream++)->value = E_TOKEN_AS;
                    }
                    else if (strncmp(token_buffer, "if", buffer_len) == 0)
                    {
                        (stream++)->value = E_TOKEN_IF;
                    }
                    else if (strncmp(token_buffer, "else", buffer_len) == 0)
                    {
                        (stream++)->value = E_TOKEN_ELSE;
                    }
                    else if (strncmp(token_buffer, "match", buffer_len) == 0)
                    {
                        (stream++)->value = E_TOKEN_MATCH;
                    }
                    else if (strncmp(token_buffer, "while", buffer_len) == 0)
                    {
                        (stream++)->value = E_TOKEN_WHILE;
                    }
                    else if (strncmp(token_buffer, "loop", buffer_len) == 0)
                    {
                        (stream++)->value = E_TOKEN_LOOP;
                    }
                    else if (strncmp(token_buffer, "break", buffer_len) == 0)
                    {
                        (stream++)->value = E_TOKEN_BREAK;
                    }
                    else if (strncmp(token_buffer, "continue", buffer_len) == 0)
                    {
                        (stream++)->value = E_TOKEN_CONTINUE;
                    }
                    else
                    {
                        char *string = calloc(buffer_len + 1, sizeof(char));
                        memcpy(string, token_buffer, buffer_len + 1);

                        (stream++)->data.string = string;
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

        if (capacity == stream - *result)
        {
            capacity *= TOKENSTREAM_GROWTHFACTOR;

            ptrdiff_t len = stream - *result;

            struct glcpg_token *resized = realloc(*result, sizeof(struct glcpg_token) * capacity);
            if (resized == NULL)
            {
                token_stream_free(result);
                return E_TOK_MEMORYERROR;
            }

            *result = resized;
            stream  = *result + len;
        }

        stream->debug_info = debug_info;

        // Start new token
        if (isdigit(c))
        {
            stream->value              = E_TOKEN_INTEGER;
            token_buffer[buffer_len++] = c;
            continue;
        }
        if (isalpha(c) || c == '_')
        {
            stream->value              = E_TOKEN_IDENT;
            token_buffer[buffer_len++] = c;
            continue;
        }
        if (c == '"')
        {
            stream->value              = E_TOKEN_STRING;
            token_buffer[buffer_len++] = c;
            continue;
        }

        if (isspace(c))
        {
            prev = c;
            continue;
        }

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

        case ';': result = E_TOKEN_SEMICOLON; break;
        case ',': result = E_TOKEN_COMMA; break;
        case '.': result = E_TOKEN_POINT; break;
        case ':': result = E_TOKEN_COLON; break;

        case '?': result = E_TOKEN_QUESTION; break;
        case '~': result = E_TOKEN_TILDE; break;
        case '^': result = E_TOKEN_CARET; break;

        case '+':
            result = E_TOKEN_PLUS;
            prev   = '+';
            break;
        case '-':
            result = E_TOKEN_MINUS;
            prev   = '-';
            break;
        case '*':
            result = E_TOKEN_STAR;
            prev   = '*';
            break;
        case '/':
            result = E_TOKEN_SLASH;
            prev   = '/';
            break;

        case '<':
            result = E_TOKEN_LT;
            prev   = '<';
            break;
        case '!':
            result = E_TOKEN_EXMARK;
            prev   = '!';
            break;

        case '>':
            if (prev == '-')
            {
                stream -= 1;
                result = E_TOKEN_IS;
            }
            else { result = E_TOKEN_GT; }
            prev = '-';
            break;
        case '&':
            if (prev == '&')
            {
                stream -= 1;
                result = E_TOKEN_ANDAND;
            }
            else { result = E_TOKEN_AND; }
            prev = '&';
            break;
        case '|':
            if (prev == '|')
            {
                stream -= 1;
                result = E_TOKEN_BARBAR;
            }
            else { result = E_TOKEN_BAR; }
            prev = '|';
            break;

        case '=':
        {
            char prevprev = prev;

            switch (prevprev)
            {
            default:
                result            = E_TOKEN_EQUAL;
                prev              = '=';
                (stream++)->value = result;
                continue;

            case '=': result = E_TOKEN_EQEQ; break;
            case '!': result = E_TOKEN_NEQ; break;
            case '<': result = E_TOKEN_LEQ; break;
            case '>': result = E_TOKEN_GEQ; break;

            case '+': result = E_TOKEN_PLUSEQUAL; break;
            case '-': result = E_TOKEN_MINUSEQUAL; break;
            case '*': result = E_TOKEN_STAREQUAL; break;
            case '/': result = E_TOKEN_SLASHEQUAL; break;

            case '|': result = E_TOKEN_BAREQUAL; break;
            case '&': result = E_TOKEN_ANDEQUAL; break;
            }

            stream -= 1;
            prev = ' ';
            break;
        }
        default: prev = ' ';
        }

        // Unknown token
        (stream++)->value = result;
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

    if (capacity == stream - *result)
    {
        capacity *= TOKENSTREAM_GROWTHFACTOR;

        ptrdiff_t len = stream - *result;

        struct glcpg_token *resized = realloc(*result, sizeof(struct glcpg_token) * capacity);
        if (resized == NULL)
        {
            token_stream_free(result);
            return E_TOK_MEMORYERROR;
        }

        *result = resized;
        stream  = *result + len;
    }

    (stream++)->value = E_TOKEN_EOF;

    return stream - *result;
}