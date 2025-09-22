#include "tree.h"
#include "log.h"

void glc_ast_root_push(struct glc_ast_root *root, struct glc_ast_statement new)
{
    struct glc_ast_statement *new_ptr;

    new_ptr = realloc(root->decls, sizeof(struct glc_ast_statement) * (root->num_decls + 1));
    if (new_ptr == NULL)
    {
        // TODO: Throw error
        return;
    }

    root->decls                    = new_ptr;
    root->decls[root->num_decls++] = new;
}