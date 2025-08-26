// Sort of counterintuitively, but we generate a parser using a parser.
#include "token.h"
#include "log.h"
#include "error.h"
#include "tree.h"
#include "table.h"

#include <string.h>
#include <stdlib.h>
#include <errno.h>

#define INPUTPATHS_GROWTHRATE   2
#define INPUTPATHS_INITCAPACITY 1

#define TERMINALS_GROWTHRATE   2
#define TERMINALS_INITCAPACITY 8

struct Options
{
    const char **input_paths;
    const char  *output_path;
};

struct Options opts = {
    .output_path = NULL,
    .input_paths = NULL,
};

enum LogLevel verbosity;

void help()
{
    fprintf(stdout, "USAGE: glc_pg [options] -o OUTPUT_FILE input_files...\n");
}

int __alloc_input_paths(const char ***paths, size_t capacity)
{
    const char **newptr;
    if (paths == NULL) { return E_GLCPG_INVALIDINPUT; }

    if (capacity == 0)
    {
        free(*paths);
        *paths = NULL;
        return 0;
    }

    newptr = realloc(*paths, sizeof(const char **) * capacity);
    if (newptr == NULL)
    {
        free(*paths);
        *paths = NULL;
        return E_GLCPG_MEMORYERROR;
    }

    *paths = newptr;
    return 0;
}

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

int main(const int argc, char *const *argv)
{
    // Quick and dirty CLI
    size_t    num_paths;
    size_t    paths_capacity;
    int       index;
    ptrdiff_t ret;

    paths_capacity = INPUTPATHS_INITCAPACITY;
    num_paths      = 0;
    ret            = __alloc_input_paths(&opts.input_paths, paths_capacity);
    if (ret < 0) { return ret; }

    verbosity = E_INFO;

    if (argc == 1)
    {
        help();
        return 0;
    }

    // Parse Input
    index = 0;
    while (++index < argc)
    {
        const char   *arg = argv[index];
        unsigned long len = strlen(arg);

        if (arg[0] != '-')
        {
            // We don't have an option (must be input file)
            if (num_paths == paths_capacity)
            {
                paths_capacity *= INPUTPATHS_GROWTHRATE;
                ret = __alloc_input_paths(&opts.input_paths, paths_capacity);
                if (ret < 0) { return ret; }
            }

            opts.input_paths[num_paths++] = arg;
            continue;
        }

        if (strncmp(arg, "--", 2))
        {
            // Short Option
            unsigned long cursor = 1;

            while (cursor < len)
            {
                switch (arg[cursor++])
                {
                case 'v':
                {
                    verbosity++;
                    continue;
                }
                case 'o':
                {
                    if (cursor != len || index == argc)
                    {
                        glc_log(E_ERROR, "Invalid usage of -o\n");
                        help();
                        return -1;
                    }
                    else
                    {
                        // Read next argv
                        opts.output_path = argv[++index];
                        break;
                    }
                }

                default: glc_log(E_ERROR, "Unknown Option: -%c\n", arg[cursor - 1]);
                case 'h':
                {
                    help();
                    return 0;
                }
                }
            }
        }
        else
        {
            // Long Option; By here we've guarenteed that the -- exists at least
            const char *opt = arg + 2;
            len -= 2;

            if (strncmp(opt, "version", 7) == 0)
            {
                fprintf(stdout, "GLang Parser Generator\nVersion: 0.1.0\n");
                return 0;
            }

            if (strncmp(opt, "output", 6) == 0)
            {
                len -= 6;
                opt += 6;

                // Values have two ways of working; --this=that, or --this that
                if (len == 0)
                {
                    // Next string must exist
                    if (++index == argc)
                    {
                        glc_log(
                          E_ERROR,
                          "Invalid Usage of --output: Expected value, found nothing.\n");
                        return -1;
                    }

                    opts.output_path = argv[index];
                }
                else
                {
                    if (len == 1 || opt[0] != '=')
                    {
                        glc_log(
                          E_ERROR,
                          "Invalid Usage of --output: Expected value, found nothing.\n");
                        return -1;
                    }

                    opts.output_path = opt + 1;
                }

                continue;
            }

            // Didn't Hit anything :(
            glc_log(E_ERROR, "Unknown Option: %s\n", opt);
            help();
            return -1;
        }
    }

    if (opts.output_path == NULL)
    {
        glc_log(E_ERROR, "Missing Output!\n");
        help();
        return -1;
    }

    if (opts.input_paths == NULL || num_paths == 0)
    {
        glc_log(E_ERROR, "Missing Input!\n");
        help();
        return -1;
    }

    glc_log(E_DEBUG, "Verbosity: %d\n", verbosity);
    glc_log(E_DEBUG, "Output Path: %s\n", opts.output_path);
    glc_log(E_DEBUG, "Input Paths: \n");
    for (size_t i = 0; i < num_paths; i++) { glc_log(E_DEBUG, "  - %s\n", opts.input_paths[i]); }

    struct glcpg_grammar grammar = {
        .num_nonterminals = 0,
        .num_items        = 0,
        .items            = NULL,
        .nonterminals     = NULL,
    };
    for (size_t i = 0; i < num_paths; i++)
    {
        // First, tokenize all the files
        FILE *file = fopen(opts.input_paths[i], "r");
        if (file == NULL)
        {
            glc_log(
              E_ERROR,
              "Error while opening file '%s': %s\n",
              opts.input_paths[i],
              strerror(errno));

            free(opts.input_paths);
            return E_GLCPG_IOERROR;
        }

        struct glcpg_token *tokens;
        ptrdiff_t           len = glcpg_lexer_file(&tokens, file);
        if (len < 0)
        {
            fclose(file);
            free(opts.input_paths);
            return ret;
        }

        struct glcpg_grammar file_grammar;
        ret = glcpg_parse(&file_grammar, tokens);
        if (ret < 0)
        {
            fclose(file);
            free(opts.input_paths);
            return ret;
        }

        ret = glcpg_grammar_merge(&grammar, &file_grammar);
        if (ret < 0)
        {
            struct glcpg_token *start = tokens, tok;
            while ((tok = *(tokens++)).type != E_PGTOK_EOF) { glcpg_token_free(&tok); }
            free(start);

            fclose(file);
            free(opts.input_paths);
            return ret;
        }

        glcpg_grammar_free(&file_grammar);

        // god i miss destructors
        struct glcpg_token *start = tokens, tok;
        while ((tok = *(tokens++)).type != E_PGTOK_EOF) { glcpg_token_free(&tok); }
        free(start);

        fclose(file);
    }

    glcpg_grammar_classify(&grammar);

    // glc_log(E_DEBUG, "Num Nonterminals: %d\n", grammar.num_nonterminals);
    // for (size_t i = 0; i < grammar.num_nonterminals; i++)
    //{
    //     glc_log(
    //       E_DEBUG,
    //       "  - %s: %d\n",
    //       grammar.nonterminals[i].name,
    //       grammar.nonterminals[i].num_rules);
    // }

    struct glcpg_actiontable_entry *table = NULL;
    ret                                   = glcpg_table_create(&table, "Expr", &grammar);
    if (ret < 0)
    {
        glcpg_grammar_free(&grammar);
        free(opts.input_paths);
        return ret;
    }

    // Parse string
    const char *parse_string = "1 + 2 * (3 + 4)";

    struct glcpg_token *parse_stream = NULL;
    ret                              = glcpg_lexer_string(&parse_stream, parse_string);
    if (ret < 0) { return ret; }

    size_t                  capacity = 8;
    size_t                  size     = 0;
    struct glcpg_parseitem *stack;
    ret = __alloc_parse_items(&stack, capacity);
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
            case E_PGACT_INVALID:
                glc_log(E_ERROR, "Invalid State Transition!\n");
                return -1;
            case E_PGACT_ACCEPT:
                break;
            case E_PGACT_REDUCE:
                glc_log(E_DEBUG, "Reduce!\n");
                return -1;
                // TODO: Grammar reduction (AST Generation)

            case E_PGACT_GOTO:
                front->state_id = actions[i].next_state;
                break;

            case E_PGACT_SHIFT:
                size++;
                stack[size].state_id = actions[i].next_state;
                stack[size].lookahead = *(parse_stream++);
                break;
        }
    }

    free(table);

    ret = 0;
    glcpg_grammar_free(&grammar);
    free(opts.input_paths);
    return ret;
}
