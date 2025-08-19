#pragma once

#include <stddef.h>
#include <stdint.h>
#include "token.h"

struct glcpg_item
{
    enum
    {
        E_PGITM_TERMINAL,
        E_PGITM_NONTERMINAL,
        E_PGITM_EOF,
    } type;

    const char *value;
};

struct glcpg_rule
{
    size_t     num_items;
    ptrdiff_t *item;
};

struct glcpg_ruletable
{
    const char *name;

    size_t             num_rules;
    struct glcpg_rule *rules;
};

struct glcpg_grammar
{
    struct glcpg_item *items;
    size_t             num_items;

    size_t                  num_nonterminals;
    struct glcpg_ruletable *nonterminals;
};

/// Returns number of unique rules, or negative value on error.
int glcpg_parse(struct glcpg_grammar *result, struct glcpg_token *tokens);

int  glcpg_grammar_merge(struct glcpg_grammar *target, struct glcpg_grammar *source);
void glcpg_grammar_classify(struct glcpg_grammar *target);

void glcpg_grammar_free(struct glcpg_grammar *grammar);