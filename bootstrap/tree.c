#include "tree.h"

// Notes;
// Could ScopeExpr and CallExpr be too similar?

// Quick little syntax cheatsheat;
// Program ::= (Function | External | Constant | Struct | Enum)*
// Function ::= "fn" IDENTIFIER "(" (NAME ":"" TYPE)* ")" "->" TYPE BlockExpr
// External ::= "extern" "fn" IDENTIFIER "(" (NAME ":"" TYPE)* ")" "->" TYPE ";"
// Constant ::= "const" IDENTIFIER ":" TYPE "=" Expression ";"
// TODO: Struct
// TODO: Enum

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
// TODO: Match

void ast_program_free(struct ast_program *program)
{
    // TODO
}
void ast_node_free(struct ast_node *node)
{
    // TODO
}
void ast_statement_free(struct ast_statement *statement)
{
    // TODO
}
void ast_expression_free(struct ast_expression *expression)
{
    // TODO
}
void ast_type_free(struct ast_type *type)
{
    // TODO
}

int ast_program_from_tokens(struct ast_program *program, struct token_stream *token_stream)
{
    // TODO
    return -1;
}