#include "table.h"
#include "error.h"
#include "log.h"
#include "error.h"

#include <stdlib.h>
#include <string.h>

#define ITEMSETS_INITCAPACITY 64
#define ITEMSETS_GROWTHFACTOR 2

#define PARSEITEMS_INITCAPACITY 64
#define PARSEITEMS_GROWTHFACTOR 2

int __alloc_parseitems(struct glcpg_parseitem_0 **ptr, size_t capacity)
{
    struct glcpg_parseitem_0 *newptr;
    if (ptr == NULL) { return E_GLCPG_INVALIDINPUT; }

    if (capacity == 0)
    {
        free(*ptr);
        *ptr = NULL;
        return 0;
    }

    newptr = realloc(*ptr, sizeof(struct glcpg_parseitem_0) * capacity);
    if (newptr == NULL)
    {
        free(*ptr);
        *ptr = NULL;
        return E_GLCPG_MEMORYERROR;
    }

    *ptr = newptr;
    return 0;
}

int __alloc_lr0_itemsets(struct glcpg_itemset_0 **ptr, size_t capacity)
{
    struct glcpg_itemset_0 *newptr;
    if (ptr == NULL) { return E_GLCPG_INVALIDINPUT; }

    if (capacity == 0)
    {
        free(*ptr);
        *ptr = NULL;
        return 0;
    }

    newptr = realloc(*ptr, sizeof(struct glcpg_itemset_0) * capacity);
    if (newptr == NULL)
    {
        free(*ptr);
        *ptr = NULL;
        return E_GLCPG_MEMORYERROR;
    }

    *ptr = newptr;
    return 0;
}

int __alloc_items2(struct glcpg_item **ptr, size_t capacity)
{
    struct glcpg_item *newptr;
    if (ptr == NULL) { return E_GLCPG_INVALIDINPUT; }

    if (capacity == 0)
    {
        free(*ptr);
        *ptr = NULL;
        return 0;
    }

    newptr = realloc(*ptr, sizeof(struct glcpg_item) * capacity);
    if (newptr == NULL)
    {
        // FIXME: Leaky leaky
        free(*ptr);
        *ptr = NULL;
        return E_GLCPG_MEMORYERROR;
    }

    *ptr = newptr;
    return 0;
}

int __compare_parseitem(
  struct glcpg_parseitem_0 *lhs,
  struct glcpg_parseitem_0 *rhs,
  struct glcpg_grammar     *grammar)
{
    if (lhs == NULL || rhs == NULL) { return 0; }

    if (lhs->parse_idx != rhs->parse_idx) { return 0; }
    if (strcmp(lhs->result, rhs->result) != 0) { return 0; }

    if (lhs->rule.num_items != rhs->rule.num_items) { return 0; }
    for (size_t i = 0; i < lhs->rule.num_items; i++)
    {
        ptrdiff_t lhs_idx = lhs->rule.item[i];
        ptrdiff_t rhs_idx = rhs->rule.item[i];
        if (lhs_idx != rhs_idx) { return 0; }
    }

    return 1;
}

void __debug_itemset(struct glcpg_itemset_0 *set, struct glcpg_grammar *grammar)
{
    glc_log(E_DEBUG, "Num Items %d\n", set->num_items);
    for (size_t i = 0; i < set->num_items; i++)
    {
        struct glcpg_parseitem_0 *rule = set->items + i;

        glc_log(E_DEBUG, "  - %s ::=", rule->result);
        for (size_t j = 0; j < rule->rule.num_items; j++)
        {
            if (j == rule->parse_idx) { printf(" ."); }
            ptrdiff_t idx = rule->rule.item[j];
            printf(" %s", grammar->items[idx].value);
        }
        if (rule->parse_idx == rule->rule.num_items) { printf(" ."); }
        printf("\n");
    }
}

void __debug_firstset(struct glcpg_firstset *set)
{
    glc_log(E_DEBUG, "%s: %d\n", set->key, set->count);
    for (size_t i = 0; i < set->count; i++)
    {
        struct glcpg_item *item = set->first + i;

        glc_log(E_DEBUG, "  - %s\n", item->value);
    }
}

int __close_itemset(struct glcpg_itemset_0 *result, struct glcpg_grammar *grammar)
{
    int    ret, updated;
    size_t capacity;

    if (result == NULL || grammar == NULL) { return E_GLCPG_INVALIDINPUT; }

    updated  = 1;
    capacity = result->num_items;
    while (updated == 1)    // FIXME: Can cause infinite recursion if fail to parse in some way
    {
        //__debug_itemset(result, grammar);
        updated = 0;

        if (capacity == result->num_items)
        {
            capacity *= PARSEITEMS_GROWTHFACTOR;
            ret = __alloc_parseitems(&result->items, capacity);
            if (ret < 0) { return ret; }
        }

        size_t numitms = result->num_items;
        for (size_t i = 0; i < numitms; i++)
        {
            struct glcpg_parseitem_0 *itm = result->items + i;
            if (itm->parse_idx == itm->rule.num_items)
            {
                // Do something else...
                continue;
            }

            ptrdiff_t         lookahead = itm->rule.item[itm->parse_idx];
            struct glcpg_item la_itm    = grammar->items[lookahead];

            if (la_itm.type == E_GLCPG_NONTERMINAL)
            {
                struct glcpg_ruletable *nt = NULL;
                for (size_t j = 0; j < grammar->num_nonterminals; j++)
                {
                    if (strcmp(grammar->nonterminals[j].name, la_itm.value) == 0)
                    {
                        nt = grammar->nonterminals + j;
                        break;
                    }
                }
                if (nt == NULL)
                {
                    glc_log(E_WARN, "Improperly classified NonTerminal\n");
                    continue;
                }

                while (capacity <= (result->num_items + nt->num_rules))
                {
                    capacity *= PARSEITEMS_GROWTHFACTOR;
                    ret = __alloc_parseitems(&result->items, capacity);
                    if (ret < 0) { return ret; }
                }

                for (size_t j = 0; j < nt->num_rules; j++)
                {
                    struct glcpg_parseitem_0 newitm = { .result    = nt->name,
                                                        .parse_idx = 0,
                                                        .rule      = nt->rules[j] };

                    // ~~Blindly hope there's no duplicates~~ there are :(
                    size_t k;
                    for (k = 0; k < result->num_items; k++)
                    {
                        if (__compare_parseitem(result->items + k, &newitm, grammar) == 1) break;
                    }

                    if (k == result->num_items)
                    {
                        result->items[result->num_items++] = newitm;
                        updated                            = 1;
                    }
                }
            }
        }
    }

    return 0;
}

int __compute_first_set(struct glcpg_firstset **result, struct glcpg_grammar *grammar)
{
    struct glcpg_firstset *sets;
    int                    ret;

    if (result == NULL || grammar == NULL) { return E_GLCPG_INVALIDINPUT; }

    // First Pass; Figure out inputs for everything
    sets = calloc(grammar->num_nonterminals, sizeof(struct glcpg_firstset));
    for (size_t i = 0; i < grammar->num_nonterminals; i++)
    {
        struct glcpg_firstset  *set = sets + i;
        struct glcpg_ruletable *nt  = grammar->nonterminals + i;

        set->key = nt->name;

        size_t capacity = nt->num_rules;
        set->count      = 0;
        ret             = __alloc_items2(&set->first, capacity);
        if (ret < 0) { return ret; }

        for (size_t j = 0; j < nt->num_rules; j++)
        {
            ptrdiff_t itm = nt->rules[j].item[0];

            size_t k;
            for (k = 0; k < set->count; k++)
            {
                if (strcmp(set->first[k].value, grammar->items[itm].value) == 0) break;
            }
            if (k != set->count) { continue; }

            set->first[set->count++] = grammar->items[itm];
        }

        ret = __alloc_items2(&set->first, set->count);
        if (ret < 0) { return ret; }
    }

    // Multi Pass; Evaluate iteratively until complete
    int updated = 1;
    while (updated != 0)
    {
        updated = 0;

        for (size_t i = 0; i < grammar->num_nonterminals; i++)
        {
            struct glcpg_firstset *set = sets + i;

            size_t setcount = set->count;
            for (size_t j = 0; j < setcount; j++)
            {
                if (set->first[j].type != E_GLCPG_NONTERMINAL) { continue; }

                // Self-Lookup for insertion values
                struct glcpg_firstset *target = NULL;
                for (size_t k = 0; k < grammar->num_nonterminals; k++)
                {
                    if (strcmp(sets[k].key, set->first[j].value) == 0)
                    {
                        target = sets + k;
                        break;
                    }
                }

                if (target == NULL)
                {
                    glc_log(E_WARN, "NonTerminal '%s' doesn't exist\n", set->first[j].value);
                    continue;
                }

                // Reserve more space
                ret = __alloc_items2(&set->first, set->count + target->count);
                if (ret < 0) { return ret; }

                for (size_t k = 0; k < target->count; k++)
                {
                    size_t l;
                    for (l = 0; l < set->count; l++)
                    {
                        if (strcmp(target->first[k].value, set->first[l].value) == 0) { break; }
                    }

                    if (l < set->count) { continue; }

                    set->first[set->count++] = target->first[k];
                    updated                  = 1;
                }

                ret = __alloc_items2(&set->first, set->count);
                if (ret < 0) { return ret; }
            }
        }
    }

    // Final Pass; Remove all Non-Terminals
    for (size_t i = 0; i < grammar->num_nonterminals; i++)
    {
        struct glcpg_firstset *set = sets + i;

        for (ptrdiff_t j = set->count - 1; j >= 0; j--)
        {
            if (set->first[j].type == E_GLCPG_TERMINAL) continue;

            printf("Removing item %s\n", set->first[j].value);

            // Shift array back one element
            memmove(
              set->first + j,
              set->first + j + 1,
              (set->count - j - 1) * sizeof(struct glcpg_item));
            set->count--;
        }

        // Shrink to fit
        ret = __alloc_items2(&set->first, set->count);
        if (ret < 0) { return ret; }
    }

    *result = sets;
    return grammar->num_nonterminals;
}

ptrdiff_t
  glcpg_table_create(struct glcpg_table *result, size_t lookahead, struct glcpg_grammar *grammar)
{
    size_t                  itemset_capacity;
    size_t                  num_itemsets;
    struct glcpg_itemset_0 *itemsets;

    int ret;

    if (result == NULL || grammar == NULL) { return E_GLCPG_INVALIDINPUT; }

    // Find Root Node
    struct glcpg_ruletable *root = NULL;
    for (size_t root_idx = 0; root_idx < grammar->num_nonterminals; root_idx++)
    {
        if (strcmp(grammar->nonterminals[root_idx].name, "Root") == 0)
        {
            root = grammar->nonterminals + root_idx;
            break;
        }
    }
    if (root == NULL)
    {
        glc_log(E_ERROR, "Grammar missing 'Root' node\n");
        return E_GLCPG_UNEXPECTED;
    }

    if (root->num_rules != 1)
    {
        glc_log(E_ERROR, "'Root' node too large\n");
        return E_GLCPG_UNEXPECTED;
    }

    struct glcpg_firstset *firstset = NULL;
    ret                             = __compute_first_set(&firstset, grammar);
    if (ret < 0) { return ret; }

    for (size_t i = 0; i < ret; i++) { __debug_firstset(firstset + i); }

    itemsets         = NULL;
    itemset_capacity = ITEMSETS_INITCAPACITY;
    ret              = __alloc_lr0_itemsets(&itemsets, itemset_capacity);
    if (ret < 0) { return ret; }

    struct glcpg_itemset_0 root_itemset = {
        .items     = NULL,
        .num_items = 1,
    };
    ret = __alloc_parseitems(&root_itemset.items, 1);

    root_itemset.items[0].parse_idx = 0;
    root_itemset.items[0].rule      = root->rules[0];
    root_itemset.items[0].result    = root->name;

    ret = __close_itemset(&root_itemset, grammar);
    if (ret < 0) { return ret; }

    __debug_itemset(&root_itemset, grammar);

    // ret = __close_itemset(&itemsets[0], grammar);
    // if (ret < 0) { return ret; }

    return 0;
}