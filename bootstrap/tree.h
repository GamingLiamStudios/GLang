#pragma once

#include <stddef.h>
#include "token.h"

enum ast_operation
{
    // Arithmatic
    E_AST_OP_ADD,    // +
    E_AST_OP_SUB,    // -
    E_AST_OP_MUL,    // *
    E_AST_OP_DIV,    // /

    // Binary Bitwise
    E_AST_OP_AND,    // & or && (Boolean)
    E_AST_OP_OR,     // | or || (Boolean)
    E_AST_OP_XOR,    // ^

    // Comparisons
    E_AST_OP_LT,     // <
    E_AST_OP_LE,     // <=
    E_AST_OP_EQ,     // ==
    E_AST_OP_NEQ,    // !=
    E_AST_OP_GE,     // >=
    E_AST_OP_GT,     // >

    // Unary
    E_AST_OP_NEGATE,    // -
    E_AST_OP_DEREF,     // *
    E_AST_OP_BORROW,    // &
    E_AST_OP_UNWRAP,    // ?

    // Unary Bitwise
    E_AST_OP_INVERT,    // ~ or ! (Boolean)
};

struct ast_type
{
    const char *typename;
    // TODO: Generics
};

struct ast_param
{
    const char     *name;
    struct ast_type type;
};

struct ast_enum_value
{
    enum
    {
        E_AST_ENUM_BASIC,
        E_AST_ENUM_TUPLE,
        E_AST_ENUM_STRUCT,
    } type;

    const char *ident;
    size_t      count;

    union
    {
        // Used by; Struct
        struct ast_param *param_list;

        // Used by; Tuple
        struct ast_type *type_list;
    } value;
};

struct ast_argument
{
    const char            *name;
    struct ast_expression *value;
};

struct ast_expression
{
    enum
    {
        E_AST_EXPR_ASSIGN,
        E_AST_EXPR_BLOCK,

        E_AST_EXPR_CAST,
        E_AST_EXPR_LAMBDA,
        E_AST_EXPR_STRUCT,

        E_AST_EXPR_OPERATION,
        E_AST_EXPR_CALL,
        E_AST_EXPR_CONST,
        E_AST_EXPR_VARIABLE,

        E_AST_EXPR_LOOP,
        E_AST_EXPR_IF,
        E_AST_EXPR_WHILE,
    } type;

    union
    {
        // Used by; Assign
        struct
        {
            const char            *ident;
            struct ast_expression *expr;
        } assign;

        // Used by; Block, Loop
        struct
        {
            struct ast_statement *stmts;
            size_t                num_stmts;

            struct ast_expression *expr;
        } block;

        // Used by; Cast
        struct
        {
            struct ast_expression *expr;
            struct ast_type        target;
        } cast;

        // Used by; Lambda
        struct
        {
            struct ast_param *param_list;
            size_t            num_params;

            struct ast_type       *result;
            struct ast_expression *expr;
        } lambda;

        // Used by; Struct
        struct
        {
            const char *ident;

            struct ast_argument *args;
            size_t               num_args;
        } struct_call;

        struct
        {
            enum ast_operation     type;
            struct ast_expression *lhs;
            struct ast_expression *rhs;
        } operation;

        // Used by; Call
        struct
        {
            struct ast_expression *function;
            struct ast_expression *args;
            size_t                 num_args;
        } call;

        // Used by; If, While
        struct
        {
            struct ast_expression *cond;
            struct ast_expression *if_true;
            struct ast_expression *if_false;
        } branch;
    } value;
};

struct ast_statement
{
    enum
    {
        E_AST_STMT_LET,
        E_AST_STMT_EXPR,

        E_AST_STMT_IF,
        E_AST_STMT_LOOP,
        E_AST_STMT_WHILE,

        E_AST_STMT_BREAK,
        E_AST_STMT_CONTINUE,
        E_AST_STMT_RETURN,
    } type;

    union
    {
        // Used by; Let
        struct
        {
            const char      *ident;
            struct ast_type *result;

            struct ast_expression expr;
        } let;

        // Used by; If, While
        struct
        {
            struct ast_expression  cond;
            struct ast_expression  if_true;
            struct ast_expression *if_false;
        } branch;

        // Used by; Expr, Break, Return, Loop
        struct ast_expression *expr;
    } value;
};

struct ast_block
{
    struct ast_statement *statements;
    size_t                num_statements;

    struct ast_expression *expression;
};

struct ast_decl
{
    enum
    {
        E_AST_DECL_FUNCTION,
        E_AST_DECL_EXTERN,
        E_AST_DECL_STRUCT,
        E_AST_DECL_ENUM,
        E_AST_DECL_CONST,
    } type;

    union
    {
        // Used by; Function, Extern
        struct
        {
            const char     *ident;
            struct ast_type result;

            struct ast_param *param_list;
            struct ast_block *body;
        } function;

        // Used by; Const
        struct
        {
            const char     *ident;
            struct ast_type result;

            struct ast_expression value;
        } constant;

        // Used by; Enum
        struct
        {
            struct ast_enum_value *values;
        } enum_decl;

        // Used by; Struct
        struct
        {
            struct ast_param *param_list;
        } struct_decl;
    } value;
};

struct ast_program
{
    struct ast_decl *root_nodes;
    size_t           capacity;
    size_t           count;
};

enum ast_parse_error
{
    E_AST_MEMORYERROR = -255,
    E_AST_UNEXPECTED,
    E_AST_INVALIDINPUT,
    E_AST_DELIM,
};

int ast_type_free(struct ast_type *type);
int ast_parm_free(struct ast_param *param);
int ast_enum_value_free(struct ast_enum_value *value);
int ast_argument_free(struct ast_argument *arg);
int ast_expression_free(struct ast_expression *expr);
int ast_statement_free(struct ast_statement *statement);
int ast_decl_free(struct ast_decl *decl);
int ast_program_free(struct ast_program *progrma);

int ast_program_parse(struct ast_program *program, struct token_stream *tokens);
int ast_decl_parse(struct ast_decl *decl, struct token_stream *tokens);