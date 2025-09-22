%{
#include "bootstrap/tree.h"

#include "parser.h"
#include "lexer.h"

#include <stdlib.h>
#include <string.h>

void yyerror(YYLTYPE *locp, struct glc_ast_root *root, yyscan_t scanner, const char *s, ...);

struct glc_ast_expr glc_ast_build_binary(int type, struct glc_ast_expr lhs, struct glc_ast_expr rhs);
%}

%defines "parser.h"
%define parse.trace

%define api.pure full
%locations

%code requires {
    #include "bootstrap/tree.h"

    typedef void* yyscan_t;
}
%lex-param   { yyscan_t scanner }
%parse-param { struct glc_ast_root *root }
%parse-param { yyscan_t scanner }

%union {
    struct glc_number number;
    const char *ident;

    struct glc_ast_statement stmt;
    struct glc_ast_type type;
    struct glc_ast_expr expr;

    struct glc_ast_variable variable;
    struct glc_ast_function func;

    struct {
        struct glc_ast_decl_argitem *args;
        size_t                       num_args;
    } args;
    struct glc_ast_decl_argitem arg1;
}

%token TOK_LPAREN "("
%token TOK_RPAREN ")"

%token TOK_LBRACE "{"
%token TOK_RBRACE "}"

%token TOK_LBRACK "["
%token TOK_RBRACK "]"

%token TOK_COLON ":"
%token TOK_SEMICOLON ";"
%token TOK_COMMA ","

%token TOK_PLUS "+"
%token TOK_MINUS "-"

%token TOK_STAR "*"
%token TOK_LSLASH "/"

%token TOK_LCHEV "<"
%token TOK_RCHEV ">"

%token TOK_EQUAL "="
%token TOK_EQEQ "=="
%token TOK_LTEQ "<="
%token TOK_GTEQ ">="

%token TOK_AND "&"
%token TOK_BAR "|"
%token TOK_EXMARK "!"

%token TOK_BOOLAND "&&"
%token TOK_BOOLOR "||"

%token TOK_QMARK "?"
%token TOK_SARROW "->"
%token TOK_BARROW "=>"

%token <number> TOK_NUMBER "number"
%token <ident> TOK_IDENT "ident"

%token TOK_CONST "const"
%token TOK_FUNCT "fn"
%token TOK_LET "let"
%token TOK_IF "if"
%token TOK_EXTERN "extern"

%type <type> type

%type <stmt> root-item

%type <variable> root-variable
%type <variable> variable

%type <func> function
%type <func> function-inner

%type <args> function-args
%type <arg1> arguement

%type <stmt> statement
%type <stmt> body

%type <expr> expr

%left "+" "-"
%left "*" "/"
%left "<"

/* TODO: Implement Deallocators */
%destructor { free((char *) $$); } <ident>
%destructor { $$.num_args = 0; free($$.args); } <args>

%%

input
    : %empty { }
    | input root-item { glc_ast_root_push(root, $2); }
    ;


/* Allowed Root Items:
    - If + Match
    - Const Variable
    - Fn
    - Import (mod + use)
*/
root-item
    : root-variable ";" { $$.type = E_AST_STMT_VARIABLE; $$.v_variable = $1; }
    | function { $$.type = E_AST_STMT_FUNCTION; $$.v_func = $1; }
    ;


root-variable
    : "const" "ident" ":" type "=" expr {
        $$.ident = $2;
        $$.type = $4;
        $$.value = $6;
        $$.flags = E_AST_VAR_CONST;
    }
    ;

variable
    : root-variable { $$ = $1; }
    | "let" "ident" ":" type "=" expr {
        $$.ident = $2;
        $$.type = $4;
        $$.value = $6;
    }
    | "let" "ident" "=" expr {
        $$.ident = $2;
        $$.type.ident = NULL;
        $$.value = $4;
    };

function
    : function-inner "{" body "}" {
        $$ = $1;
        $$.body = calloc(1, sizeof(struct glc_ast_statement));
        $$.body[0] = $3;
        $$.flags = 0;
    }
    | "const" function-inner "{" body "}" {
        $$ = $2;
        $$.body = calloc(1, sizeof(struct glc_ast_statement));
        $$.body[0] = $4;
        $$.flags = E_AST_FUNC_CONST;
    }
    | "extern" function-inner ";" {
        $$ = $2;
        $$.body = NULL;
        $$.flags = E_AST_FUNC_EXTERN;
    }

function-inner
    : "fn" "ident" "(" function-args ")" {
        $$.ident = $2;
        $$.args = $4.args;
        $$.num_args = $4.num_args;
        $$.type.ident = NULL;
    }
    | "fn" "ident" "(" function-args ")" "->" type {
        $$.ident = $2;
        $$.args = $4.args;
        $$.num_args = $4.num_args;
        $$.type = $7;
    }
    ;

function-args
    : %empty { 
        $$.args = NULL; $$.num_args = 0; 
    }
    | arguement {
        $$.num_args = 0;
        $$.args = calloc(1, sizeof(struct glc_ast_decl_argitem));
        $$.args[$$.num_args++] = $1;
    }
    | function-args "," arguement {
        $$ = $1;
        $$.args = realloc($$.args, ($$.num_args + 1) * sizeof(struct glc_ast_decl_argitem));
        $$.args[$$.num_args++] = $3;
        /* FIXME: Error Handling on `realloc` failure */
    }
    ;

arguement
    : "ident" ":" type {
        $$.ident = $1;
        $$.type = $3;
    }
    ;

body
    : %empty { $$.type = E_AST_STMT_BODY; $$.v_body.num_stmts = 0; $$.v_body.stmts = NULL; }
    | statement {
        $$.type = E_AST_STMT_BODY;

        $$.v_body.num_stmts = 0;
        $$.v_body.stmts = calloc(1, sizeof(struct glc_ast_statement));
        $$.v_body.stmts[$$.v_body.num_stmts++] = $1;
    }
    | body ";" { $$ = $1; }
    | body ";" statement {
        $$ = $1;
        /* FIXME: Error Handling on `realloc` failure */
        $$.v_body.stmts = realloc($$.v_body.stmts, ($$.v_body.num_stmts + 1) * sizeof(struct glc_ast_statement));
        $$.v_body.stmts[$$.v_body.num_stmts++] = $1;
    }
    ;

statement
    : expr { $$.type = E_AST_STMT_EXPR; $$.v_expr = $1; }
    | variable { $$.type = E_AST_STMT_VARIABLE; $$.v_variable = $1; }
    ;

expr
    : expr "+" expr { $$ = glc_ast_build_binary(E_AST_EXPR_ADD, $1, $3); }
    | expr "-" expr { $$ = glc_ast_build_binary(E_AST_EXPR_SUB, $1, $3); }
    | expr "*" expr { $$ = glc_ast_build_binary(E_AST_EXPR_MUL, $1, $3); }
    | expr "<" expr { $$ = glc_ast_build_binary(E_AST_EXPR_LT, $1, $3); }
    | "number" { $$.type = E_AST_EXPR_NUM; $$.v_num.val = $1; }
    | "number" "ident" { 
        $$.type = E_AST_EXPR_NUM; 
        $$.v_num.val = $1; 
        $$.v_num.type = calloc(1, sizeof(struct glc_ast_type));
        $$.v_num.type[0].ident = $2;
    }
    | "ident" { $$.type = E_AST_EXPR_VAR; $$.v_var = $1; }
    | "(" expr ")" { $$ = $2; }
    ;

type
    : "ident" { $$.ident = $1; }
    ;

%%

struct glc_ast_expr glc_ast_build_binary(int type, struct glc_ast_expr lhs, struct glc_ast_expr rhs)
{
    struct glc_ast_expr val;
    val.type = type;

    val.v_children = calloc(2, sizeof(struct glc_ast_expr));
    val.v_children[0] = lhs;
    val.v_children[1] = rhs;

    return val;
}
