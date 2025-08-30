#include "token.h"

#include "parse-gen/log.h"

#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <limits.h>
#include <errno.h>

#include <stdio.h>

int __alloc_expr_tokens(struct expr_token **tokens, size_t capacity)
{
    struct expr_token *new_ptr;
    if (tokens == NULL) return 1;

    if (capacity == 0)
    {
        free(*tokens);
        *tokens = NULL;
        return 0;
    }

    new_ptr = realloc(*tokens, sizeof(struct expr_token) * capacity);
    if (new_ptr == NULL)
    {
        glc_log(
          E_ERROR,
          "Failed to alloc memory (%lu bytes)\n",
          sizeof(struct expr_token) * capacity);

        free(*tokens);
        *tokens = NULL;
        return 1;
    }

    *tokens = new_ptr;
    return 0;
}

ptrdiff_t expr_lexer_string(struct expr_token **result, const char *input)
{
    if (result == NULL || input == NULL) return -1;

    size_t             capacity = 8;
    size_t             items    = 0;
    struct expr_token *tokens   = NULL;
    if (__alloc_expr_tokens(&tokens, capacity)) return -1;

    const char *number_start = NULL;
    char        tok;
    while ((tok = *(input++)) != '\0')
    {
        if (capacity <= (items + 1))
        {
            capacity *= 2;
            if (__alloc_expr_tokens(&tokens, capacity)) return -1;
        }

        if (isdigit(tok))
        {
            if (number_start == NULL) number_start = input - 1;
            continue;
        }

        if (number_start != NULL)
        {
            // Parse number
            unsigned long val = strtoul(number_start, (char **) &input, 10);
            if (val == ULONG_MAX && errno == ERANGE)
            {
                glc_log(E_ERROR, "Out of Range\n");

                free(tokens);
                return -1;
            }

            tokens[items++] = (struct expr_token) {
                .type  = E_ETOK_NUM,
                .value = val,
            };
            number_start = NULL;
        }

        if (isspace(tok)) continue;

        switch (tok)
        {
        case '(': tokens[items++].type = E_ETOK_LBRACKET; break;
        case ')': tokens[items++].type = E_ETOK_RBRACKET; break;
        case '-': tokens[items++].type = E_ETOK_MINUS; break;
        case '!': tokens[items++].type = E_ETOK_EXMARK; break;
        case '+': tokens[items++].type = E_ETOK_PLUS; break;
        case '*': tokens[items++].type = E_ETOK_STAR; break;
        case '/': tokens[items++].type = E_ETOK_SLASH; break;
        case '>': tokens[items++].type = E_ETOK_GT; break;
        case '<': tokens[items++].type = E_ETOK_LT; break;

        case '=':
            if (*(input++) != '=')
            {
                glc_log(E_ERROR, "Unknown token %c\n", tok);
                free(tokens);
                return -1;
            }

            tokens[items++].type = E_ETOK_EQEQ;
            break;
        case '&':
            if (*(input++) != '&')
            {
                glc_log(E_ERROR, "Unknown token %c\n", tok);
                free(tokens);
                return -1;
            }

            tokens[items++].type = E_ETOK_EQEQ;
            break;
        case '|':
            if (*(input++) != '|')
            {
                glc_log(E_ERROR, "Unknown token %c\n", tok);
                free(tokens);
                return -1;
            }

            tokens[items++].type = E_ETOK_EQEQ;
            break;

        default:
            glc_log(E_ERROR, "Unknown token %c\n", tok);
            free(tokens);
            return -1;
        }
    }

    if (__alloc_expr_tokens(&tokens, items)) return -1;
    *result = tokens;
    return items;
}