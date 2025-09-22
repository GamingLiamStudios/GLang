%{
#include "parser.h"

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#define YY_USER_ACTION \
    yylloc->first_line = yylloc->last_line; \
    yylloc->first_column = yylloc->last_column; \
    for(int i = 0; yytext[i] != '\0'; i++) { \
        if(yytext[i] == '\n') { \
            yylloc->last_line++; \
            yylloc->last_column = 0; \
        } \
        else { \
            yylloc->last_column++; \
        } \
    }

void yyerror(YYLTYPE *locp, struct glc_ast_root *root, yyscan_t scanner, const char *s, ...);
%}

%option never-interactive warn nodefault noyywrap yylineno
%option bison-bridge bison-locations nounistd reentrant

/* Definitions */

DIGIT [0-9]
ID [a-zA-Z][a-zA-Z0-9_]*

%x C_COMMENT

%%

[ \r\n\t]* { /* Skip emptiness */ }

{DIGIT}+\.{DIGIT}+ { 
    sscanf(yytext, "%lu.%lu", &yylval->number.integer, &yylval->number.fractional); 
    return TOK_NUMBER; 
}
{DIGIT}+ { 
    // TODO: Base-2/8/16 numerals
    // TODO: Long-integer seperators (ie. 15'000'000)
    sscanf(yytext, "%lu", &yylval->number.integer); 
    return TOK_NUMBER;
}

"//".*\n        {  }
"/*"            { BEGIN(C_COMMENT); }
<C_COMMENT>"*/" { BEGIN(INITIAL); }
<C_COMMENT>\n   { yylineno++; }
<C_COMMENT>.    { }

"(" { return TOK_LPAREN; }
")" { return TOK_RPAREN; }

"{" { return TOK_LBRACE; }
"}" { return TOK_RBRACE; }

"[" { return TOK_LBRACK; }
"]" { return TOK_RBRACK; }

"<" { return TOK_LCHEV; }
">" { return TOK_RCHEV; }

"!" { return TOK_EXMARK; }
"?" { return TOK_QMARK; }

"=" { return TOK_EQUAL; }
"==" { return TOK_EQEQ; }
">=" { return TOK_GTEQ; }
"<=" { return TOK_LTEQ; }

"&" { return TOK_AND; }
"|" { return TOK_BAR; }
"&&" { return TOK_BOOLAND; }
"||" { return TOK_BOOLOR; }

":" { return TOK_COLON; }
";" { return TOK_SEMICOLON; }
"," { return TOK_COMMA; }

"+" { return TOK_PLUS; } 
"-" { return TOK_MINUS; }
"*" { return TOK_STAR; }
"/" { return TOK_LSLASH; }

"->" { return TOK_SARROW; }

"const" { return TOK_CONST; }
"fn" { return TOK_FUNCT; }
"let" { return TOK_LET; }
"if" { return TOK_IF; }
"extern" { return TOK_EXTERN; }

{ID} { 
    /* Must be last, or will interfere with lexing keywords */
    yylval->ident = calloc(strlen(yytext) + 1, sizeof(char));
    strcpy((char *) yylval->ident, yytext);
    return TOK_IDENT;
}

. { return YYerror; }

%%

/* User Code */
void yyerror(YYLTYPE *locp, struct glc_ast_root *root, yyscan_t scanner, const char *s, ...) {
    va_list args;
    va_start(args, s);

    fprintf(stderr,
        "Error at %d:%d ",
        locp->first_line, locp->first_column);

    vfprintf(stderr, s, args);
    fprintf(stderr, "\n");

    va_end(args);
}