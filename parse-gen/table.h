#pragma once

#include "tree.h"

#include <stddef.h>

struct glcpg_parseitem_0
{
    const char       *result;
    struct glcpg_rule rule;

    size_t parse_idx;
};

struct glcpg_itemset_0
{
    struct glcpg_parseitem_0 *items;
    size_t                    num_items;
};

struct glcpg_firstset
{
    const char *key;

    struct glcpg_item *first;
    size_t             count;
};

struct glcpg_table
{
    struct glcpg_itemset_0 *sets;
    size_t                  num_sets;

    size_t lookahead;
};

ptrdiff_t
  glcpg_table_create(struct glcpg_table *result, size_t lookahead, struct glcpg_grammar *grammar);