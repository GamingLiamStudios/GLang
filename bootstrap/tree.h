#pragma once

#include <stddef.h>
#include "token.h"

enum ast_typedef
{
    E_AST_TYPEDEF_INFERRED = -1,

    E_AST_TYPEDEF_UNSIGNED8,
    E_AST_TYPEDEF_UNSIGNED16,
    E_AST_TYPEDEF_UNSIGNED32,
    E_AST_TYPEDEF_UNSIGNED64,

    E_AST_TYPEDEF_SIGNED8,
    E_AST_TYPEDEF_SIGNED16,
    E_AST_TYPEDEF_SIGNED32,
    E_AST_TYPEDEF_SIGNED64,

    E_AST_TYPEDEF_PTR,
    E_AST_TYPEDEF_PTRLEN,
    E_AST_TYPEDEF_PTRDIFF,

    E_AST_TYPEDEF_FLOAT32,
    E_AST_TYPEDEF_FLOAT64,

    // Advanced Data Structures
    E_AST_TYPEDEF_STRUCT,
    E_AST_TYPEDEF_ENUM,
    E_AST_TYPEDEF_ARRAY,
    E_AST_TYPEDEF_FUNCTION,
};

// Every Ident has an associated Type
struct ast_identifier
{
    const char      *name;
    enum ast_typedef type;
};

struct ast_expr_constant
{
    enum
    {
        E_AST_EXPR_CONSTANT_STRING,
        E_AST_EXPR_CONSTANT_INTEGER,
        E_AST_EXPR_CONSTANT_DECIMAL,
    } type;

    union
    {
        const char *string;
        long        integer;
        struct
        {
            long          integer;
            unsigned long fractional;
        } decimal;
    } value;
};

struct ast_node
{
    enum
    {
        E_AST_FUNCTIONDEF,
        E_AST_ASSIGN,

        E_AST_CONSTANT,
        E_AST_IDENTIFIER,
        E_AST_FUNCTIONCALL,

        // Unary Operator
        E_AST_INVERT,

        // Binary Operator
        E_AST_ADD,
        E_AST_SUB,
        E_AST_MUL,
        E_AST_DIV,

        E_AST_AND,
        E_AST_OR,

        E_AST_BRANCH,
        E_AST_LOOP,
        E_AST_COND,
    } type;

    union
    {
        struct ast_expr_constant constant;
        struct ast_identifier    identifier;

        struct
        {
            struct ast_identifier identifier;
            struct ast_node      *value;
        } assign;

        struct ast_node *unary;
        struct
        {
            struct ast_node *lhs;
            struct ast_node *rhs;
        } binary;

        struct
        {
            struct ast_identifier identifier;
            struct ast_node      *arguments;
            size_t                num_arguments;
        } function_call;

        struct
        {
            struct ast_identifier *parameters;
            size_t                 num_parameters;

            struct ast_node *body_nodes;
            size_t           num_body_nodes;
        } function_def;

        struct
        {
            enum
            {
                E_AST_COND_LESS,
                E_AST_COND_LESSEQ,
                E_AST_COND_EQ,
                E_AST_COND_GREATEQ,
                E_AST_COND_GREAT,
            } type;

            struct ast_node *lhs;
            struct ast_node *rhs;
        } cond;

        struct
        {
            struct ast_node *condition;
            struct ast_node *if_true;     // Can be NULL
            struct ast_node *if_false;    // Can be NULL
        } branch;

        struct
        {
            struct ast_node *condition;
            struct ast_node *exec;
        } loop;
    } value;
};

struct ast_list
{
    struct ast_node *nodes;
    size_t           num_nodes;
    size_t           capacity;
};

void ast_list_free(struct ast_list *nodes);

int ast_parse_tokens(struct ast_list *nodes, struct token_stream *tokens);