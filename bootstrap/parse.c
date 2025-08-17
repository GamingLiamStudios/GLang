#include "tree.h"
#include "log.h"

#include <string.h>
#include <stdlib.h>

#if defined(__GNUC__)    // GCC, Clang, ICC

#define unreachable() (__builtin_unreachable())

#elif defined(_MSC_VER)    // MSVC

#define unreachable() (__assume(false))

#else

#error "No Unreachable Defintion exists"

#endif

#define PROGRAM_NODECAPACITY 64
#define GROWTHFACTOR         2

// Sample Grammar
// Root ::= Expr EOF

// Expr ::= Sum
// Expr ::= Expr > Sum
// Expr ::= Expr == Sum
// Expr ::= Expr < Sum

// Sum ::= Product
// Sum ::= Sum + Product
// Sum ::= Sum - Product

// Product ::= Unary
// Product ::= Product * Unary
// Product ::= Product / Unary

// Unary ::= Term
// Unary ::= ! Term
// Unary ::= - Term

// Term ::= Integer
// Term ::= Identifier
// Term ::= ( Expr )

// States;

// I0;
// Root ::= . Expr
// + Expr ::= . Sum
// + Expr ::= . Expr (> == <) Sum
// + Sum ::= . Product
// + Sum ::= . Sum (+, -) Product
// + Product ::= . Unary
// + Product ::= . Product (* /) Unary
// + Unary ::= . Term
// + Unary ::= . (! -) Term
// + Term ::= . Integer
// + Term ::= . Identifier
// + Term ::= . ( Expr )

// I1;
// Root ::= Expr . EOF
// Expr ::= Expr . (> == <) Sum

// I2;
// Expr ::= Sum .
// Sum ::= Sum . (+ -) Product

// I3;
// Sum ::= Product .
// Product ::= Product . (* /) Unary

// I4;
// Product ::= Unary .

// I5;
// Unary ::= ! . Term
// + Term ::= . Integer
// + Term ::= . Identifier
// + Term ::= . ( Expr )

// I6;
// Unary ::= - . Term
// + Term ::= . Integer
// + Term ::= . Identifier
// + Term ::= . ( Expr )

// I7;
// Unary ::= Term .

// I8;
// Term ::= Integer .

// I9;
// Term ::= Identifier .

// I10;
// Term ::= ( . Expr )
// + Expr ::= . Sum
// + Expr ::= . Expr (> == <) Sum
// + Sum ::= . Product
// + Sum ::= . Sum (+ -) Product
// + Product ::= . Unary
// + Product ::= . Product (* /) Unary
// + Unary ::= . Term
// + Unary ::= . (! -) Term
// + Term ::= . Integer
// + Term ::= . Identifier
// + Term ::= . ( Expr )

// I11;
// Expr ::= Expr > . Sum
// + Expr ::= . Sum
// + Expr ::= . Expr (> == <) Sum
// + Sum ::= . Product
// + Sum ::= . Sum (+ -) Product
// + Product ::= . Unary
// + Product ::= . Product (* /) Unary
// + Unary ::= . Term
// + Unary ::= . (! -) Term
// + Term ::= . Integer
// + Term ::= . Identifier
// + Term ::= . ( Expr )

// I12;
// Expr ::= Expr == . Sum
// + Expr ::= . Sum
// + Expr ::= . Expr (> == <) Sum
// + Sum ::= . Product
// + Sum ::= . Sum (+ -) Product
// + Product ::= . Unary
// + Product ::= . Product (* /) Unary
// + Unary ::= . Term
// + Unary ::= . (! -) Term
// + Term ::= . Integer
// + Term ::= . Identifier
// + Term ::= . ( Expr )

// I13;
// Expr ::= Expr < . Sum
// + Expr ::= . Sum
// + Expr ::= . Expr (> == <) Sum
// + Sum ::= . Product
// + Sum ::= . Sum (+ -) Product
// + Product ::= . Unary
// + Product ::= . Product (* /) Unary
// + Unary ::= . Term
// + Unary ::= . (! -) Term
// + Term ::= . Integer
// + Term ::= . Identifier
// + Term ::= . ( Expr )

// I14;
// Sum ::= Sum + . Product
// + Sum ::= . Product
// + Sum ::= . Sum (+ -) Product
// + Product ::= . Unary
// + Product ::= . Product (* /) Unary
// + Unary ::= . Term
// + Unary ::= . (! -) Term
// + Term ::= . Integer
// + Term ::= . Identifier
// + Term ::= . ( Expr )

// I15;
// Sum ::= Sum - . Product
// + Sum ::= . Product
// + Sum ::= . Sum (+ -) Product
// + Product ::= . Unary
// + Product ::= . Product (* /) Unary
// + Unary ::= . Term
// + Unary ::= . (! -) Term
// + Term ::= . Integer
// + Term ::= . Identifier
// + Term ::= . ( Expr )

// I16;
// Product ::= Product * . Unary
// + Product ::= . Unary
// + Product ::= . Product (* /) Unary
// + Unary ::= . Term
// + Unary ::= . (! -) Term
// + Term ::= . Integer
// + Term ::= . Identifier
// + Term ::= . ( Expr )

// I17;
// Product ::= Product / . Unary
// + Product ::= . Unary
// + Product ::= . Product (* /) Unary
// + Unary ::= . Term
// + Unary ::= . (! -) Term
// + Term ::= . Integer
// + Term ::= . Identifier
// + Term ::= . ( Expr )

// I18;
// Unary ::= ! Term .
// Unary ::= - Term .

// I19;
// Term ::= ( Expr . )

// I20;
// Expr ::= Expr > Sum .
// Expr ::= Expr == Sum .
// Expr ::= Expr < Sum .

// I21;
// Sum ::= Sum + Product .
// Sum ::= Sum - Product .

// I22;
// Product ::= Product * Unary .
// Product ::= Product / Unary .

// I23;
// Term ::= ( Expr ) .

// Transition Table;
//      R   E   S   P   U   T Int Id   >  ==   <   +   -   *   /   !   (   )   $
// I0  xx   1   2   3   4   7   8  9  xx  xx  xx  xx   5  xx  xx   6  10  xx  xx
// I1  xx  xx  xx  xx  xx  xx  xx xx  11  12  13  xx  xx  xx  xx  xx  xx  xx  xx
// I2  xx  xx  xx  xx  xx  xx  xx xx  xx  xx  xx  14  15  xx  xx  xx  xx  xx  xx
// I3  xx  xx  xx  xx  xx  xx  xx xx  xx  xx  xx  xx  xx  16  17  xx  xx  xx  xx
// I4  xx  xx  xx  xx  xx  xx  xx xx  xx  xx  xx  xx  xx  xx  xx  xx  xx  xx  xx
// I5  xx  xx  xx  xx  xx  18   8  9  xx  xx  xx  xx  xx  xx  xx  xx  10  xx  xx
// I6  xx  xx  xx  xx  xx  18   8  9  xx  xx  xx  xx  xx  xx  xx  xx  10  xx  xx
// I7  xx  xx  xx  xx  xx  xx  xx xx  xx  xx  xx  xx  xx  xx  xx  xx  xx  xx  xx
// I8  xx  xx  xx  xx  xx  xx  xx xx  xx  xx  xx  xx  xx  xx  xx  xx  xx  xx  xx
// I9  xx  xx  xx  xx  xx  xx  xx xx  xx  xx  xx  xx  xx  xx  xx  xx  xx  xx  xx
// I10 xx   1   2   3   4   7   8  9  xx  xx  xx  xx   5  xx  xx   6  10  xx  xx
// I11 xx  xx  20   3   4   7   8  9  xx  xx  xx  xx   5  xx  xx   6  10  xx  xx
// I12 xx  xx  20   3   4   7   8  9  xx  xx  xx  xx   5  xx  xx   6  10  xx  xx
// I13 xx  xx  20   3   4   7   8  9  xx  xx  xx  xx   5  xx  xx   6  10  xx  xx
// I14 xx  xx  xx  21   4   7   8  9  xx  xx  xx  xx   5  xx  xx   6  10  xx  xx
// I15 xx  xx  xx  21   4   7   8  9  xx  xx  xx  xx   5  xx  xx   6  10  xx  xx
// I16 xx  xx  xx  xx  22   7   8  9  xx  xx  xx  xx   5  xx  xx   6  10  xx  xx
// I17 xx  xx  xx  xx  22   7   8  9  xx  xx  xx  xx   5  xx  xx   6  10  xx  xx
// I18 xx  xx  xx  xx  xx  xx  xx xx  xx  xx  xx  xx  xx  xx  xx  xx  xx  xx  xx
// I19 xx  xx  xx  xx  xx  xx  xx xx  xx  xx  xx  xx  xx  xx  xx  xx  xx  23  xx
// I20 xx  xx  xx  xx  xx  xx  xx xx  xx  xx  xx  xx  xx  xx  xx  xx  xx  xx  xx
// I21 xx  xx  xx  xx  xx  xx  xx xx  xx  xx  xx  xx  xx  xx  xx  xx  xx  xx  xx
// I22 xx  xx  xx  xx  xx  xx  xx xx  xx  xx  xx  xx  xx  xx  xx  xx  xx  xx  xx
// I23 xx  xx  xx  xx  xx  xx  xx xx  xx  xx  xx  xx  xx  xx  xx  xx  xx  xx  xx

// Action Table;
//      R   E   S   P   U   T Int Id   >  ==   <   +   -   *   /   !   (   )   $
// I0  xx  g1  g2  g3  g4  g7  s8 s9  xx  xx  xx  xx  s5  xx  xx  s6 s10  xx  xx
// I1  xx  xx  xx  xx  xx  xx  xx xx s11 s12 s13  xx  xx  xx  xx  xx  xx  xx  acc
// I2  xx  xx  xx  xx  xx  xx  xx xx  xx  xx  xx s14 s15  xx  xx  xx  xx  xx  xx
// I3  xx  xx  xx  xx  xx  xx  xx xx  xx  xx  xx  xx  xx s16 s17  xx  xx  xx  xx
// I4  Reduce 'Product ::= Unary'
// I5  xx  xx  xx  xx  xx g18  s8 s9  xx  xx  xx  xx  xx  xx  xx  xx s10  xx  xx
// I6  xx  xx  xx  xx  xx g18  s8 s9  xx  xx  xx  xx  xx  xx  xx  xx s10  xx  xx
// I7  Reduce 'Unary ::= Term'
// I8  Reduce 'Term ::= Integer'
// I9  Reduce 'Term ::= Identifier'
// I10 xx g19  g2  g3  g4  g7  s8 s9  xx  xx  xx  xx  s5  xx  xx  s6 s10  xx  xx
// I11 xx g20  g2  g3  g4  g7  s8 s9  xx  xx  xx  xx  s5  xx  xx  s6 s10  xx  xx
// I12 xx g20  g2  g3  g4  g7  s8 s9  xx  xx  xx  xx  s5  xx  xx  s6 s10  xx  xx
// I13 xx g20  g2  g3  g4  g7  s8 s9  xx  xx  xx  xx  s5  xx  xx  s6 s10  xx  xx
// I14 xx  xx g21  g3  g4  g7  s8 s9  xx  xx  xx  xx  s5  xx  xx  s6 s10  xx  xx
// I15 xx  xx g21  g3  g4  g7  s8 s9  xx  xx  xx  xx  s5  xx  xx  s6 s10  xx  xx
// I16 xx  xx  xx g22  g4  g7  s8 s9  xx  xx  xx  xx  s5  xx  xx  s6 s10  xx  xx
// I17 xx  xx  xx g22  g4  g7  s8 s9  xx  xx  xx  xx  s5  xx  xx  s6 s10  xx  xx
// I18 Reduce 'Unary ::= (! -) Term'
// I19 xx  xx  xx  xx  xx  xx  xx xx  xx  xx  xx  xx  xx  xx  xx  xx  xx s23  xx
// I20 Reduce 'Expr ::= Expr (> == <) Expr'
// I21 Reduce 'Sum ::= Sum (+ -) Sum'
// I22 Reduce 'Product ::= Product (* /) Product'
// I23 Reduce 'Term ::= ( Expr )'

struct glc_parse_state
{
    int state;

    enum
    {
        E_GLC_PARSE_TOKEN,
        E_GLC_PARSE_EXPR,
        E_GLC_PARSE_SUM,
        E_GLC_PARSE_PRODUCT,
        E_GLC_PARSE_UNARY,
        E_GLC_PARSE_TERM,
    } type;
    union
    {
        struct glcpg_token token;

        struct glc_parse_expr node;
    } value;
};

enum glc_parse_error
{
    E_GLC_ERR_INVALIDINPUT = -127,
    E_GLC_ERR_MEMORY,
    E_GLC_ERR_UNEXPECTED,
    E_GLC_ERR_INVALIDSTATE,
};

int __alloc_parse_stack(struct glc_parse_state **stack, size_t capacity)
{
    struct glc_parse_state *new_ptr;

    if (stack == NULL) { return E_GLC_ERR_INVALIDINPUT; }

    // Since realloc(ptr, 0) is impl specific
    if (capacity == 0)
    {
        free(*stack);
        *stack = NULL;
        return 0;
    }

    new_ptr = realloc(*stack, sizeof(struct glc_parse_state) * capacity);
    if (new_ptr == NULL)
    {
        free(*stack);
        stack = NULL;
        return E_GLC_ERR_MEMORY;
    }

    *stack = new_ptr;
    return 0;
}

int glc_parse(struct glc_parse_expr *result, struct glcpg_token *stream)
{
    struct glc_parse_state *state_stack;
    size_t                  capacity;
    size_t                  state_depth;

    int ret;

    if (stream == NULL || result == NULL) { return E_GLC_ERR_INVALIDINPUT; }

    capacity    = PROGRAM_NODECAPACITY;
    state_depth = 0;
    state_stack = NULL;

    ret = __alloc_parse_stack(&state_stack, capacity);
    if (ret < 0) { return ret; }

    // Init stack with EOF
    state_stack[0] = (struct glc_parse_state) { .state       = 0,
                                                .type        = E_GLC_PARSE_TOKEN,
                                                .value.token = (struct glcpg_token) {
                                                  .value = E_TOKEN_EOF,
                                                } };

    while (1)
    {
        struct glc_parse_state *head;

        if (state_depth == capacity)
        {
            capacity *= GROWTHFACTOR;
            ret = __alloc_parse_stack(&state_stack, capacity);
            if (ret < 0) { return ret; }
        }

        head = state_stack + state_depth;
        switch (head->state)
        {
        case 0:
        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
        case 16:
        case 17:
        {
            // Root ::= . Expr

            switch (stream->value)
            {
            default:
                glc_log(
                  E_ERROR,
                  "Unexpected token at %d:%d\n",
                  stream->debug_info.line,
                  stream->debug_info.column);
                return E_GLC_ERR_UNEXPECTED;

            case E_TOKEN_INTEGER:
                // Shift -> I8
                state_stack[++state_depth] = (struct glc_parse_state) {
                    .state       = 8,
                    .type        = E_GLC_PARSE_TOKEN,
                    .value.token = *stream,
                };
                stream += 1;
                break;

            case E_TOKEN_IDENT:
                // Shift -> I9
                state_stack[++state_depth] = (struct glc_parse_state) {
                    .state       = 9,
                    .type        = E_GLC_PARSE_TOKEN,
                    .value.token = *stream,
                };
                stream += 1;
                break;

            case E_TOKEN_MINUS:
                // Shift -> I5
                state_stack[++state_depth] = (struct glc_parse_state) {
                    .state       = 5,
                    .type        = E_GLC_PARSE_TOKEN,
                    .value.token = *stream,
                };
                stream += 1;
                break;

            case E_TOKEN_EXMARK:
                // Shift -> I6
                state_stack[++state_depth] = (struct glc_parse_state) {
                    .state       = 6,
                    .type        = E_GLC_PARSE_TOKEN,
                    .value.token = *stream,
                };
                stream += 1;
                break;

            case E_TOKEN_LBRACE:
                // Shift -> I10
                state_stack[++state_depth] = (struct glc_parse_state) {
                    .state       = 6,
                    .type        = E_GLC_PARSE_TOKEN,
                    .value.token = *stream,
                };
                stream += 1;
                break;
            }

            break;
        }

        case 1:
        {
            // Root ::= Expr . EOF
            // Root ::= Expr . (> == <) Expr

            switch (stream->value)
            {
            default:
                glc_log(
                  E_ERROR,
                  "Unexpected token at %d:%d\n",
                  stream->debug_info.line,
                  stream->debug_info.column);
                return E_GLC_ERR_UNEXPECTED;

            case E_TOKEN_EOF:
                // Fully parsed input; return success
                glc_log(E_DEBUG, "Stack size: %d\n", state_depth);
                return 0;

            case E_TOKEN_LT:
                // Shift -> I11
                state_stack[++state_depth] = (struct glc_parse_state) {
                    .state       = 11,
                    .type        = E_GLC_PARSE_TOKEN,
                    .value.token = *stream,
                };
                stream += 1;
                break;

            case E_TOKEN_EQEQ:
                // Shift -> I12
                state_stack[++state_depth] = (struct glc_parse_state) {
                    .state       = 12,
                    .type        = E_GLC_PARSE_TOKEN,
                    .value.token = *stream,
                };
                stream += 1;
                break;

            case E_TOKEN_GT:
                // Shift -> I13
                state_stack[++state_depth] = (struct glc_parse_state) {
                    .state       = 13,
                    .type        = E_GLC_PARSE_TOKEN,
                    .value.token = *stream,
                };
                stream += 1;
                break;
            }

            break;
        }
        case 2:
        {
            // Expr ::= Sum .
            // Sum ::= Sum . (+ -) Sum
            switch (stream->value)
            {
            default:
                glc_log(
                  E_ERROR,
                  "Unexpected token at %d:%d\n",
                  stream->debug_info.line,
                  stream->debug_info.column);
                return E_GLC_ERR_UNEXPECTED;

            case E_TOKEN_PLUS:
                // Shift -> I14
                state_stack[++state_depth] = (struct glc_parse_state) {
                    .state       = 14,
                    .type        = E_GLC_PARSE_TOKEN,
                    .value.token = *stream,
                };
                stream += 1;
                break;

            case E_TOKEN_MINUS:
                // Shift -> I15
                state_stack[++state_depth] = (struct glc_parse_state) {
                    .state       = 15,
                    .type        = E_GLC_PARSE_TOKEN,
                    .value.token = *stream,
                };
                stream += 1;
                break;
            }

            break;
        }
        case 3:
        {
            // Sum ::= Product .
            // Product ::= Product . (* /) Product
            switch (stream->value)
            {
            default:
                glc_log(
                  E_ERROR,
                  "Unexpected token at %d:%d\n",
                  stream->debug_info.line,
                  stream->debug_info.column);
                return E_GLC_ERR_UNEXPECTED;

            case E_TOKEN_STAR:
                // Shift -> I16
                state_stack[++state_depth] = (struct glc_parse_state) {
                    .state       = 16,
                    .type        = E_GLC_PARSE_TOKEN,
                    .value.token = *stream,
                };
                stream += 1;
                break;

            case E_TOKEN_SLASH:
                // Shift -> I17
                state_stack[++state_depth] = (struct glc_parse_state) {
                    .state       = 17,
                    .type        = E_GLC_PARSE_TOKEN,
                    .value.token = *stream,
                };
                stream += 1;
                break;
            }

            break;
        }
        case 4:
        {
            // Product ::= Unary .
            if (state_depth < 1 || head->type != E_GLC_PARSE_UNARY)
            {
                return E_GLC_ERR_INVALIDSTATE;
            }
            head->type = E_GLC_PARSE_PRODUCT;
            switch (state_stack[state_depth - 1].state)
            {
            default: break;

            case 0:
            case 10:
            case 11:
            case 12:
            case 13:
            case 14:
            case 15: head->state = 3; break;

            case 16:
            case 17: head->state = 22; break;
            }

            break;
        }
        case 5:
        {
            // Unary ::= ! . Term
            switch (stream->value)
            {
            default:
                glc_log(
                  E_ERROR,
                  "Unexpected token at %d:%d\n",
                  stream->debug_info.line,
                  stream->debug_info.column);
                return E_GLC_ERR_UNEXPECTED;

            case E_TOKEN_INTEGER:
                // Shift -> I8
                state_stack[++state_depth] = (struct glc_parse_state) {
                    .state       = 8,
                    .type        = E_GLC_PARSE_TOKEN,
                    .value.token = *stream,
                };
                stream += 1;
                break;

            case E_TOKEN_IDENT:
                // Shift -> I9
                state_stack[++state_depth] = (struct glc_parse_state) {
                    .state       = 9,
                    .type        = E_GLC_PARSE_TOKEN,
                    .value.token = *stream,
                };
                stream += 1;
                break;

            case E_TOKEN_LBRACE:
                // Shift -> I10
                state_stack[++state_depth] = (struct glc_parse_state) {
                    .state       = 10,
                    .type        = E_GLC_PARSE_TOKEN,
                    .value.token = *stream,
                };
                stream += 1;
                break;
            }
            break;
        }
        case 6:
        {
            // Unary ::= - . Term
            switch (stream->value)
            {
            default:
                glc_log(
                  E_ERROR,
                  "Unexpected token at %d:%d\n",
                  stream->debug_info.line,
                  stream->debug_info.column);
                return E_GLC_ERR_UNEXPECTED;

            case E_TOKEN_INTEGER:
                // Shift -> I8
                state_stack[++state_depth] = (struct glc_parse_state) {
                    .state       = 8,
                    .type        = E_GLC_PARSE_TOKEN,
                    .value.token = *stream,
                };
                stream += 1;
                break;

            case E_TOKEN_IDENT:
                // Shift -> I9
                state_stack[++state_depth] = (struct glc_parse_state) {
                    .state       = 9,
                    .type        = E_GLC_PARSE_TOKEN,
                    .value.token = *stream,
                };
                stream += 1;
                break;

            case E_TOKEN_LBRACE:
                // Shift -> I10
                state_stack[++state_depth] = (struct glc_parse_state) {
                    .state       = 10,
                    .type        = E_GLC_PARSE_TOKEN,
                    .value.token = *stream,
                };
                stream += 1;
                break;
            }
            break;
        }
        case 7:
        {
            // Unary ::= Term .
            if (state_depth < 1 || head->type != E_GLC_PARSE_TERM)
            {
                return E_GLC_ERR_INVALIDSTATE;
            }
            head->type = E_GLC_PARSE_UNARY;
            switch (state_stack[state_depth - 1].state)
            {
            default: break;

            case 0:
            case 10:
            case 11:
            case 12:
            case 13:
            case 14:
            case 15:
            case 16:
            case 17: head->state = 4; break;
            }

            break;
        }
        case 8:
        {
            struct glcpg_token tok;

            if (state_depth < 1 || head->type != E_GLC_PARSE_TOKEN)
            {
                return E_GLC_ERR_INVALIDSTATE;
            }
            tok = head->value.token;
            switch (tok.value)
            {
            default:
                glc_log(
                  E_ERROR,
                  "Unexpected token at %d:%d (Expected Integer)\n",
                  stream->debug_info.line,
                  stream->debug_info.column);
                return E_GLC_ERR_UNEXPECTED;

            case E_TOKEN_INTEGER:
                head->value.node = (struct glc_parse_expr) {
                    .type          = E_GLC_EXPR_INTEGER,
                    .value.integer = tok.data.integer,
                };
                break;
            }

            head->type = E_GLC_PARSE_TERM;
            switch (state_stack[state_depth - 1].state)
            {
            default: break;

            case 0:
            case 10:
            case 11:
            case 12:
            case 13:
            case 14:
            case 15:
            case 16:
            case 17: head->state = 7; break;

            case 5:
            case 6: head->state = 18; break;
            }

            break;
        }
        case 9:
        {
            struct glcpg_token tok;

            if (state_depth < 1 || head->type != E_GLC_PARSE_TOKEN)
            {
                return E_GLC_ERR_INVALIDSTATE;
            }
            tok = head->value.token;
            switch (tok.value)
            {
            default:
                glc_log(
                  E_ERROR,
                  "Unexpected token at %d:%d (Expected Identifier)\n",
                  stream->debug_info.line,
                  stream->debug_info.column);
                return E_GLC_ERR_UNEXPECTED;

            case E_TOKEN_STRING:
            {
                size_t len       = strlen(tok.data.string);
                head->value.node = (struct glc_parse_expr) {
                    .type        = E_GLC_EXPR_IDENT,
                    .value.ident = calloc(len + 1, sizeof(char)),
                };
                strncpy(head->value.node.value.ident, tok.data.string, len);
                break;
            }
            }

            head->type = E_GLC_PARSE_TERM;
            switch (state_stack[state_depth - 1].state)
            {
            default: break;

            case 0:
            case 10:
            case 11:
            case 12:
            case 13:
            case 14:
            case 15:
            case 16:
            case 17: head->state = 7; break;

            case 5:
            case 6: head->state = 18; break;
            }

            break;
        }
        case 18:
        {
            // Unary ::= (! -) Term .
            struct glc_parse_expr *children;

            if (
              state_depth < 2 || state_stack[state_depth - 0].type != E_GLC_PARSE_TERM ||
              state_stack[state_depth - 1].type != E_GLC_PARSE_TOKEN)
            {
                return E_GLC_ERR_INVALIDSTATE;
            }

            children = calloc(1, sizeof(struct glc_parse_expr));
            memcpy(children + 0, &head->value.node, sizeof(struct glc_parse_expr));

            head -= 1;
            head->value.node.value.children = children;

            switch (state_stack[state_depth - 1].value.token.value)
            {
            default:
                glc_log(
                  E_ERROR,
                  "Unexpected token at %d:%d (Expected '!' or '-')\n",
                  stream->debug_info.line,
                  stream->debug_info.column);
                return E_GLC_ERR_UNEXPECTED;

            case E_TOKEN_EXMARK: head->value.node.type = E_GLC_EXPR_NOT; break;
            case E_TOKEN_MINUS: head->value.node.type = E_GLC_EXPR_NEG; break;
            }

            head->type = E_GLC_PARSE_UNARY;
            switch (state_stack[state_depth - 2].state)
            {
            default: break;

            case 0:
            case 10:
            case 11:
            case 12:
            case 13:
            case 14:
            case 15:
            case 16:
            case 17: head->state = 4; break;
            }

            state_depth -= 1;
            break;
        }
        case 19:
        {
            // Term ::= ( Expr . )
            switch (stream->value)
            {
            default:
                glc_log(
                  E_ERROR,
                  "Unexpected token at %d:%d\n",
                  stream->debug_info.line,
                  stream->debug_info.column);
                return E_GLC_ERR_UNEXPECTED;

            case E_TOKEN_RBRACE:
                // Shift -> I23
                state_stack[++state_depth] = (struct glc_parse_state) {
                    .state       = 23,
                    .type        = E_GLC_PARSE_TOKEN,
                    .value.token = *stream,
                };
                stream += 1;
                break;
            }
            break;
        }
        case 20:
        {
            // Expr ::= Expr (> == <) Expr .
            struct glc_parse_expr *children;

            if (
              state_depth < 3 || state_stack[state_depth - 0].type != E_GLC_PARSE_EXPR ||
              state_stack[state_depth - 1].type != E_GLC_PARSE_TOKEN ||
              state_stack[state_depth - 2].type != E_GLC_PARSE_EXPR)
            {
                return E_GLC_ERR_INVALIDSTATE;
            }

            children = calloc(2, sizeof(struct glc_parse_expr));
            memcpy(
              children + 0,
              &state_stack[state_depth - 0].value.node,
              sizeof(struct glc_parse_expr));
            memcpy(
              children + 1,
              &state_stack[state_depth - 2].value.node,
              sizeof(struct glc_parse_expr));

            head -= 2;
            head->value.node.value.children = children;

            switch (state_stack[state_depth - 1].value.token.value)
            {
            default:
                glc_log(
                  E_ERROR,
                  "Unexpected token at %d:%d (Expected '>', '==' or '<')\n",
                  stream->debug_info.line,
                  stream->debug_info.column);
                return E_GLC_ERR_UNEXPECTED;

            case E_TOKEN_LT: head->value.node.type = E_GLC_EXPR_LT; break;
            case E_TOKEN_EQEQ: head->value.node.type = E_GLC_EXPR_EQ; break;
            case E_TOKEN_GT: head->value.node.type = E_GLC_EXPR_GT; break;
            }

            head->type = E_GLC_PARSE_EXPR;
            switch (state_stack[state_depth - 3].state)
            {
            default: break;

            case 0: head->state = 1; break;

            case 10: head->state = 19; break;

            case 11:
            case 12:
            case 13: head->state = 20; break;
            }

            state_depth -= 2;
            break;
        }
        case 21:
        {
            // Sum ::= Sum (+ -) Sum .
            struct glc_parse_expr *children;

            if (
              state_depth < 3 || state_stack[state_depth - 0].type != E_GLC_PARSE_SUM ||
              state_stack[state_depth - 1].type != E_GLC_PARSE_TOKEN ||
              state_stack[state_depth - 2].type != E_GLC_PARSE_SUM)
            {
                return E_GLC_ERR_INVALIDSTATE;
            }

            children = calloc(2, sizeof(struct glc_parse_expr));
            memcpy(
              children + 0,
              &state_stack[state_depth - 0].value.node,
              sizeof(struct glc_parse_expr));
            memcpy(
              children + 1,
              &state_stack[state_depth - 2].value.node,
              sizeof(struct glc_parse_expr));

            head -= 2;
            head->value.node.value.children = children;

            switch (state_stack[state_depth - 1].value.token.value)
            {
            default:
                glc_log(
                  E_ERROR,
                  "Unexpected token at %d:%d (Expected '+' or '-')\n",
                  stream->debug_info.line,
                  stream->debug_info.column);
                return E_GLC_ERR_UNEXPECTED;

            case E_TOKEN_PLUS: head->value.node.type = E_GLC_EXPR_ADD; break;
            case E_TOKEN_MINUS: head->value.node.type = E_GLC_EXPR_SUB; break;
            }

            head->type = E_GLC_PARSE_SUM;
            switch (state_stack[state_depth - 3].state)
            {
            default: break;

            case 0:
            case 10:
            case 11:
            case 12:
            case 13: head->state = 2; break;

            case 14:
            case 15: head->state = 21; break;
            }

            state_depth -= 2;
            break;
        }
        case 22:
        {
            // Product ::= Product (* /) Product .
            struct glc_parse_expr *children;

            if (
              state_depth < 3 || state_stack[state_depth - 0].type != E_GLC_PARSE_PRODUCT ||
              state_stack[state_depth - 1].type != E_GLC_PARSE_TOKEN ||
              state_stack[state_depth - 2].type != E_GLC_PARSE_PRODUCT)
            {
                return E_GLC_ERR_INVALIDSTATE;
            }

            children = calloc(2, sizeof(struct glc_parse_expr));
            memcpy(
              children + 0,
              &state_stack[state_depth - 0].value.node,
              sizeof(struct glc_parse_expr));
            memcpy(
              children + 1,
              &state_stack[state_depth - 2].value.node,
              sizeof(struct glc_parse_expr));

            head -= 2;
            head->value.node.value.children = children;

            switch (state_stack[state_depth - 1].value.token.value)
            {
            default:
                glc_log(
                  E_ERROR,
                  "Unexpected token at %d:%d (Expected '*' or '/')\n",
                  stream->debug_info.line,
                  stream->debug_info.column);
                return E_GLC_ERR_UNEXPECTED;

            case E_TOKEN_STAR: head->value.node.type = E_GLC_EXPR_MUL; break;
            case E_TOKEN_SLASH: head->value.node.type = E_GLC_EXPR_DIV; break;
            }

            head->type = E_GLC_PARSE_PRODUCT;
            switch (state_stack[state_depth - 3].state)
            {
            default: break;

            case 0:
            case 10:
            case 11:
            case 12:
            case 13:
            case 14:
            case 15: head->state = 3; break;

            case 16:
            case 17: head->state = 22; break;
            }

            state_depth -= 2;
            break;
        }
        case 23:
        {
            // Term ::= ( Expr ) .
            struct glc_parse_expr *children;

            if (
              state_depth < 3 || state_stack[state_depth - 0].type != E_GLC_PARSE_TOKEN ||
              state_stack[state_depth - 1].type != E_GLC_PARSE_EXPR ||
              state_stack[state_depth - 2].type != E_GLC_PARSE_TOKEN)
            {
                return E_GLC_ERR_INVALIDSTATE;
            }

            children = calloc(1, sizeof(struct glc_parse_expr));
            memcpy(
              children + 0,
              &state_stack[state_depth - 1].value.node,
              sizeof(struct glc_parse_expr));

            head -= 2;
            head->value.node.value.children = children;
            head->value.node.type           = E_GLC_EXPR_SCOPE;

            head->type = E_GLC_PARSE_TERM;
            switch (state_stack[state_depth - 3].state)
            {
            default: break;

            case 0:
            case 10:
            case 11:
            case 12:
            case 13:
            case 14:
            case 15:
            case 16:
            case 17: head->state = 7; break;

            case 5:
            case 6: head->state = 18; break;
            }

            state_depth -= 2;
            break;
        }
        }
    }
}