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

int __alloc_lritems(struct glcpg_lritem **ptr, size_t capacity)
{
    struct glcpg_lritem *newptr;
    if (ptr == NULL) { return E_GLCPG_INVALIDINPUT; }

    if (capacity == 0)
    {
        free(*ptr);
        *ptr = NULL;
        return 0;
    }

    newptr = realloc(*ptr, sizeof(struct glcpg_lritem) * capacity);
    if (newptr == NULL)
    {
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

void __debug_lritem(struct glcpg_lritem *item, struct glcpg_grammar *grammar)
{
    glc_log(E_DEBUG, "%s ::=", item->key);
    for (size_t i = 0; i < item->rule.num_items; i++)
    {
        ptrdiff_t idx = item->rule.item[i];
        printf(" %s", grammar->items[idx].value);
    }

    printf(" {");
    for (size_t i = 0; i < item->num_follow; i++) { printf(" %s", item->follow[i].value); }

    printf(" }\n");
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

            if (la_itm.type == E_PGITM_NONTERMINAL)
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
                if (set->first[j].type != E_PGITM_NONTERMINAL) { continue; }

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
            if (set->first[j].type == E_PGITM_TERMINAL) continue;

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

ptrdiff_t __close_lritemset(
  struct glcpg_lritem  **state_k,
  size_t                 size,
  struct glcpg_grammar  *grammar,
  struct glcpg_firstset *firstset)
{
    int    ret;
    size_t capacity = size;

    if (state_k == NULL || *state_k == NULL || size == 0 || grammar == NULL || firstset == NULL)
    {
        return E_GLCPG_INVALIDINPUT;
    }

    // Closure;
    // [A → α • B β, a]
    // [B → • γ, b], b ∈ FIRST(βa)

    ptrdiff_t processed = 0;
    while (processed < size)
    {
        size_t prev_state0_size = size;
        for (; processed < prev_state0_size; processed++)
        {
            struct glcpg_lritem *state = *state_k + processed;

            ptrdiff_t          idx  = state->rule.item[state->parse_idx];
            struct glcpg_item *next = grammar->items + idx;

            if (next->type != E_PGITM_NONTERMINAL)
            {
                // Rule cannot be expanded further
                continue;
            }

            struct glcpg_ruletable *nt = NULL;
            for (size_t i = 0; i < grammar->num_nonterminals; i++)
            {
                if (strcmp(grammar->nonterminals[i].name, next->value) == 0)
                {
                    nt = grammar->nonterminals + i;
                    break;
                }
            }

            if (nt == NULL)
            {
                glc_log(E_WARN, "NonTerminal '%s' doesn't exist\n", next->value);
                continue;
            }

            for (size_t i = 0; i < nt->num_rules; i++)
            {
                struct glcpg_parseitem_0 cmp = {
                    .parse_idx = 0,
                    .result    = nt->name,
                    .rule      = nt->rules[i],
                };

                // Search current processed list for definitions of next
                struct glcpg_lritem *existing = NULL;
                for (size_t j = 0; j < size; j++)
                {
                    struct glcpg_lritem *lookup = *state_k + j;
                    if (__compare_parseitem(
                          &(struct glcpg_parseitem_0) {
                            .parse_idx = lookup->parse_idx,
                            .result    = lookup->key,
                            .rule      = lookup->rule,
                          },
                          &cmp,
                          grammar))
                    {
                        existing = lookup;
                        break;
                    }
                }

                struct glcpg_item *follows;
                size_t             num_follows;

                if (state->parse_idx + 1 >= state->rule.num_items)
                {
                    // Propagate input follows
                    num_follows = state->num_follow;
                    follows     = state->follow;
                }
                else
                {
                    // Use next token directly
                    idx                          = state->rule.item[state->parse_idx + 1];
                    struct glcpg_item *lookahead = grammar->items + idx;

                    if (lookahead->type == E_PGITM_NONTERMINAL)
                    {
                        // Find next in firstset
                        struct glcpg_firstset *first_next = NULL;
                        for (size_t j = 0; j < grammar->num_nonterminals; j++)
                        {
                            if (strcmp(firstset[j].key, lookahead->value) == 0)
                            {
                                first_next = firstset + j;
                                break;
                            }
                        }

                        if (first_next == NULL)
                        {
                            glc_log(
                              E_ERROR,
                              "FirstSet doesn't exist for NonTerminal '%s'\n",
                              lookahead->value);
                            return E_GLCPG_UNEXPECTED;
                        }

                        // Copy firstset into follow
                        num_follows = first_next->count;
                        follows     = first_next->first;
                    }
                    else
                    {
                        num_follows = 1;
                        follows     = lookahead;
                    }
                }

                if (existing == NULL)
                {
                    // Rule not added; Push to state
                    if (capacity == size)
                    {
                        capacity *= 2;
                        ret = __alloc_lritems(state_k, capacity);
                        if (ret < 0) { return ret; }

                        state = *state_k + processed;
                    }

                    struct glcpg_lritem item;

                    item.key       = nt->name;
                    item.rule      = nt->rules[i];
                    item.parse_idx = 0;

                    item.num_follow = num_follows;
                    item.follow     = calloc(num_follows, sizeof(struct glcpg_item));
                    memcpy(item.follow, follows, num_follows * sizeof(struct glcpg_item));

                    (*state_k)[size++] = item;
                    continue;
                }

                // Rule exists; Append follows onto existing rule state
                ret = __alloc_items2(&existing->follow, existing->num_follow + num_follows);
                if (ret < 0) { return ret; }
                memcpy(
                  existing->follow + existing->num_follow,
                  follows,
                  num_follows * sizeof(struct glcpg_item));
                existing->num_follow += num_follows;
            }
        }
    }

    // Shrink to fit
    ret = __alloc_lritems(state_k, size);
    if (ret < 0) { return ret; }

    return size;
}

ptrdiff_t glcpg_table_create(
  struct glcpg_table   *result,
  const char           *root_node,
  struct glcpg_grammar *grammar)
{
    int ret;

    if (result == NULL || grammar == NULL) { return E_GLCPG_INVALIDINPUT; }

    // Find Root Node
    struct glcpg_ruletable *root = NULL;
    for (size_t i = 0; i < grammar->num_nonterminals; i++)
    {
        if (strcmp(grammar->nonterminals[i].name, root_node) == 0)
        {
            root = grammar->nonterminals + i;
            break;
        }
    }
    if (root == NULL)
    {
        glc_log(E_ERROR, "Grammar missing Root NonTerminal '%s'\n", root_node);
        return E_GLCPG_INVALIDINPUT;
    }

    ptrdiff_t root_idx;
    for (root_idx = 0; root_idx < grammar->num_items; root_idx++)
    {
        if (strcmp(grammar->items[root_idx].value, root_node) == 0) { break; }
    }
    if (root_idx >= grammar->num_items)
    {
        glc_log(E_ERROR, "Grammar missing Root Item '%s'\n", root_node);
        return E_GLCPG_INVALIDINPUT;
    }

    struct glcpg_firstset *firstset = NULL;
    ret                             = __compute_first_set(&firstset, grammar);
    if (ret < 0) { return ret; }

    for (size_t i = 0; i < ret; i++) { __debug_firstset(firstset + i); }

    // Build state 0
    struct glcpg_lritem *state0 = NULL;
    ret                         = __alloc_lritems(&state0, 1);
    if (ret < 0) { return ret; }

    state0[0].key  = "";
    state0[0].rule = (struct glcpg_rule) {
        .num_items = 1,
        .item      = &root_idx,
    };

    state0[0].parse_idx  = 0;
    state0[0].num_follow = 1;
    state0[0].follow     = NULL;
    ret                  = __alloc_items2(&state0[0].follow, 1);
    if (ret < 0) { return ret; }

    state0[0].follow[0].type = E_PGITM_EOF;

    ret = __close_lritemset(&state0, 1, grammar, firstset);
    if (ret < 0) { return ret; }
    size_t state0_size = ret;

    for (size_t i = 0; i < state0_size; i++) { __debug_lritem(state0 + i, grammar); }

    return 0;
}