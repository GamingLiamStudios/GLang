#include "tree.h"

#include <stdlib.h>
#include <string.h>

#include "token.h"
#include "log.h"

#if defined(__GNUC__)    // GCC, Clang, ICC

#define unreachable() (__builtin_unreachable())

#elif defined(_MSC_VER)    // MSVC

#define unreachable() (__assume(false))

#else

#error "No Unreachable Defintion exists"

#endif

#define PROGRAM_NODECAPACITY 256
#define PROGRAM_NODEGROWTH   2

int ast_expression_from_tokens(
  struct ast_expression *parsed,
  struct ast_expression *previous,
  struct token         **token_stream);

void ast_program_free(struct ast_program *program)
{
    if (program == NULL) { return; }

    for (size_t i = 0; i < program->num_nodes; i++) { ast_node_free(program->nodes + i); }

    free(program->nodes);
    program->nodes         = NULL;
    program->num_nodes     = 0;
    program->node_capacity = 0;
}
void ast_node_free(struct ast_node *node)
{
    if (node == NULL) { return; }

    ast_type_free(&node->node_type);
    free((char *) node->ident);
    node->ident = NULL;

    switch (node->type)
    {
    case E_AST_NODE_CONSTANT: ast_expression_free(&node->data.value); break;
    case E_AST_NODE_EXTERNAL:
    case E_AST_NODE_FUNCTION:
    {
        // Blind safety in my own code
        if (node->data.function.num_params > 0)
        {
            for (size_t i = 0; i < node->data.function.num_params; i++)
            {
                free((char *) node->data.function.param_idents[i]);
                ast_type_free(node->data.function.param_types + i);
            }
            free(node->data.function.param_idents);
            free(node->data.function.param_types);
        }
        node->data.function.num_params   = 0;
        node->data.function.param_idents = NULL;
        node->data.function.param_types  = NULL;

        ast_expression_free(node->data.function.body);
        node->data.function.body = NULL;
        break;
    }
    }
}
void ast_statement_free(struct ast_statement *statement)
{
    // TODO
}
void ast_expression_free(struct ast_expression *expression)
{
    if (expression == NULL) { return; }
    switch (expression->type)
    {
    case E_AST_EXPR_VARIABLE:
        free((char *) expression->value.variable_ident);
        expression->value.variable_ident = NULL;
        break;

    case E_AST_EXPR_CONSTANT:
        if (expression->value.constant.type == E_AST_CONST_STRING)
        {
            free((char *) expression->value.constant.value.string);
            expression->value.constant.value.string = NULL;
        }
        break;

    case E_AST_EXPR_BLOCK:
        for (size_t i = 0; i < expression->value.block.num_statements; i++)
            ast_statement_free(expression->value.block.statements + i);
        free(expression->value.block.statements);
        expression->value.block.statements     = NULL;
        expression->value.block.num_statements = 0;

        ast_expression_free(expression->value.block.expression);
        free(expression->value.block.expression);
        expression->value.block.expression = NULL;
        break;

    case E_AST_EXPR_SCOPE:
    case E_AST_EXPR_LOOP:
        ast_expression_free(expression->value.expr);
        free(expression->value.expr);
        expression->value.expr = NULL;
        break;

    case E_AST_EXPR_CALL:
        for (size_t i = 0; i < expression->value.call.num_expressions; i++)
            ast_expression_free(expression->value.call.expressions[i]);
        free(expression->value.call.expressions);
        expression->value.call.expressions     = NULL;
        expression->value.call.num_expressions = 0;
        break;

    case E_AST_EXPR_LET:
        free((char *) expression->value.let.ident);
        expression->value.let.ident = NULL;

        ast_type_free(expression->value.let.type);
        free(expression->value.let.type);
        expression->value.let.type = NULL;

        ast_expression_free(expression->value.let.expression);
        free(expression->value.let.expression);
        expression->value.let.expression = NULL;
        break;

    case E_AST_EXPR_CAST:
        ast_expression_free(expression->value.cast.expression);
        free(expression->value.cast.expression);
        expression->value.cast.expression = NULL;

        ast_type_free(&expression->value.cast.target);
        break;

    case E_AST_EXPR_OPER:
        ast_expression_free(expression->value.oper.lhs);
        free(expression->value.oper.lhs);
        expression->value.oper.lhs = NULL;

        ast_expression_free(expression->value.oper.rhs);
        free(expression->value.oper.rhs);
        expression->value.oper.rhs = NULL;
        break;

    case E_AST_EXPR_IF:
    case E_AST_EXPR_WHILE:
        ast_expression_free(expression->value.branch.condition);
        free(expression->value.branch.condition);
        expression->value.branch.condition = NULL;

        ast_expression_free(expression->value.branch.if_true);
        free(expression->value.branch.if_true);
        expression->value.branch.if_true = NULL;

        ast_expression_free(expression->value.branch.if_false);
        free(expression->value.branch.if_false);
        expression->value.branch.if_false = NULL;
        break;
    }
}
void ast_type_free(struct ast_type *type)
{
    if (type == NULL) { return; }

    free((char *) type->type_name);
    type->type_name = NULL;
}

int ast_program_grow(struct ast_program *program)
{
    if (program == NULL) { return -1; }

    program->node_capacity *= PROGRAM_NODEGROWTH;
    void *new_ptr = realloc(program->nodes, sizeof(struct ast_node) * program->node_capacity);
    if (new_ptr == NULL)
    {
        ast_program_free(program);
        return E_AST_MEMORYERROR;
    }

    program->nodes = new_ptr;
    return 0;
}

int ast_type_from_tokens(struct ast_type *parsed, struct token **token_stream)
{
    if (parsed == NULL || token_stream == NULL || (*token_stream) == NULL) { return -1; }

    // Pull next token
    struct token ident = *((*token_stream)++);
    if (ident.value != E_TOKEN_IDENTIFIER)
    {
        glc_log(
          E_ERROR,
          "Expected Identifier at position %d:%d\n",
          ident.debug_info.line,
          ident.debug_info.column);
        return E_AST_UNEXPECTED;
    }

    size_t len        = strlen(ident.data.string) + 1;
    parsed->type_name = calloc(len, sizeof(char));
    memcpy((char *) parsed->type_name, ident.data.string, len);

    return 0;
}

int ast_program_from_tokens(struct ast_program *program, struct token_stream *token_stream)
{
    struct token *cursor = token_stream->tokens;
    struct token  next;
    int           error;

    if (program == NULL) { return -1; }

    // Allocate program (if not already)
    program->num_nodes = 0;
    if (program->node_capacity == 0)
    {
        program->node_capacity = PROGRAM_NODECAPACITY / PROGRAM_NODEGROWTH;
        int error              = ast_program_grow(program);
        if (error < 0) { return error; }
    }

    while ((next = *(cursor++)).value != E_TOKEN_EOF)
    {
        // Must be a valid Program subtype
        // TODO: Visibility modifiers

        switch (next.value)
        {
        case E_TOKEN_EOF: unreachable();
        case E_TOKEN_CONST:
        {
            struct ast_node *current = program->nodes + program->num_nodes;
            current->type            = E_AST_NODE_CONSTANT;

            next = *(cursor++);
            if (next.value != E_TOKEN_IDENTIFIER)
            {
                glc_log(
                  E_ERROR,
                  "Expected Identifier at position %d:%d\n",
                  next.debug_info.line,
                  next.debug_info.column);

                ast_program_free(program);
                return E_AST_UNEXPECTED;
            }

            size_t len     = strlen(next.data.string);
            current->ident = calloc(sizeof(char), len + 1);
            memcpy((char *) current->ident, next.data.string, sizeof(char) * len);

            next = *(cursor++);
            if (next.value != ':')
            {
                glc_log(
                  E_ERROR,
                  "Expected ':' at position %d:%d\n",
                  next.debug_info.line,
                  next.debug_info.column);

                ast_program_free(program);
                return E_AST_UNEXPECTED;
            }

            error = ast_type_from_tokens(&current->node_type, &cursor);
            if (error < 0) { return error; }

            next = *(cursor++);
            if (next.value != '=')
            {
                glc_log(
                  E_ERROR,
                  "Expected '=' at position %d:%d (got '%d')\n",
                  next.debug_info.line,
                  next.debug_info.column,
                  next.value);

                ast_program_free(program);
                return E_AST_UNEXPECTED;
            }

            break;
        }

        case E_TOKEN_FUNCTION:
            // TODO
            break;

        case E_TOKEN_EXTERN:
            // TODO
            break;

        default:
            glc_log(
              E_ERROR,
              "Unexpected Token at position %d:%d (got %d)\n",
              next.debug_info.line,
              next.debug_info.column,
              next.value);

            // TODO: Better debug logging; What is bad token?
            ast_program_free(program);
            return E_AST_UNEXPECTED;
        }

        // Grow program if needed
        if (program->node_capacity == ++program->num_nodes)
        {
            int error = ast_program_grow(program);
            if (error < 0) { return error; }
        }
    }

    return 0;
}