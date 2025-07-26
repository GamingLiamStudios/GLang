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
    E_AST_OP_AND,    // &
    E_AST_OP_OR,     // |
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
    E_AST_OP_INVERT,    // ~
};

struct ast_type
{
    const char *type_name;
    // TODO: Generics
};

struct ast_expression
{
    enum
    {
        E_AST_EXPR_CONSTANT = -127,
        E_AST_EXPR_VARIABLE,

        E_AST_EXPR_BLOCK,
        E_AST_EXPR_SCOPE,

        E_AST_EXPR_CALL,
        E_AST_EXPR_LOOP,

        E_AST_EXPR_LET,
        E_AST_EXPR_CAST,

        E_AST_EXPR_OPER,

        E_AST_EXPR_IF,
        E_AST_EXPR_WHILE,
    } type;

    // TODO: Debug info

    union
    {
        // Used by; Variable
        const char *variable_ident;

        // Used by; Constant
        struct
        {
            enum
            {
                E_AST_CONST_STRING,
                E_AST_CONST_INTEGER,
                E_AST_CONST_FLOATING,
            } type;

            union
            {
                // Sign is handled by negate operation
                unsigned long integer;
                const char   *string;

                struct
                {
                    signed long   integer;
                    unsigned long fractional;
                } floating;
            } value;
        } constant;

        // Used by; Block
        struct
        {
            struct ast_statement *statements;
            size_t                num_statements;

            struct ast_expression *expression;    // NULL if void
        } block;

        // Used by; Scope, Loop
        struct ast_expression *expr;

        // Used by; Call
        struct
        {
            // Includes function as expressions[0]
            struct ast_expression **expressions;
            size_t                  num_expressions;
        } call;

        // Used by; Let
        struct
        {
            const char      *ident;
            struct ast_type *type;

            struct ast_expression *expression;
        } let;

        // Used by; Cast
        struct
        {
            struct ast_expression *expression;
            struct ast_type        target;
        } cast;

        // Used by; Operation
        struct
        {
            enum ast_operation     op;
            struct ast_expression *lhs;
            struct ast_expression *rhs;
        } oper;

        // Used by; If, While
        struct
        {
            struct ast_expression *condition;

            struct ast_expression *if_true;
            struct ast_expression *if_false;
        } branch;
    } value;
};

struct ast_statement
{
    enum
    {
        E_AST_STMT_EXPR,
        E_AST_STMT_RETURN,
        E_AST_STMT_BREAK,

        E_AST_STMT_CONTINUE,
    } type;

    // Why union when (most) types use expr? future proofing ig
    union
    {
        struct ast_expression *expr;    // NULL if void
    } data;
};

struct ast_node
{
    enum
    {
        E_AST_NODE_FUNCTION,
        E_AST_NODE_EXTERNAL,
        E_AST_NODE_CONSTANT,
    } type;

    const char     *ident;
    struct ast_type node_type;

    union
    {
        // Used by; Function, External
        struct
        {
            const char     **param_idents;
            struct ast_type *param_types;
            size_t           num_params;

            struct ast_expression *body;    // NULL if external
        } function;

        // Used by; Constant
        struct ast_expression value;
    } data;
};

struct ast_program
{
    struct ast_node *nodes;
    size_t           num_nodes;
    size_t           node_capacity;
};

void ast_program_free(struct ast_program *program);
void ast_node_free(struct ast_node *node);
void ast_statement_free(struct ast_statement *statement);
void ast_expression_free(struct ast_expression *expression);
void ast_type_free(struct ast_type *type);

enum ast_program_create_error
{
    E_AST_MEMORYERROR = -255,
    E_AST_UNEXPECTED,
    E_AST_INVALIDINPUT,
    E_AST_DELIM,
};

int ast_program_from_tokens(struct ast_program *program, struct token_stream *token_stream);