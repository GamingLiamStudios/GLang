#include "tree.h"
#include "log.h"
#include "error.h"

#include <stdlib.h>
#include <string.h>

#define ITEMS_GROWTHFACTOR 2
#define ITEMS_INITCAPACITY 64

struct __named_rule
{
    const char       *name;
    struct glcpg_rule rule;
};

int __alloc_items(struct glcpg_item **ptr, size_t capacity)
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
        free(*ptr);
        *ptr = NULL;
        return E_GLCPG_MEMORYERROR;
    }

    *ptr = newptr;
    return 0;
}

int __alloc_items_idx(ptrdiff_t **ptr, size_t capacity)
{
    ptrdiff_t *newptr;
    if (ptr == NULL) { return E_GLCPG_INVALIDINPUT; }

    if (capacity == 0)
    {
        free(*ptr);
        *ptr = NULL;
        return 0;
    }

    newptr = realloc(*ptr, sizeof(ptrdiff_t) * capacity);
    if (newptr == NULL)
    {
        free(*ptr);
        *ptr = NULL;
        return E_GLCPG_MEMORYERROR;
    }

    *ptr = newptr;
    return 0;
}

int __alloc_rules(struct glcpg_rule **ptr, size_t capacity)
{
    struct glcpg_rule *newptr;
    if (ptr == NULL) { return E_GLCPG_INVALIDINPUT; }

    if (capacity == 0)
    {
        free(*ptr);
        *ptr = NULL;
        return 0;
    }

    newptr = realloc(*ptr, sizeof(struct glcpg_rule) * capacity);
    if (newptr == NULL)
    {
        free(*ptr);
        *ptr = NULL;
        return E_GLCPG_MEMORYERROR;
    }

    *ptr = newptr;
    return 0;
}

int __alloc_nonterminals(struct glcpg_ruletable **ptr, size_t capacity)
{
    struct glcpg_ruletable *newptr;
    if (ptr == NULL) { return E_GLCPG_INVALIDINPUT; }

    if (capacity == 0)
    {
        free(*ptr);
        *ptr = NULL;
        return 0;
    }

    newptr = realloc(*ptr, sizeof(struct glcpg_ruletable) * capacity);
    if (newptr == NULL)
    {
        free(*ptr);
        *ptr = NULL;
        return E_GLCPG_MEMORYERROR;
    }

    *ptr = newptr;
    return 0;
}

ptrdiff_t __parse_rule(
  struct __named_rule *result,
  struct glcpg_item  **items,
  size_t              *items_count,
  size_t              *items_capacity,
  struct glcpg_token  *tokens)
{
    struct glcpg_token *cur, *start;
    int                 ret;

    size_t capacity;
    if (result == NULL) { return E_GLCPG_INVALIDINPUT; }

    start = tokens;
    cur   = tokens++;
    if (cur->type != E_PGTOK_IDENT)
    {
        glc_log(E_ERROR, "1Unexpected Token in Rule; Expected Ident %d\n", cur->type);
        return E_GLCPG_UNEXPECTED;
    }

    result->name = cur->value.ident;

    cur = tokens++;
    if (cur->type != E_PGTOK_EQUAL)
    {
        glc_log(E_ERROR, "2Unexpected Token in Rule; Expected '::='\n");
        return E_GLCPG_UNEXPECTED;
    }

    // FIXME: Leaky on fail :(
    capacity               = ITEMS_INITCAPACITY;
    result->rule.item      = NULL;
    result->rule.num_items = 0;
    ret                    = __alloc_items_idx(&result->rule.item, capacity);
    if (ret < 0) { return ret; }

    cur = tokens++;
    while (cur->type != E_PGTOK_EOL && cur->type != E_PGTOK_EOF)
    {
        if (*items_count == *items_capacity)
        {
            *items_capacity *= ITEMS_GROWTHFACTOR;
            ret = __alloc_items(items, *items_capacity);
            if (ret < 0) { return ret; }
        }

        if (capacity == result->rule.num_items)
        {
            capacity *= ITEMS_GROWTHFACTOR;
            ret = __alloc_items_idx(&result->rule.item, capacity);
            if (ret < 0) { return ret; }
        }

        if (cur->type != E_PGTOK_IDENT)
        {
            glc_log(E_ERROR, "3Unexpected Token in Rule; Expected Ident\n");
            return E_GLCPG_UNEXPECTED;
        }

        // Find item ptr for current item
        struct glcpg_item *itm = NULL;
        for (size_t i = 0; i < *items_count; i++)
        {
            struct glcpg_item *existing = *items + i;
            if (strcmp(existing->value, cur->value.ident) == 0)
            {
                itm = existing;
                break;
            }
        }

        if (itm == NULL)
        {
            (*items)[*items_count].type = E_GLCPG_TERMINAL;

            size_t len                   = strlen(cur->value.ident);
            (*items)[*items_count].value = calloc(len + 1, sizeof(char));
            strncpy((char *) (*items)[*items_count].value, cur->value.ident, len);

            itm = (*items) + (*items_count)++;
        }

        result->rule.item[result->rule.num_items++] = itm - *items;

        cur = tokens++;
    }

    ret = __alloc_items_idx(&result->rule.item, result->rule.num_items);
    if (ret < 0) { return ret; }

    return tokens - start;
}

int glcpg_parse(struct glcpg_grammar *result, struct glcpg_token *tokens)
{
    // FIXME: Leaky on fail :(
    size_t    items_capacity, nt_capacity;
    ptrdiff_t ret;

    struct glcpg_token cur;

    if (result == NULL) { return E_GLCPG_INVALIDINPUT; }

    items_capacity    = ITEMS_INITCAPACITY;
    result->num_items = 0;
    result->items     = NULL;
    ret               = __alloc_items(&result->items, items_capacity);
    if (ret < 0) { return ret; }

    // Inital parse
    nt_capacity              = ITEMS_INITCAPACITY;
    result->num_nonterminals = 0;
    result->nonterminals     = NULL;
    ret                      = __alloc_nonterminals(&result->nonterminals, nt_capacity);
    if (ret < 0) { return ret; }

    while (1)
    {
        cur = *tokens;
        if (cur.type == E_PGTOK_EOF) { break; }
        if (cur.type == E_PGTOK_EOL)
        {
            tokens++;
            continue;
        }    // Skip empty lines

        if (nt_capacity == result->num_nonterminals)
        {
            nt_capacity *= ITEMS_GROWTHFACTOR;
            ret = __alloc_nonterminals(&result->nonterminals, nt_capacity);
            if (ret < 0) { return ret; }
        }

        struct __named_rule rule;
        ret = __parse_rule(&rule, &result->items, &result->num_items, &items_capacity, tokens);
        if (ret < 0)
        {
            free(result->items);
            return ret;
        }
        tokens += ret;

        struct glcpg_ruletable *nt = NULL;
        for (size_t i = 0; i < result->num_nonterminals; i++)
        {
            if (strcmp(rule.name, result->nonterminals[i].name) == 0)
            {
                nt = result->nonterminals + i;
                break;
            }
        }

        if (nt == NULL)
        {
            nt = result->nonterminals + result->num_nonterminals++;

            nt->num_rules = 0;
            nt->rules     = NULL;

            size_t len = strlen(rule.name);
            nt->name   = calloc(len + 1, sizeof(char));
            strncpy((char *) nt->name, rule.name, len);
        }

        // TODO: Use capacity instead of 'num_rules'
        ret = __alloc_rules(&nt->rules, nt->num_rules + 1);
        if (ret < 0) { return ret; }

        // TODO: Deduplication
        nt->rules[nt->num_rules++] = rule.rule;

        if ((tokens - 1)->type == E_PGTOK_EOF) { break; }
    }

    // Shrink to fit
    ret = __alloc_nonterminals(&result->nonterminals, result->num_nonterminals);
    if (ret < 0) { return ret; }

    ret = __alloc_items(&result->items, result->num_items);
    if (ret < 0) { return ret; }

    return 0;
}

void glcpg_grammar_classify(struct glcpg_grammar *target)
{
    for (size_t i = 0; i < target->num_items; i++) { target->items[i].type = E_GLCPG_TERMINAL; }

    for (size_t j = 0; j < target->num_nonterminals; j++)
    {
        struct glcpg_item *itm = NULL;
        for (size_t i = 0; i < target->num_items; i++)
        {
            if (strcmp(target->items[i].value, target->nonterminals[j].name) == 0)
            {
                itm = target->items + i;
                break;
            }
        }

        if (itm == NULL)
        {
            if (strcmp(target->nonterminals[j].name, "Root") != 0)
            {
                glc_log(
                  E_WARN,
                  "Didn't encounter non-terminal `%s` in items\n",
                  target->nonterminals[j].name);
            }
            continue;
        }
        itm->type = E_GLCPG_NONTERMINAL;
    }
}

int glcpg_grammar_merge(struct glcpg_grammar *target, struct glcpg_grammar *source)
{
    int    ret;
    size_t new_size;

    if (target == NULL || source == NULL) { return E_GLCPG_INVALIDINPUT; }

    if (target->items == NULL && target->num_items != 0) { return E_GLCPG_INVALIDINPUT; }
    if (source->items == NULL && source->num_items != 0) { return E_GLCPG_INVALIDINPUT; }

    ret = __alloc_items(&target->items, target->num_items + source->num_items);
    if (ret < 0) { return E_GLCPG_MEMORYERROR; }

    new_size = target->num_items;
    for (size_t i = 0; i < source->num_items; i++)
    {
        struct glcpg_item *target_itm, *source_itm;
        target_itm = NULL;
        source_itm = source->items + i;

        for (size_t j = 0; j < target->num_items; j++)
        {
            if (strcmp(source_itm->value, target->items[j].value) == 0)
            {
                target_itm = target->items + i;
                break;
            }
        }

        if (target_itm == NULL)
        {
            target_itm       = target->items + new_size;
            target_itm->type = source_itm->type;

            size_t len        = strlen(source_itm->value);
            target_itm->value = calloc(len + 1, sizeof(char));
            strncpy((char *) target_itm->value, source_itm->value, len);

            new_size += 1;
        }
    }
    target->num_items = new_size;

    ret = __alloc_nonterminals(
      &target->nonterminals,
      target->num_nonterminals + source->num_nonterminals);
    if (ret < 0) { return E_GLCPG_MEMORYERROR; }

    new_size = target->num_nonterminals;
    for (size_t i = 0; i < source->num_nonterminals; i++)
    {
        struct glcpg_ruletable *target_nt, *source_nt;
        target_nt = NULL;
        source_nt = source->nonterminals + i;

        for (size_t j = 0; j < target->num_nonterminals; j++)
        {
            if (strcmp(target->nonterminals[j].name, source_nt->name) == 0)
            {
                target_nt = target->nonterminals + j;
                break;
            }
        }

        if (target_nt == NULL)
        {
            target_nt = target->nonterminals + new_size++;

            size_t len      = strlen(source_nt->name);
            target_nt->name = calloc(len + 1, sizeof(char));
            strncpy((char *) target_nt->name, source_nt->name, len);

            target_nt->num_rules = source_nt->num_rules;
            target_nt->rules     = calloc(target_nt->num_rules, sizeof(struct glcpg_rule));

            for (size_t j = 0; j < target_nt->num_rules; j++)
            {
                target_nt->rules[j].num_items = source_nt->rules[j].num_items;
                target_nt->rules[j].item = calloc(source_nt->rules[j].num_items, sizeof(ptrdiff_t));

                // TODO: Deduplication
                for (size_t k = 0; k < source_nt->rules[j].num_items; k++)
                {
                    ptrdiff_t src_idx = source_nt->rules[j].item[k];
                    size_t    l;
                    for (l = 0; l < target->num_items; l++)
                    {
                        if (strcmp(target->items[l].value, source->items[src_idx].value) == 0)
                        {
                            break;
                        }
                    }

                    if (l >= source->num_items)
                    {
                        glc_log(E_ERROR, "Couldn't find item inside merged itemset\n");
                        return E_GLCPG_UNEXPECTED;
                    }

                    target_nt->rules[j].item[k] = l;
                }

                // target_nt->rules[j] = source_nt->rules[j];
            }

            continue;
        }

        ret = __alloc_rules(&target_nt->rules, target_nt->num_rules + source_nt->num_rules);
        if (ret < 0) { return E_GLCPG_MEMORYERROR; }
    }

    target->num_nonterminals = new_size;

    return 0;
}

void glcpg_grammar_free(struct glcpg_grammar *grammar)
{
    if (grammar == NULL) { return; }

    if (grammar->items != NULL)
    {
        for (size_t i = 0; i < grammar->num_items; i++)
        {
            struct glcpg_item *itm = grammar->items + i;
            free((char *) itm->value);
        }
        free(grammar->items);
        grammar->items = NULL;
    }

    if (grammar->nonterminals != NULL)
    {
        for (size_t i = 0; i < grammar->num_nonterminals; i++)
        {
            struct glcpg_ruletable *nt = grammar->nonterminals + i;
            if (nt->rules == NULL) { continue; }

            for (size_t j = 0; j < nt->num_rules; j++) { free(nt->rules[j].item); }

            free(nt->rules);
            free((char *) nt->name);
        }
        free(grammar->nonterminals);
        grammar->nonterminals = NULL;
    }

    grammar->num_items        = 0;
    grammar->num_nonterminals = 0;
}