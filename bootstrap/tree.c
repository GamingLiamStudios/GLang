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

int ast_type_free(struct ast_type *type);
int ast_parm_free(struct ast_param *param);
int ast_enum_value_free(struct ast_enum_value *value);
int ast_argument_free(struct ast_argument *arg);
int ast_expression_free(struct ast_expression *expr);
int ast_statement_free(struct ast_statement *statement);
int ast_decl_free(struct ast_decl *decl);
int ast_program_free(struct ast_program *progrma);

int ast_program_from_tokens(struct ast_program *program, struct token_stream *token_stream);