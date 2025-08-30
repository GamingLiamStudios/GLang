#include "parse-gen/token.h"
#include "parse-gen/tree.h"
#include "parse-gen/log.h"
#include "parse-gen/table.h"
#include "parse-gen/error.h"

#include "token.h"

#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

enum LogLevel verbosity = E_DEBUG;

struct glcpg_parseitem
{
    size_t             state_id;
    struct glcpg_token lookahead;
};

int __alloc_parse_items(struct glcpg_parseitem **items, size_t capacity)
{
    struct glcpg_parseitem *newptr;
    if (items == NULL) { return E_GLCPG_INVALIDINPUT; }

    if (capacity == 0)
    {
        free(*items);
        *items = NULL;
        return 0;
    }

    newptr = realloc(*items, sizeof(const char **) * capacity);
    if (newptr == NULL)
    {
        free(*items);
        *items = NULL;
        return E_GLCPG_MEMORYERROR;
    }

    *items = newptr;
    return 0;
}

int main(int argc, char **argv)
{
    int                  ret;
    struct glcpg_grammar grammar = {};

    const char *filepath     = "parse-gen/expr_test/expr.pg";
    const char *parse_string = "1 + 2 * (3 + 4)";

    // First, tokenize all the files
    FILE *file = fopen(filepath, "r");
    if (file == NULL)
    {
        glc_log(E_ERROR, "Error while opening file: %s\n", strerror(errno));
        return errno;
    }

    struct glcpg_token *tokens;
    ptrdiff_t           len = glcpg_lexer_file(&tokens, file);
    if (len == 0)
    {
        glc_log(E_ERROR, "Failed to parse file '%s'\n", filepath);
        return -1;
    }

    ret = glcpg_parse(&grammar, tokens);
    if (ret < 0)
    {
        glc_log(E_ERROR, "Failed to parse file '%s'\n", filepath);
        return ret;
    }

    fclose(file);
    glcpg_grammar_classify(&grammar);

    struct glcpg_actiontable_entry *table = NULL;
    ret                                   = glcpg_table_create(&table, "Expr", &grammar);
    if (ret < 0)
    {
        glcpg_grammar_free(&grammar);
        return ret;
    }

    // Parse string
    struct expr_token *expr_tok_stream = NULL;
    len                                = expr_lexer_string(&expr_tok_stream, parse_string);
    if (len < 0) { return len; }

    glc_log(E_INFO, "%lu Tokens\n", len);

    /*
    size_t                  capacity = 8;
    size_t                  size     = 0;
    struct glcpg_parseitem *stack    = NULL;
    ret                              = __alloc_parse_items(&stack, capacity);
    if (ret < 0) { return ret; }

    stack[0].state_id  = 0;
    stack[0].lookahead = *(parse_stream++);

    while (1)
    {
        if (size == capacity)
        {
            capacity *= 2;
            ret = __alloc_parse_items(&stack, capacity);
            if (ret < 0) { return ret; }
        }

        struct glcpg_parseitem *front = stack + size;

        struct glcpg_actiontable_entry *actions = table + front->state_id * grammar.num_items;

        size_t i;
        for (i = 0; i < grammar.num_items; i++)
        {
            struct glcpg_item itm = grammar.items[i];
            printf("%s\n", itm.value);
            if (
              front->lookahead.type != E_PGITM_EOF &&
              strcmp(front->lookahead.value.ident, itm.value) == 0)
            {
                break;
            }
        }
        if (i >= grammar.num_items)
        {
            char tok_dbg[1024];
            glcpg_token_debug(tok_dbg, 1024, front->lookahead);
            glc_log(E_ERROR, "Token doesn't exist in grammar! %s\n", tok_dbg);
            return -1;
        }

        switch (actions[i].action)
        {
        case E_PGACT_INVALID: glc_log(E_ERROR, "Invalid State Transition!\n"); return -1;
        case E_PGACT_ACCEPT: break;
        case E_PGACT_REDUCE:
            glc_log(E_DEBUG, "Reduce!\n");
            return -1;
            // TODO: Grammar reduction (AST Generation)

        case E_PGACT_GOTO: front->state_id = actions[i].next_state; break;

        case E_PGACT_SHIFT:
            size++;
            stack[size].state_id  = actions[i].next_state;
            stack[size].lookahead = *(parse_stream++);
            break;
        }
    }
        */

    free(table);

    // Free everything...
    glcpg_grammar_free(&grammar);
}