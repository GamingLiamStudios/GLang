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

// Notes;
// Could ScopeExpr and CallExpr be too similar?

// Quick little syntax cheatsheat;
// Program ::= (Function | External | Constant | Struct | Enum)*
// Function ::= "fn" IDENTIFIER "(" (NAME ":"" TYPE)* ")" "->" TYPE BlockExpr
// External ::= "extern" "fn" IDENTIFIER "(" (NAME ":"" TYPE)* ")" "->" TYPE ";"
// Constant ::= "const" IDENTIFIER ":" TYPE "=" Expression ";"
// TODO: Struct Syntax
// TODO: Enum Syntax

// ExprStmt ::= Expression ";"
// RetStmt ::= "return" Expression? ";"
// BreakStmt ::= "break" Expression? ";"
// ContinueStmt ::= "continue" ";"

// BlockExpr ::= "{" Statement* Expression "}"
// ScopeExpr ::= "(" Expression ")"

// LetExpr ::= "let" IDENTIFIER [":" TYPE] "=" Expression
// IfExpr ::= "if" Expression BlockExpr ["else" BlockExpr]
// BinaryExpr ::= Expression OP Expression
// UnaryExpr ::= OP Expression | Expression OP
// CallExpr ::= Expression "(" Expression* ")"
// LoopExpr ::= "loop" BlockExpr
// WhileExpr ::= "while" Expression BlockExpr
// CastExpr ::= Expression as TYPE
// TODO: Match Syntax

#define PROGRAM_NODECAPACITY 256
#define PROGRAM_NODEGROWTH   2

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

    case E_AST_EXPR_UNARY:
    case E_AST_EXPR_BINARY:
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

enum ast_expression_result
{
    E_AST_PUSH,
    E_AST_POP,
};

// Returns `enum ast_expression_result`, or Negative value on error
int ast_expression_from_tokens(
  struct ast_expression *parsed,
  struct ast_expression *previous,
  struct token         **token_stream)
{
    int error;

    if (token_stream == NULL || *token_stream == NULL) { return -1; }
    if (parsed == NULL) { return -1; }

    struct token next = *((*token_stream)++);
    switch (next.value)
    {
    case E_TOKEN_EOF: glc_log(E_ERROR, "Unexpected EOF\n"); return E_AST_UNEXPECTED;
    case E_TOKEN_CONST:
    case E_TOKEN_FUNCTION:
    case E_TOKEN_EXTERN:
    case E_TOKEN_STRUCT:
    case E_TOKEN_ENUM:
    case E_TOKEN_PUBLIC:
    case E_TOKEN_TRAIT:
    case E_TOKEN_ELSE:

    // TODO: Support `impl trait`
    case E_TOKEN_IMPLEMENTS:

    // TODO: Support match-case
    case E_TOKEN_MATCH:

    // TODO: Handle control-flow
    case E_TOKEN_BREAK:
    case E_TOKEN_CONTINUE:
        glc_log(
          E_ERROR,
          "Unexpected Keyword at position %d:%d\n",
          next.debug_info.line,
          next.debug_info.column);
        return E_AST_UNEXPECTED;

    case E_TOKEN_STRING:
    {
        parsed->type                = E_AST_EXPR_CONSTANT;
        parsed->value.constant.type = E_AST_CONST_STRING;

        size_t len                          = strlen(next.data.string) + 1;
        parsed->value.constant.value.string = calloc(sizeof(char), len);
        memcpy((char *) parsed->value.constant.value.string, next.data.string, len);

        return E_AST_PUSH;
    }
    case E_TOKEN_INTEGER:
    {
        parsed->type                         = E_AST_EXPR_CONSTANT;
        parsed->value.constant.type          = E_AST_CONST_INTEGER;
        parsed->value.constant.value.integer = next.data.integer;

        return E_AST_PUSH;
    }

    case E_TOKEN_IDENTIFIER:
    {
        parsed->type = E_AST_EXPR_VARIABLE;

        size_t len                   = strlen(next.data.string);
        parsed->value.variable_ident = calloc(sizeof(char), len + 1);
        memcpy((char *) parsed->value.variable_ident, next.data.string, len);

        return E_AST_PUSH;
    }

    case E_TOKEN_AS:
    {
        // Parse next expression
        struct ast_type target;
        error = ast_type_from_tokens(&target, token_stream);
        if (error < 0) { return error; }

        if (previous == NULL)
        {
            glc_log(
              E_ERROR,
              "Unexpected `as` at position %d:%d\n",
              next.debug_info.line,
              next.debug_info.column);

            ast_type_free(&target);
            return E_AST_UNEXPECTED;
        }

        parsed->type                  = E_AST_EXPR_CAST;
        parsed->value.cast.expression = calloc(sizeof(struct ast_expression), 1);
        memcpy(parsed->value.cast.expression, previous, sizeof(struct ast_expression));

        memcpy(&parsed->value.cast.target, &target, sizeof(struct ast_type));
        // Can leave `target` dangling due to reuse of internal data - essentially moving it

        return E_AST_POP;
    }

    case E_TOKEN_LOOP:
    {
        // Peek next expression
        struct token         *cursor = *token_stream;
        struct ast_expression block_expr;
        error = ast_expression_from_tokens(&block_expr, NULL, token_stream);
        if (error < 0) { return error; }

        if (block_expr.type != E_AST_EXPR_BLOCK)
        {
            glc_log(
              E_ERROR,
              "Expected '{' at %d:%d\n",
              cursor->debug_info.line,
              cursor->debug_info.column);

            ast_expression_free(&block_expr);
            return E_AST_UNEXPECTED;
        }

        parsed->type       = E_AST_EXPR_LOOP;
        parsed->value.expr = calloc(sizeof(struct ast_expression), 1);
        memcpy(parsed->value.expr, &block_expr, sizeof(struct ast_expression));

        return E_AST_PUSH;
    }

    case E_TOKEN_WHILE:
    {
        // Pop next expression
        struct ast_expression cond_expr;
        error = ast_expression_from_tokens(&cond_expr, NULL, token_stream);
        if (error < 0) { return error; }

        struct token         *cursor = *token_stream;
        struct ast_expression block_expr;
        error = ast_expression_from_tokens(&block_expr, NULL, &cursor);
        if (error < 0)
        {
            ast_expression_free(&cond_expr);
            return error;
        }

        if (block_expr.type != E_AST_EXPR_BLOCK)
        {
            glc_log(
              E_ERROR,
              "Expected '{' at %d:%d\n",
              cursor->debug_info.line,
              cursor->debug_info.column);

            ast_expression_free(&block_expr);
            ast_expression_free(&cond_expr);
            return E_AST_UNEXPECTED;
        }

        parsed->type = E_AST_EXPR_WHILE;

        parsed->value.branch.condition = calloc(sizeof(struct ast_expression), 1);
        memcpy(parsed->value.branch.condition, &cond_expr, sizeof(struct ast_expression));

        parsed->value.branch.if_true = calloc(sizeof(struct ast_expression), 1);
        memcpy(parsed->value.branch.if_true, &block_expr, sizeof(struct ast_expression));

        return E_AST_PUSH;
    }

    case E_TOKEN_IF:
    {
        // Pop next expression
        struct ast_expression cond_expr;
        error = ast_expression_from_tokens(&cond_expr, NULL, token_stream);
        if (error < 0) { return error; }

        struct token         *cursor = *token_stream;
        struct ast_expression true_expr;
        error = ast_expression_from_tokens(&true_expr, NULL, &cursor);
        if (error < 0)
        {
            ast_expression_free(&cond_expr);
            return error;
        }

        if (true_expr.type != E_AST_EXPR_BLOCK)
        {
            glc_log(
              E_ERROR,
              "Expected '{' at %d:%d\n",
              cursor->debug_info.line,
              cursor->debug_info.column);

            ast_expression_free(&true_expr);
            ast_expression_free(&cond_expr);
            return E_AST_UNEXPECTED;
        }

        parsed->type                  = E_AST_EXPR_IF;
        parsed->value.branch.if_false = NULL;

        // Peek next token
        if ((*token_stream)->value == E_TOKEN_ELSE)
        {
            *token_stream += 1;

            struct token         *cursor = *token_stream;
            struct ast_expression false_expr;
            error = ast_expression_from_tokens(&false_expr, NULL, &cursor);
            if (error < 0)
            {
                ast_expression_free(&true_expr);
                ast_expression_free(&cond_expr);
                return error;
            }

            if (!(false_expr.type == E_AST_EXPR_BLOCK || false_expr.type == E_AST_EXPR_IF))
            {
                glc_log(
                  E_ERROR,
                  "Expected '{' or chained if at %d:%d\n",
                  cursor->debug_info.line,
                  cursor->debug_info.column);

                ast_expression_free(&false_expr);
                ast_expression_free(&true_expr);
                ast_expression_free(&cond_expr);
                return E_AST_UNEXPECTED;
            }

            parsed->value.branch.if_false = calloc(sizeof(struct ast_expression), 1);
            memcpy(parsed->value.branch.if_false, &false_expr, sizeof(struct ast_expression));
        }

        parsed->value.branch.condition = calloc(sizeof(struct ast_expression), 1);
        memcpy(parsed->value.branch.condition, &cond_expr, sizeof(struct ast_expression));

        parsed->value.branch.if_true = calloc(sizeof(struct ast_expression), 1);
        memcpy(parsed->value.branch.if_true, &true_expr, sizeof(struct ast_expression));

        return E_AST_PUSH;
    }

    case E_TOKEN_LET:
    {
        parsed->type = E_AST_EXPR_LET;

        // Pop identifier
        struct token ident = *((*token_stream)++);
        if (ident.value != E_TOKEN_IDENTIFIER)
        {
            glc_log(
              E_ERROR,
              "Expected Identifier at %d:%d\n",
              ident.debug_info.line,
              ident.debug_info.column);

            return E_AST_UNEXPECTED;
        }

        parsed->value.let.type = NULL;
        if ((*token_stream)->value == ':')
        {
            *token_stream += 1;

            // Pop type
            struct ast_type type;
            error = ast_type_from_tokens(&type, token_stream);
            if (error < 0) { return error; }

            parsed->value.let.type = calloc(sizeof(struct ast_type), 1);
            memcpy(parsed->value.let.type, &type, sizeof(struct ast_type));
        }

        struct token assign = *((*token_stream)++);
        if (assign.value != '=')
        {
            glc_log(
              E_ERROR,
              "Expected '=' at %d:%d\n",
              ident.debug_info.line,
              ident.debug_info.column);

            ast_type_free(parsed->value.let.type);
            return E_AST_UNEXPECTED;
        }

        struct ast_expression value_expr;
        error = ast_expression_from_tokens(&value_expr, NULL, token_stream);
        if (error < 0)
        {
            ast_type_free(parsed->value.let.type);
            return error;
        }

        size_t len              = strlen(ident.data.string);
        parsed->value.let.ident = calloc(sizeof(char), len + 1);
        memcpy((char *) parsed->value.let.ident, ident.data.string, len);

        parsed->value.let.expression = calloc(sizeof(struct ast_expression), 1);
        memcpy(parsed->value.let.expression, &value_expr, sizeof(struct ast_expression));

        return E_AST_PUSH;
    }

    case '{':
    {
        // TODO
        return E_AST_PUSH;
    }

    case '(':
    {
        struct ast_expression expr;
        int                   result = ast_expression_from_tokens(&expr, NULL, token_stream);
        if (result < 0)
        {
            ast_expression_free(&expr);
            return result;
        }

        while ((*token_stream)->value != ')')
        {
            struct token *prev_cursor = *token_stream;

            struct ast_expression next_expr;
            int result = ast_expression_from_tokens(&next_expr, &expr, token_stream);
            if (result < 0)
            {
                ast_expression_free(&expr);
                return result;
            }

            switch ((enum ast_expression_result) result)
            {
            case E_AST_POP: memcpy(&expr, &next_expr, sizeof(struct ast_expression)); break;
            case E_AST_PUSH:
                glc_log(
                  E_ERROR,
                  "Unexpected Expression at position %d:%d\n",
                  prev_cursor->debug_info.line,
                  prev_cursor->debug_info.column);

                ast_expression_free(&expr);
                ast_expression_free(&next_expr);
                return E_AST_UNEXPECTED;
            }
        }

        parsed->type       = E_AST_EXPR_SCOPE;
        parsed->value.expr = calloc(sizeof(struct ast_expression), 1);
        memcpy(parsed->value.expr, &expr, sizeof(struct ast_expression));

        return E_AST_PUSH;
    }

    case '.':
    {
        // Parse next expression
        struct ast_expression next_expr;
        error = ast_expression_from_tokens(&next_expr, NULL, token_stream);
        if (error < 0) { return error; }

        // Floating-point numbers
        if (
          previous != NULL && previous->type == E_AST_EXPR_CONSTANT &&
          previous->value.constant.type == E_AST_CONST_INTEGER &&
          next_expr.type == E_AST_EXPR_CONSTANT &&
          next_expr.value.constant.type == E_AST_CONST_INTEGER)
        {
            // Floating point value
            unsigned long integer = parsed->value.constant.value.integer;

            parsed->value.constant.type                      = E_AST_CONST_FLOATING;
            parsed->value.constant.value.floating.integer    = integer;
            parsed->value.constant.value.floating.fractional = ((*token_stream)++)->data.integer;

            return E_AST_POP;
        }

        // TODO: Member Functions
        glc_log(
          E_ERROR,
          "Member Functions/Variables not implemented! (used at %d:%d)\n",
          next.debug_info.line,
          next.debug_info.column);

        ast_expression_free(&next_expr);
        return E_AST_UNEXPECTED;
    }

    // Ambiguious Operators (depends on previous)
    case '-':
    case '*':
    case '&':
    case '!':
    {
        // TODO
        return E_AST_PUSH;
    }

    // Ambiguious Operators (depends on next)
    case '>':
    case '=':
    case '<':
    {
        // TODO
        return E_AST_PUSH;
    }

    // Binary Operators
    case '+':
    case '^':
    case '/':
    case '|':
    {
        // TODO
        return E_AST_POP;
    }

    // Unary Operators
    case '~':
    {
        // TODO
        return E_AST_PUSH;
    }
    }

    glc_log(
      E_ERROR,
      "Unexpected '%c' at %d:%d\n",
      next.value,
      next.debug_info.column,
      next.debug_info.line);
    return E_AST_UNEXPECTED;
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

            struct ast_expression expr;
            int                   result = ast_expression_from_tokens(&expr, NULL, &cursor);
            if (result < 0)
            {
                ast_expression_free(&expr);
                return result;
            }

            while (cursor->value != ';')
            {
                struct token *prev_cursor = cursor;

                struct ast_expression next_expr;
                int result = ast_expression_from_tokens(&next_expr, &expr, &cursor);
                if (result < 0)
                {
                    ast_expression_free(&expr);
                    return result;
                }

                printf("%d %d\n", prev_cursor->value, cursor->value);

                switch ((enum ast_expression_result) result)
                {
                case E_AST_POP: memcpy(&expr, &next_expr, sizeof(struct ast_expression)); break;
                case E_AST_PUSH:
                    glc_log(
                      E_ERROR,
                      "Unexpected Expression at position %d:%d\n",
                      prev_cursor->debug_info.line,
                      prev_cursor->debug_info.column);

                    ast_expression_free(&expr);
                    ast_expression_free(&next_expr);
                    return E_AST_UNEXPECTED;
                }
            }

            current->data.value = expr;

            next = *(cursor++);
            if (next.value != ';')
            {
                glc_log(
                  E_ERROR,
                  "Expected ';' at position %d:%d\n",
                  next.debug_info.line,
                  next.debug_info.column);

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