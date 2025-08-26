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

int __alloc_states(struct glcpg_lrstate **ptr, size_t capacity)
{
    struct glcpg_lrstate *newptr;
    if (ptr == NULL) { return E_GLCPG_INVALIDINPUT; }

    if (capacity == 0)
    {
        free(*ptr);
        *ptr = NULL;
        return 0;
    }

    newptr = realloc(*ptr, sizeof(struct glcpg_lrstate) * capacity);
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
        if (item->parse_idx == i) { printf(" ."); }
        printf(" %s", grammar->items[idx].value);
    }

    if (item->parse_idx == item->rule.num_items) { printf(" ."); }

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
  size_t                *size,
  struct glcpg_grammar  *grammar,
  struct glcpg_firstset *firstset)
{
    int    ret;
    size_t capacity = *size;

    if (
      state_k == NULL || *state_k == NULL || size == NULL || *size == 0 || grammar == NULL ||
      firstset == NULL)
    {
        return E_GLCPG_INVALIDINPUT;
    }

    // Closure;
    // [A → α • B β, a]
    // [B → • γ, b], b ∈ FIRST(βa)

    int updated = 1;
    while (updated)
    {
        updated                 = 0;
        size_t prev_state0_size = *size;
        for (size_t processed = 0; processed < prev_state0_size; processed++)
        {
            struct glcpg_lritem *state = *state_k + processed;

            if (state->parse_idx == state->rule.num_items) { continue; }

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
                for (size_t j = 0; j < *size; j++)
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
                    if (capacity == *size)
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

                    (*state_k)[(*size)++] = item;
                    updated               = 1;
                    continue;
                }

                struct glcpg_item *prev_follow;
                if (follows == state->follow) { prev_follow = state->follow; }
                else { prev_follow = NULL; }

                // Rule exists; Append follows onto existing rule state
                ret = __alloc_items2(&existing->follow, existing->num_follow + num_follows);
                if (ret < 0) { return ret; }

                if (prev_follow != NULL && state->follow != prev_follow)
                {
                    follows = state->follow;
                }

                for (size_t j = 0; j < num_follows; j++)
                {
                    size_t k;
                    for (k = 0; k < existing->num_follow; k++)
                    {
                        if (existing->follow[k].type == follows[j].type)
                        {
                            if (
                              existing->follow[k].type != E_PGITM_EOF &&
                              follows[j].type != E_PGITM_EOF)
                            {
                                if (strcmp(existing->follow[k].value, follows[j].value) == 0) break;
                            }
                            else { break; }
                        }
                    }

                    if (k >= existing->num_follow)
                    {
                        existing->follow[existing->num_follow++] = follows[j];
                        updated                                  = 1;
                    }
                }

                ret = __alloc_items2(&existing->follow, existing->num_follow);
                if (ret < 0) { return ret; }
            }
        }
    }

    // Shrink to fit
    ret = __alloc_lritems(state_k, *size);
    if (ret < 0) { return ret; }

    return 0;
}

void __clone_lritem(struct glcpg_lritem *dst, struct glcpg_lritem *src)
{
    if (src == NULL || dst == NULL) { return; }

    dst->key        = src->key;
    dst->num_follow = src->num_follow;

    dst->follow = calloc(src->num_follow, sizeof(struct glcpg_item));
    memcpy(dst->follow, src->follow, src->num_follow * sizeof(struct glcpg_item));

    dst->parse_idx = src->parse_idx;
    dst->rule      = src->rule;
}

ptrdiff_t glcpg_table_create(
  struct glcpg_actiontable_entry **result,
  const char                      *root_node,
  struct glcpg_grammar            *grammar)
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

    //for (size_t i = 0; i < ret; i++) { __debug_firstset(firstset + i); }

    size_t                num_states     = 0;
    size_t                state_capacity = 8;
    struct glcpg_lrstate *states         = NULL;
    ret                                  = __alloc_states(&states, state_capacity);
    if (ret < 0) { return ret; }

    // Build state 0
    {
        struct glcpg_lrstate *state0 = states + num_states++;
        state0->itemset              = NULL;
        state0->num_items            = 1;
        ret                          = __alloc_lritems(&state0->itemset, 1);
        if (ret < 0) { return ret; }

        state0->itemset[0].key  = "";
        state0->itemset[0].rule = (struct glcpg_rule) {
            .num_items = 1,
            .item      = &root_idx,
        };

        state0->itemset[0].parse_idx  = 0;
        state0->itemset[0].num_follow = 1;
        state0->itemset[0].follow     = NULL;
        ret                           = __alloc_items2(&state0->itemset[0].follow, 1);
        if (ret < 0) { return ret; }

        state0->itemset[0].follow[0].type  = E_PGITM_EOF;
        state0->itemset[0].follow[0].value = NULL;

        state0->gotos = calloc(grammar->num_items, sizeof(ptrdiff_t));
    }

    // Build rest of states
    int updated = 1;
    while (updated)
    {
        updated = 0;

        size_t prev_num_states = num_states;
        for (size_t processed = 0; processed < prev_num_states; processed++)
        {
            struct glcpg_lrstate *state = states + processed;

            ret = __close_lritemset(&state->itemset, &state->num_items, grammar, firstset);
            if (ret < 0) { return ret; }

            // Allocate space for possible states
            if ((ptrdiff_t) (state_capacity - num_states) < grammar->num_items)
            {
                state_capacity *= 2;
                ret = __alloc_states(&states, state_capacity);
                if (ret < 0) { return ret; }

                state = states + processed;
            }

            // For each item...
            for (size_t i = 0; i < state->num_items; i++)
            {
                struct glcpg_lritem *item = state->itemset + i;
                if (item->parse_idx >= item->rule.num_items) { continue; }

                ptrdiff_t         next_idx = item->rule.item[item->parse_idx];
                struct glcpg_item next     = grammar->items[next_idx];
                if (next.type == E_PGITM_EOF) { continue; }

                // printf("next %d: %s\n", i, next.value);

                // Find state [A → α • B β, a]
                size_t j;
                for (j = 0; j < num_states; j++)
                {
                    struct glcpg_lrstate *search = states + j;

                    size_t k;
                    for (k = 0; k < search->num_items; k++)
                    {
                        struct glcpg_lritem *search_item = search->itemset + k;
                        if (search_item->parse_idx < 1) { continue; }

                        size_t search_next_idx = search_item->rule.item[search_item->parse_idx - 1];
                        struct glcpg_item search_next = grammar->items[search_next_idx];

                        if (
                          search_next.type == next.type &&
                          strcmp(search_next.value, next.value) == 0)
                        {
                            break;
                        }
                    }

                    if (k < search->num_items) { break; }
                }

                if (j < num_states)
                {
                    // State already exists
                    state->gotos[next_idx] = j - processed;

                    // Make sure exact current state (+ 1) exists in target state
                    struct glcpg_parseitem_0 cmp_item = {
                        .parse_idx = item->parse_idx + 1,
                        .result    = item->key,
                        .rule      = item->rule,
                    };

                    struct glcpg_lrstate *search = states + j;
                    size_t                k;
                    for (k = 0; k < search->num_items; k++)
                    {
                        struct glcpg_lritem *search_item = search->itemset + k;

                        if (__compare_parseitem(
                              &(struct glcpg_parseitem_0) {
                                .parse_idx = search_item->parse_idx,
                                .result    = search_item->key,
                                .rule      = search_item->rule,
                              },
                              &cmp_item,
                              grammar))
                        {
                            break;
                        }
                    }

                    if (k >= search->num_items)
                    {
                        // Item doesn't exist; push to state
                        ret = __alloc_lritems(&search->itemset, search->num_items + 1);
                        if (ret < 0) { return ret; }

                        struct glcpg_lritem *next_item = search->itemset + search->num_items++;
                        __clone_lritem(next_item, item);
                        next_item->parse_idx++;
                        // printf("%lu %lu", j, search->num_items);
                        //__debug_lritem(next_item, grammar);

                        updated = 1;
                    }

                    continue;
                }

                // Add new state & close
                struct glcpg_lrstate next_state;
                next_state.gotos = calloc(grammar->num_items, sizeof(ptrdiff_t));

                next_state.itemset   = NULL;
                next_state.num_items = 1;
                ret                  = __alloc_lritems(&next_state.itemset, 1);
                if (ret < 0) { return ret; }

                __clone_lritem(next_state.itemset + 0, item);
                next_state.itemset[0].parse_idx++;
                //__debug_lritem(next_state.itemset, grammar);

                ret =
                  __close_lritemset(&next_state.itemset, &next_state.num_items, grammar, firstset);
                if (ret < 0) { return ret; }

                state->gotos[next_idx] = num_states - processed;
                states[num_states++]   = next_state;
                updated                = 1;
            }
        }
    }

    ret = __alloc_states(&states, num_states);
    if (ret < 0) { return ret; }

    //for (size_t i = 0; i < num_states; i++)
    //{
    //    struct glcpg_lrstate *state = states + i;
    //
    //    // Debug states
    //    glc_log(E_DEBUG, "\n");
    //    glc_log(E_DEBUG, "State %lu %lu;\n", i, state->num_items);
    //    for (size_t i = 0; i < state->num_items; i++)
    //    {
    //        __debug_lritem(state->itemset + i, grammar);
    //    }
    //
    //    glc_log(E_DEBUG, "Goto;");
    //    for (size_t i = 0; i < grammar->num_items; i++)
    //    {
    //        // Goto
    //        printf(", '%s': %ld", grammar->items[i].value, state->gotos[i]);
    //    }
    //    printf("\n");
    //}

    // Build action table
    struct glcpg_actiontable_entry *action_table;
    action_table =
      calloc(num_states * (grammar->num_items + 1), sizeof(struct glcpg_actiontable_entry));

    // Shift to m; [A -> a . B b, a]
    // Reduce A -> a; [A -> a ., a]

    for (size_t i = 0; i < num_states; i++)
    {
        struct glcpg_actiontable_entry *tables = action_table + i * (grammar->num_items + 1);
        struct glcpg_lrstate           *state  = states + i;

        for (size_t j = 0; j < grammar->num_items; j++)
        {
            struct glcpg_item *grammar_item = grammar->items + j;
            ptrdiff_t          goto_offs    = state->gotos[j];

            if (goto_offs == 0)
            {
                tables[j].action =
                  grammar_item->type == E_PGITM_EOF ? E_PGACT_ACCEPT : E_PGACT_INVALID;
                break;
            }

            tables[j].next_state = i + goto_offs;
            switch (grammar_item->type)
            {
            case E_PGITM_EOF: tables[j].action = E_PGACT_REDUCE; break;
            case E_PGITM_NONTERMINAL: tables[j].action = E_PGACT_GOTO; break;
            case E_PGITM_TERMINAL: tables[j].action = E_PGACT_SHIFT; break;
            }
        }
    }

    // Free FirstSet
    for (size_t i = 0; i < grammar->num_nonterminals; i++)
    {
        struct glcpg_firstset *itm = firstset + i;
        free(itm->first);
    }
    free(firstset);

    // Free States
    for (size_t i = 0; i < num_states; i++)
    {
        struct glcpg_lrstate *state = states + i;

        for(size_t j = 0; j < state->num_items; j++)
        {
            struct glcpg_lritem *itm = state->itemset + j;
            free(itm->follow);
        }

        free(state->itemset);
        free(state->gotos);
    }
    free(states);

    *result = action_table;
    return num_states;
}
