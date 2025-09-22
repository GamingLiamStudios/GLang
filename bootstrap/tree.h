#pragma once

#include <stdlib.h>

struct glc_number
{
    unsigned long integer;
    unsigned long fractional;
};

struct glc_ast_type
{
    const char *ident;
};

struct glc_ast_expr
{
    enum
    {
        E_AST_EXPR_NUM,
        E_AST_EXPR_VAR,

        E_AST_EXPR_NOT,
        E_AST_EXPR_NEG,

        E_AST_EXPR_MUL,
        E_AST_EXPR_DIV,

        E_AST_EXPR_ADD,
        E_AST_EXPR_SUB,

        E_AST_EXPR_GT,
        E_AST_EXPR_GE,
        E_AST_EXPR_EQ,
        E_AST_EXPR_LE,
        E_AST_EXPR_LT,

        E_AST_EXPR_BOOLAND,
        E_AST_EXPR_BOOLOR,

        E_AST_EXPR_IF,        // 2 children
        E_AST_EXPR_IFELSE,    // 3 children
    } type;

    union
    {
        struct
        {
            struct glc_number    val;
            struct glc_ast_type *type;
        } v_num;
        const char *v_var;

        struct glc_ast_expr *v_children;
    };
};

struct glc_ast_decl_argitem
{
    const char         *ident;
    struct glc_ast_type type;
};

struct glc_ast_variable
{
    const char         *ident;
    struct glc_ast_type type;

    struct glc_ast_expr value;

    enum : int
    {
        E_AST_VAR_CONST = 1 << 0,
    } flags;
};

struct glc_ast_function
{
    const char *ident;
    // TODO: Templating

    enum : int
    {
        E_AST_FUNC_CONST  = 1 << 0,
        E_AST_FUNC_EXTERN = 1 << 1,
    } flags;

    struct glc_ast_decl_argitem *args;
    size_t                       num_args;

    struct glc_ast_type type;

    struct glc_ast_statement *body;
};

struct glc_ast_statement
{
    enum
    {
        E_AST_STMT_VARIABLE,
        E_AST_STMT_FUNCTION,

        E_AST_STMT_EXPR,
        E_AST_STMT_BODY,
        // E_AST_ROOT_STRUCT,
        // E_AST_ROOT_ENUM,
    } type;

    union
    {
        struct glc_ast_variable v_variable;
        struct glc_ast_function v_func;
        struct glc_ast_expr     v_expr;

        struct
        {
            struct glc_ast_statement *stmts;
            size_t                    num_stmts;
        } v_body;
    };
};

struct glc_ast_root
{
    struct glc_ast_statement *decls;
    size_t                    num_decls;
};

void glc_ast_root_push(struct glc_ast_root *root, struct glc_ast_statement new);