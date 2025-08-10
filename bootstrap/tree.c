#include "tree.h"

#include <stdlib.h>
#include <string.h>

#include "token.h"
#include "log.h"

void ast_type_free(struct ast_type *type)
{
    if (type == NULL) { return; }
    free((char *) type->typename);
    type->typename = NULL;
}

void ast_parm_free(struct ast_param *param);
void ast_enum_value_free(struct ast_enum_value *value);
void ast_argument_free(struct ast_argument *arg);
void ast_expression_free(struct ast_expression *expr)
{
    // TODO
}
void ast_statement_free(struct ast_statement *statement);
void ast_decl_free(struct ast_decl *decl);
void ast_program_free(struct ast_program *program)
{
    // TODO
}