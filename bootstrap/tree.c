#include "tree.h"

#include <stdlib.h>
#include <string.h>

#define AST_TREE_CAPACITY     16
#define AST_TREE_GROWTHFACTOR 2

#define AST_NODE_BACKTRACK_BUFLEN 2048

// TODO: this is awful
void ast_node_free(struct ast_node *node)
{
    if (node == NULL) { return; }

    switch (node->type)
    {
    case E_AST_FUNCTIONDEF:
        for (size_t j = 0; j < node->value.function_def.num_body_nodes; j++)
            ast_node_free(node->value.function_def.body_nodes + j);
        for (size_t j = 0; j < node->value.function_def.num_parameters; j++)
        {
            struct ast_identifier ident = node->value.function_def.parameters[j];
            free((char *) ident.name);
        }
        free(node->value.function_def.body_nodes);
        free(node->value.function_def.parameters);
        break;

    case E_AST_ASSIGN:
        free((char *) node->value.assign.identifier.name);
        ast_node_free(node->value.assign.value);
        free(node->value.assign.value);
        break;

    case E_AST_CONSTANT:
        if (node->value.constant.type == E_AST_EXPR_CONSTANT_STRING)
            free((char *) node->value.constant.value.string);
        break;
    case E_AST_IDENTIFIER: free((char *) node->value.assign.identifier.name); break;
    case E_AST_FUNCTIONCALL:
        free((char *) node->value.function_call.identifier.name);
        for (size_t i = 0; i < node->value.function_call.num_arguments; i++)
            ast_node_free(node->value.function_call.arguments + i);
        free(node->value.function_call.arguments);
        break;
    // Unary Operators
    case E_AST_INVERT:
        ast_node_free(node->value.unary);
        free(node->value.unary);
        break;

    // Binary Operators
    case E_AST_ADD:
    case E_AST_SUB:
    case E_AST_MUL:
    case E_AST_DIV:
    case E_AST_AND:
    case E_AST_OR:
        ast_node_free(node->value.binary.lhs);
        ast_node_free(node->value.binary.rhs);
        free(node->value.binary.lhs);
        free(node->value.binary.rhs);
        break;

    case E_AST_BRANCH:
        ast_node_free(node->value.branch.condition);
        ast_node_free(node->value.branch.if_true);
        ast_node_free(node->value.branch.if_false);
        free(node->value.branch.condition);
        free(node->value.branch.if_true);
        free(node->value.branch.if_false);
        break;

    case E_AST_LOOP:
        ast_node_free(node->value.loop.condition);
        ast_node_free(node->value.loop.exec);
        free(node->value.loop.condition);
        free(node->value.loop.exec);
        break;

    case E_AST_COND:
        ast_node_free(node->value.cond.lhs);
        ast_node_free(node->value.cond.rhs);
        free(node->value.cond.lhs);
        free(node->value.cond.rhs);
        break;
    }
}

void ast_list_free(struct ast_list *nodes)
{
    if (nodes == NULL) { return; }
    for (size_t i = 0; i < nodes->num_nodes; i++) ast_node_free(nodes->nodes);

    free(nodes->nodes);
    nodes->nodes     = NULL;
    nodes->num_nodes = 0;
    nodes->capacity  = 0;
}

int ast_parse_tokens(struct ast_list *nodes, struct token_stream *tokens)
{
    nodes->nodes     = calloc(AST_TREE_CAPACITY, sizeof(struct ast_node));
    nodes->capacity  = AST_TREE_CAPACITY;
    nodes->num_nodes = 0;

    return 0;
}