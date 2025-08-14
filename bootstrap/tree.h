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

enum ast_parse_error
{
    E_AST_MEMORYERROR = -255,
    E_AST_UNEXPECTED,
    E_AST_INVALIDINPUT,
    E_AST_DELIM,
    E_AST_INVALIDSTATE,
};

struct glc_parse_expr
{
    enum
    {
        E_GLC_EXPR_INTEGER,
        E_GLC_EXPR_IDENT,

        E_GLC_EXPR_ADD,
        E_GLC_EXPR_SUB,

        E_GLC_EXPR_MUL,
        E_GLC_EXPR_DIV,

        E_GLC_EXPR_NOT,
        E_GLC_EXPR_NEG,

        E_GLC_EXPR_GT,
        E_GLC_EXPR_LT,
        E_GLC_EXPR_EQ,

        E_GLC_EXPR_SCOPE
    } type;

    union
    {
        long  integer;
        char *ident;

        // Also used by scope
        struct glc_parse_expr *children;
    } value;
};

int glc_parse(struct glc_parse_expr *result, struct token *stream);