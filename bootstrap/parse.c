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

#define PROGRAM_NODECAPACITY 256
#define PROGRAM_NODEGROWTH   2

// FIXME: Thread-safe this plz
static char token_buffer[1024];

ptrdiff_t ast_parse_expression(struct ast_expression *result, struct token *stream);
ptrdiff_t ast_parse_statement(struct ast_statement *result, struct token *stream);

int ast_program_grow(struct ast_program *program)
{
    if (program == NULL) { return E_AST_INVALIDINPUT; }
    if (program->capacity == program->count) { return 0; }
    program->capacity *= PROGRAM_NODEGROWTH;

    struct token *resized =
      realloc(program->root_nodes, sizeof(struct ast_decl) * program->capacity);
    if (resized == NULL)
    {
        ast_program_free(program);
        return E_AST_MEMORYERROR;
    }

    return 0;
}

/// Returns number of Tokens parsed, or negative for Error
ptrdiff_t ast_parse_type(struct ast_type *result, struct token *stream)
{
    struct token *start, *typename;
    size_t        typename_len;

    if (result == NULL || stream == NULL) { return E_AST_INVALIDINPUT; }

    start    = stream;
    typename = stream++;
    if (typename->value != E_TOKEN_IDENT)
    {
        glc_log(
          E_ERROR,
          "Invalid Typename at %d:%d\n",
          typename->debug_info.line,
          typename->debug_info.column);
        return E_AST_UNEXPECTED;
    }

    typename_len     = strlen(typename->data.string);
    result->typename = calloc(typename_len + 1, sizeof(char));
    strncpy((char *) result->typename, typename->data.string, typename_len);

    return stream - start;
}

/// Returns number of Tokens parsed, or negative for Error
ptrdiff_t ast_parse_expression(struct ast_expression *result, struct token *stream)
{
    struct token *start;
    ptrdiff_t     ret;

    // Parse expression as follows;
    // - Read Token
    // - Lookahead to next token
    //  - If Semicolon, exit
    //  - If Valid EXPR operator, continue into expr type
    //  - If Invalid EXPR operator (given current state), exit

    if (result == NULL || stream == NULL) { return E_AST_INVALIDINPUT; }

    start = stream;

    switch ((stream++)->value)
    {
    // Can be; Primary
    case E_TOKEN_INTEGER:
    {
        return 1;
    }

    // Can be; Assign, Struct, Primary
    case E_TOKEN_IDENT:
    {
        struct token *ident = stream - 1;

        // Lookahead
        switch ((stream++)->value)
        {
        // Primary (Ident)
        case E_TOKEN_SEMICOLON:
        {
            result->type = E_AST_EXPR_VARIABLE;

            size_t len                   = strlen(ident->data.string);
            result->value.variable_ident = calloc(len + 1, sizeof(char));
            strncpy((char *) result->value.variable_ident, ident->data.string, len);

            return 1;
        }

        // Struct
        case E_TOKEN_LBRACE:
        {
            // TODO: Implement
            token_debug_str(token_buffer, sizeof(token_buffer), stream);
            glc_log(E_ERROR, "Structs Unimplemented! (at %s)\n", token_buffer);
            return -1;
        }

        // Assign
        case E_TOKEN_EQUAL:
        {
            struct ast_expression value;
            ret = ast_parse_expression(&value, stream);
            if (ret < 0) { return ret; }

            result->type = E_AST_EXPR_ASSIGN;

            size_t len                 = strlen(ident->data.string);
            result->value.assign.ident = calloc(len + 1, sizeof(char));
            strncpy((char *) result->value.assign.ident, ident->data.string, len);

            result->value.assign.expr = calloc(1, sizeof(struct ast_expression));
            memcpy(result->value.assign.expr, &value, sizeof(struct ast_expression));

            return 2 + ret;
        }

        // Operator-Assign
        case E_TOKEN_PLUSEQUAL:
        case E_TOKEN_MINUSEQUAL:
        case E_TOKEN_STAREQUAL:
        case E_TOKEN_SLASHEQUAL:
        case E_TOKEN_BAREQUAL:
        case E_TOKEN_ANDEQUAL:
        {
            // TODO: Implement
            token_debug_str(token_buffer, sizeof(token_buffer), stream);
            glc_log(E_ERROR, "Operator-Assign Unimplemented! (at %s)\n", token_buffer);
            return -1;
        }
        }
    }
    }

    stream -= 1;
    token_debug_str(token_buffer, sizeof(token_buffer), stream);
    glc_log(
      E_ERROR,
      "Unexpected Token %s at %d:%d\n",
      token_buffer,
      stream->debug_info.line,
      stream->debug_info.column);

    return -1;
}

/// Returns number of Tokens parsed, or negative for Error
ptrdiff_t ast_parse_constant(struct ast_decl *result, struct token *stream)
{
    struct token         *start, *ident;
    struct ast_type       type;
    struct ast_expression value;
    ptrdiff_t             ret;

    if (result == NULL || stream == NULL) { return E_AST_INVALIDINPUT; }

    start = stream;
    if ((stream++)->value != E_TOKEN_CONST)
    {
        token_debug_str(token_buffer, sizeof(token_buffer), stream - 1);
        glc_log(
          E_ERROR,
          "Unexpected Token `%s` at %d:%d\n",
          token_buffer,
          stream->debug_info.line,
          stream->debug_info.column);
        return E_AST_UNEXPECTED;
    }

    ident = stream++;
    if (ident->value != E_TOKEN_IDENT)
    {
        token_debug_str(token_buffer, sizeof(token_buffer), ident);
        glc_log(
          E_ERROR,
          "Unexpected Token `%s` at %d:%d (expected Ident)\n",
          token_buffer,
          stream->debug_info.line,
          stream->debug_info.column);
        return E_AST_UNEXPECTED;
    }

    if ((stream++)->value != E_TOKEN_COLON)
    {
        token_debug_str(token_buffer, sizeof(token_buffer), stream - 1);
        glc_log(
          E_ERROR,
          "Unexpected Token `%s` at %d:%d (expected ':')\n",
          token_buffer,
          stream->debug_info.line,
          stream->debug_info.column);
        return E_AST_UNEXPECTED;
    }

    ret = ast_parse_type(&type, stream);
    if (ret < 0) { return E_AST_UNEXPECTED; }
    stream += ret;

    if ((stream++)->value != E_TOKEN_EQUAL)
    {
        token_debug_str(token_buffer, sizeof(token_buffer), stream - 1);
        glc_log(
          E_ERROR,
          "Unexpected Token `%s` at %d:%d (expected '=')\n",
          token_buffer,
          stream->debug_info.line,
          stream->debug_info.column);
        return E_AST_UNEXPECTED;
    }

    ret = ast_parse_expression(&value, stream);
    if (ret < 0) { return E_AST_UNEXPECTED; }
    stream += ret;

    if ((stream++)->value != E_TOKEN_SEMICOLON)
    {
        token_debug_str(token_buffer, sizeof(token_buffer), stream - 1);
        glc_log(
          E_ERROR,
          "Unexpected Token `%s` at %d:%d (expected ';')\n",
          token_buffer,
          stream->debug_info.line,
          stream->debug_info.column);
        return E_AST_UNEXPECTED;
    }

    result->type = E_AST_DECL_CONST;

    size_t len                   = strlen(ident->data.string);
    result->value.constant.ident = calloc(len + 1, sizeof(char));
    strncpy((char *) result->value.constant.ident, ident->data.string, len);

    result->value.constant.result = type;
    result->value.constant.value  = value;

    return stream - start;
}

// TODO: Make this more of a Stack than a Linked List
struct ast_parse_state
{
    enum
    {
        E_AST_PARSE_ROOT,    // at Root-level

        E_AST_PARSE_CONST_1,
        E_AST_PARSE_CONST_2,
        E_AST_PARSE_CONST_3,
        E_AST_PARSE_CONST_4,

        E_AST_PARSE_TYPE_ROOT,

        E_AST_PARSE_EXPR_ROOT,
        E_AST_PARSE_EXPR_CONST_INT,
        E_AST_PARSE_EXPR_CONST_STR,
        // E_AST_PARSE_EXPR_CONST_FLOAT, // TODO
    } state;

    struct ast_parse_state *prev_state;

    union
    {
        struct ast_program root;

        struct
        {
            char           *ident;
            struct ast_type type;

            struct ast_expression value;
        } const_decl;

        struct ast_expression expr;

        // struct ast_type type; For when TYPE becomes more complex
    } data;
};

/// Returns negative for Error, 0 for success
/// Mostly written as a LR(0), with occasional LR(1)
int ast_parse_next(struct ast_parse_state **state, struct token *stream)
{
    struct ast_parse_state *cur;
    struct ast_parse_state *next;
    if (state == NULL || *state == NULL || stream == NULL) { return E_AST_INVALIDINPUT; }

    cur = *state;

    switch (cur->state)
    {
    case E_AST_PARSE_ROOT:
    {
        // We at root level of program; can branch into any Decl from here
        switch (stream->value)
        {
        default:
            token_debug_str(token_buffer, sizeof(token_buffer), stream);
            glc_log(
              E_ERROR,
              "Unexpected Token `%s` at %d:%d (expected ';')\n",
              token_buffer,
              stream->debug_info.line,
              stream->debug_info.column);
            return E_AST_UNEXPECTED;
        case E_TOKEN_EOF: return 0;

        case E_TOKEN_CONST:
        {
            next             = calloc(1, sizeof(struct ast_parse_state));
            next->prev_state = cur;

            next->state = E_AST_PARSE_CONST_1;
            *state      = next;
            return 0;
        }
        }
        break;
    }

    case E_AST_PARSE_TYPE_ROOT:
    {
        if (stream->value != E_TOKEN_IDENT)
        {
            glc_log(
              E_ERROR,
              "Invalid Typename at %d:%d\n",
              stream->debug_info.line,
              stream->debug_info.column);
            return E_AST_UNEXPECTED;
        }

        struct ast_type type;

        size_t len    = strlen(stream->data.string);
        type.typename = calloc(len + 1, sizeof(char));
        strncpy((char *) type.typename, stream->data.string, len);

        if (cur->prev_state == NULL)
        {
            glc_log(
              E_ERROR,
              "Invalid State! Type attached to no decl\n",
              stream->debug_info.line,
              stream->debug_info.column);
            return E_AST_INVALIDSTATE;
        }

        switch (cur->prev_state->state)
        {
        default:
            glc_log(
              E_ERROR,
              "Invalid State! Type attached to no decl\n",
              stream->debug_info.line,
              stream->debug_info.column);
            return E_AST_INVALIDSTATE;

        case E_AST_PARSE_CONST_2:
            *state = cur->prev_state;
            free(cur);

            (*state)->data.const_decl.type = type;
            (*state)->state                = E_AST_PARSE_CONST_3;

            return 0;
        }

        break;
    }

    case E_AST_PARSE_CONST_1:
    {
        if (stream->value != E_TOKEN_IDENT)
        {
            token_debug_str(token_buffer, sizeof(token_buffer), stream);
            glc_log(
              E_ERROR,
              "Unexpected Token `%s` at %d:%d (expected 'Ident')\n",
              token_buffer,
              stream->debug_info.line,
              stream->debug_info.column);
            return E_AST_UNEXPECTED;
        }

        size_t len                 = strlen(stream->data.string);
        cur->data.const_decl.ident = calloc(len + 1, sizeof(char));
        strncpy(cur->data.const_decl.ident, stream->data.string, len);

        cur->state = E_AST_PARSE_CONST_2;
        return 0;
    }
    case E_AST_PARSE_CONST_2:
    {
        if (stream->value != E_TOKEN_COLON)
        {
            token_debug_str(token_buffer, sizeof(token_buffer), stream);
            glc_log(
              E_ERROR,
              "Unexpected Token `%s` at %d:%d (expected ':')\n",
              token_buffer,
              stream->debug_info.line,
              stream->debug_info.column);
            return E_AST_UNEXPECTED;
        }

        next             = calloc(1, sizeof(struct ast_parse_state));
        next->prev_state = cur;
        next->state      = E_AST_PARSE_TYPE_ROOT;

        *state = next;
        return 0;
    }

    case E_AST_PARSE_CONST_3:
    {
        if (stream->value != E_TOKEN_EQUAL)
        {
            token_debug_str(token_buffer, sizeof(token_buffer), stream);
            glc_log(
              E_ERROR,
              "Unexpected Token `%s` at %d:%d (expected '=')\n",
              token_buffer,
              stream->debug_info.line,
              stream->debug_info.column);
            return E_AST_UNEXPECTED;
        }

        next             = calloc(1, sizeof(struct ast_parse_state));
        next->prev_state = cur;
        next->state      = E_AST_PARSE_EXPR_ROOT;

        *state = next;
        return 0;
    }
    case E_AST_PARSE_CONST_4:
    {
        if (stream->value != E_TOKEN_SEMICOLON)
        {
            token_debug_str(token_buffer, sizeof(token_buffer), stream);
            glc_log(
              E_ERROR,
              "Unexpected Token `%s` at %d:%d (expected ';')\n",
              token_buffer,
              stream->debug_info.line,
              stream->debug_info.column);
            return E_AST_UNEXPECTED;
        }

        // FIXME: Should probably be two different 'invalid states'
        if (cur->prev_state == NULL || cur->prev_state->state != E_AST_PARSE_ROOT)
        {
            glc_log(
              E_ERROR,
              "Invalid State! Const closing from outside ParseRoot\n",
              stream->debug_info.line,
              stream->debug_info.column);
            return E_AST_INVALIDSTATE;
        }

        struct ast_program *root = &(*state)->data.root;
        ast_program_grow(root);

        root->root_nodes[root->count++] = (struct ast_decl) { .type           = E_AST_DECL_CONST,
                                                              .value.constant = {
                                                                .ident = cur->data.const_decl.ident,
                                                                .result = cur->data.const_decl.type,
                                                                .value = cur->data.const_decl.value,
                                                              } };

        *state = cur->prev_state;
        free(cur);

        return 0;
    }

    case E_AST_PARSE_EXPR_ROOT:
    {
        glc_log(E_DEBUG, "Cool Expr Root Parsing Bro\n");
        break;
    }
    }

    // Unreachable error code
    return -1;
}

/// Returns number of Tokens parsed, or negative for Error
ptrdiff_t ast_parse_program(struct ast_program *result, struct token *stream)
{
    int                     ret;
    struct ast_parse_state *state;

    if (result == NULL || stream == NULL) { return E_AST_INVALIDINPUT; }

    state  = calloc(1, sizeof(struct ast_parse_state));
    *state = (struct ast_parse_state) {
        .state      = E_AST_PARSE_ROOT,
        .prev_state = NULL,
        .data.root  = (struct ast_program) { .root_nodes =
                                               calloc(PROGRAM_NODECAPACITY, sizeof(struct ast_decl)),
                                             .capacity = PROGRAM_NODECAPACITY,
                                             .count    = 0 },
    };

    while (stream->value != E_TOKEN_EOF)
    {
        memset(token_buffer, 0, sizeof(token_buffer));

        ret = ast_parse_next(&state, stream);
        if (ret < 0) { return ret; }

        stream += 1;
    }

    if (state->prev_state != NULL)
    {
        glc_log(E_ERROR, "Unexpected EOF\n");
        return E_AST_UNEXPECTED;
    }

    *result = state->data.root;
    free(state);

    return 0;
}