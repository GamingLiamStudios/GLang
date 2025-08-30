#pragma once

#if defined(__cplusplus)
extern "C"
{
#endif
#include <stddef.h>

    struct expr_token
    {
        enum
        {
            E_ETOK_EOF = 0,

            E_ETOK_NUM,
            E_ETOK_LBRACKET,
            E_ETOK_RBRACKET,

            E_ETOK_EXMARK,
            E_ETOK_MINUS,

            E_ETOK_SLASH,
            E_ETOK_STAR,

            E_ETOK_PLUS,

            E_ETOK_GT,
            E_ETOK_EQEQ,
            E_ETOK_LT,

            E_ETOK_ANDAND,
            E_ETOK_BARBAR,
        } type;

        unsigned long value;
    };

    ptrdiff_t expr_lexer_string(struct expr_token **result, const char *input);

#if defined(__cplusplus)
}
#endif