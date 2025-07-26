#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>

#include "log.h"
#include "ansi.h"
#include "token.h"
#include "tree.h"

struct CompilerOpts
{
    const char *input_path;
    const char *output_path;
    int         verbosity;
};

struct CompilerOpts opts = {
    .verbosity   = E_INFO,
    .output_path = NULL,
    .input_path  = NULL,
};

void glc_log(enum LogLevel level, const char *restrict format, ...)
{
    if (level > opts.verbosity) return;

    va_list args;
    va_start(args, format);

    switch (level)
    {
    case E_DEBUG:
    {
        fprintf(
          stdout,
          ANSI_FMT_BEGIN ANSI_FMT_COLOR_FG ANSI_FMT_COLOR_MAGENTA ANSI_FMT_END
          "[DEBUG] " ANSI_FMT_RESET);
        break;
    }
    case E_INFO:
    {
        fprintf(
          stdout,
          ANSI_FMT_BEGIN ANSI_FMT_COLOR_FG ANSI_FMT_COLOR_GREEN ANSI_FMT_END
          "[INFO] " ANSI_FMT_RESET);
        break;
    }
    case E_WARN:
    {
        fprintf(
          stdout,
          ANSI_FMT_BEGIN ANSI_FMT_COLOR_FG ANSI_FMT_COLOR_YELLOW ANSI_FMT_END
          "[WARN] " ANSI_FMT_RESET);
        break;
    }
    case E_ERROR:
    {
        fprintf(
          stderr,
          ANSI_FMT_BEGIN ANSI_FMT_COLOR_FG ANSI_FMT_COLOR_RED ANSI_FMT_END
          "[ERROR] " ANSI_FMT_RESET);
        vfprintf(stderr, format, args);
        va_end(args);
        return;
    }
    }

    vfprintf(stdout, format, args);

    va_end(args);
}

void help()
{
    fprintf(stdout, "USAGE: glc [options] -o OUTPUT_FILE input_file\n");
}

void ast_statement_debug(FILE *restrict file, struct ast_statement *restrict expr)
{
    // TODO
}

void ast_expression_debug(FILE *restrict file, struct ast_expression *restrict expr)
{
    if (expr == NULL)
    {
        fprintf(file, "void");
        return;
    }

    switch (expr->type)
    {
    case E_AST_EXPR_CONSTANT:
    {
        fprintf(file, "Const(");
        switch (expr->value.constant.type)
        {
        case E_AST_CONST_INTEGER: fprintf(file, "%lu)", expr->value.constant.value.integer); break;
        case E_AST_CONST_STRING: fprintf(file, "\"%s\")", expr->value.constant.value.string); break;
        case E_AST_CONST_FLOATING:
            fprintf(
              file,
              "%lu.%lu)",
              expr->value.constant.value.floating.integer,
              expr->value.constant.value.floating.fractional);
            break;
        }
        break;
    }
    case E_AST_EXPR_VARIABLE: fprintf(file, "Var(name=\"%s\")", expr->value.variable_ident); break;
    case E_AST_EXPR_BLOCK:
    {
        fprintf(file, "Block[");
        for (size_t i = 0; i < expr->value.block.num_statements; i++)
        {
            ast_statement_debug(file, expr->value.block.statements + i);
            fprintf(file, ", ");
        }

        ast_expression_debug(file, expr->value.block.expression);
        fprintf(file, "]");
        break;
    }
    case E_AST_EXPR_SCOPE: ast_expression_debug(file, expr->value.expr); break;
    case E_AST_EXPR_OPER:
    {
        switch (expr->value.oper.op)
        {
        case E_AST_OP_INVERT: fprintf(file, "Invert("); break;
        case E_AST_OP_SUB: fprintf(file, "Sub("); break;
        case E_AST_OP_UNWRAP: fprintf(file, "Unwrap("); break;
        case E_AST_OP_NEGATE: fprintf(file, "Negate("); break;
        case E_AST_OP_ADD: fprintf(file, "Add("); break;
        case E_AST_OP_AND: fprintf(file, "BinaryAND("); break;
        case E_AST_OP_BORROW: fprintf(file, "Borrow("); break;
        case E_AST_OP_DEREF: fprintf(file, "Deref("); break;
        case E_AST_OP_DIV: fprintf(file, "Divide("); break;
        case E_AST_OP_EQ: fprintf(file, "Equal("); break;
        case E_AST_OP_GE: fprintf(file, "GreaterEqual("); break;
        case E_AST_OP_GT: fprintf(file, "GreaterThan("); break;
        case E_AST_OP_LE: fprintf(file, "LessEqual("); break;
        case E_AST_OP_LT: fprintf(file, "LessThan("); break;
        case E_AST_OP_MUL: fprintf(file, "Mul("); break;
        case E_AST_OP_NEQ: fprintf(file, "NotEqual("); break;
        case E_AST_OP_OR: fprintf(file, "BinaryOR("); break;
        case E_AST_OP_XOR: fprintf(file, "BinaryXOR("); break;
        }

        if (expr->value.oper.lhs != NULL)
        {
            ast_expression_debug(file, expr->value.oper.lhs);
            if (expr->value.oper.rhs != NULL) { fprintf(file, ", "); }
        }

        if (expr->value.oper.rhs != NULL) ast_expression_debug(file, expr->value.oper.rhs);

        fprintf(file, ")");
        break;
    }

    case E_AST_EXPR_CALL: fprintf(file, "Call(TODO)"); break;      // TODO
    case E_AST_EXPR_CAST: fprintf(file, "Cast(TODO)"); break;      // TODO
    case E_AST_EXPR_IF: fprintf(file, "If(TODO)"); break;          // TODO
    case E_AST_EXPR_LET: fprintf(file, "Let(TODO)"); break;        // TODO
    case E_AST_EXPR_LOOP: fprintf(file, "Loop(TODO)"); break;      // TODO
    case E_AST_EXPR_WHILE: fprintf(file, "While(TODO)"); break;    // TODO
    }
}

int main(const int argc, const char *const *argv)
{
    // Quick and dirty CLI

    if (argc == 1)
    {
        help();
        return 0;
    }

    // Parse Input
    int index = 0;
    while (++index < argc)
    {
        const char   *arg = argv[index];
        unsigned long len = strlen(arg);

        if (arg[0] != '-')
        {
            // We don't have an option (must be input file)
            if (opts.input_path != NULL)
            {
                glc_log(E_ERROR, "Input File already specified (%s)\n", opts.input_path);
                return -1;
            }
            opts.input_path = arg;
            continue;
        }

        if (strncmp(arg, "--", 2))
        {
            // Short Option
            unsigned long cursor = 1;

            while (cursor < len)
            {
                switch (arg[cursor++])
                {
                case 'v':
                {
                    opts.verbosity++;
                    continue;
                }
                case 'o':
                {
                    if (cursor != len || index == argc)
                    {
                        glc_log(E_ERROR, "Invalid usage of -o\n");
                        help();
                        return -1;
                    }
                    else
                    {
                        // Read next argv
                        opts.output_path = argv[++index];
                        break;
                    }
                }

                default: glc_log(E_ERROR, "Unknown Option: -%c\n", arg[cursor - 1]);
                case 'h':
                {
                    help();
                    return 0;
                }
                }
            }
        }
        else
        {
            // Long Option; By here we've guarenteed that the -- exists at least
            const char *opt = arg + 2;
            len -= 2;

            if (strncmp(opt, "version", 7) == 0)
            {
                fprintf(stdout, "GLang Compiler - Bootstrapper\nVersion: 0.1.0\n");
                return 0;
            }

            if (strncmp(opt, "output", 6) == 0)
            {
                len -= 6;
                opt += 6;

                // Values have two ways of working; --this=that, or --this that
                if (len == 0)
                {
                    // Next string must exist
                    if (++index == argc)
                    {
                        glc_log(
                          E_ERROR,
                          "Invalid Usage of --output: Expected value, found nothing.\n");
                        return -1;
                    }

                    opts.output_path = argv[index];
                }
                else
                {
                    if (len == 1 || opt[0] != '=')
                    {
                        glc_log(
                          E_ERROR,
                          "Invalid Usage of --output: Expected value, found nothing.\n");
                        return -1;
                    }

                    opts.output_path = opt + 1;
                }

                continue;
            }

            // Didn't Hit anything :(
            glc_log(E_ERROR, "Unknown Option: %s\n", opt);
            help();
            return -1;
        }
    }

    if (opts.output_path == NULL)
    {
        glc_log(E_ERROR, "Missing Output!\n");
        help();
        return -1;
    }

    if (opts.input_path == NULL)
    {
        glc_log(E_ERROR, "Missing Input!\n");
        help();
        return -1;
    }

    glc_log(E_DEBUG, "Verbosity: %d\n", opts.verbosity);
    glc_log(E_DEBUG, "Output Path: %s\n", opts.output_path);
    glc_log(E_DEBUG, "Input Path: %s\n", opts.input_path);

    // Compilation pipeline;
    // Tokenization; Convert input file into stream of tokens
    // Parsing; Parse stream of tokens into Abstract Syntax Tree
    // Compile; Generate LLVM IR from AST

    // First step in compiling; Tokenization
    FILE *input_file = fopen(opts.input_path, "r");
    if (input_file == NULL)
    {
        glc_log(
          E_ERROR,
          "Error reading Input File (\"%s\"); %s\n",
          opts.input_path,
          strerror(errno));
        return errno;
    }

    struct token_stream stream;
    int                 result = tokenize_file(&stream, input_file);
    switch (result)
    {
    case E_MEMORYERROR:
    case E_IOERROR: fclose(input_file); return -1;
    }

    printf("%lu Tokens\n", stream.size);
    /*
    struct token *cursor = stream.tokens;
    struct token  token;
    while ((token = *(cursor++)).value != E_TOKEN_EOF)
    {
        printf("\t");
        switch (token.value)
        {
        case E_TOKEN_INTEGER: printf("Integer: %lu\n", token.data.integer); continue;
        case E_TOKEN_STRING: printf("String: %s\n", token.data.string); continue;
        case E_TOKEN_IDENTIFIER: printf("Ident: %s\n", token.data.string); continue;
        case E_TOKEN_AS: printf("as\n"); continue;
        case E_TOKEN_ENUM: printf("enum\n"); continue;
        case E_TOKEN_CONST: printf("const\n"); continue;
        case E_TOKEN_EXTERN: printf("extern\n"); continue;
        case E_TOKEN_FUNCTION: printf("fn\n"); continue;
        case E_TOKEN_IF: printf("if\n"); continue;
        case E_TOKEN_IMPLEMENTS: printf("impl\n"); continue;
        case E_TOKEN_LET: printf("let\n"); continue;
        case E_TOKEN_MATCH: printf("match\n"); continue;
        case E_TOKEN_PUBLIC: printf("pub\n"); continue;
        case E_TOKEN_STRUCT: printf("struct\n"); continue;
        case E_TOKEN_TRAIT: printf("trait\n"); continue;
        case E_TOKEN_CONTINUE: printf("continue\n"); continue;
        case E_TOKEN_BREAK: printf("break\n"); continue;
        case E_TOKEN_ELSE: printf("else\n"); continue;
        case E_TOKEN_LOOP: printf("loop\n"); continue;
        case E_TOKEN_WHILE: printf("while\n"); continue;
        case E_TOKEN_EOF: break;
        }

        printf("%c\n", token.value);
    }
        */

    struct ast_program program = {
        .node_capacity = 0,
        .num_nodes     = 0,
        .nodes         = NULL,
    };
    result = ast_program_from_tokens(&program, &stream);
    switch (result)
    {
    case E_AST_UNEXPECTED:
    case E_AST_MEMORYERROR:
        token_stream_free(&stream);
        fclose(input_file);
        return -1;
    }

    printf("%lu Nodes\n", program.num_nodes);
    for (size_t i = 0; i < program.num_nodes; i++)
    {
        struct ast_node *node = program.nodes + i;
        switch (node->type)
        {
        case E_AST_NODE_EXTERNAL:
        case E_AST_NODE_FUNCTION:
            // TODO: Debug Functions
            printf("uhh..... not debuggable yet?\n");
            break;

        case E_AST_NODE_CONSTANT:
        {
            fprintf(
              stdout,
              "Const(ident=\"%s\", type=\"%s\", value=",
              node->ident,
              node->node_type.type_name);
            ast_expression_debug(stdout, &node->data.value);

            printf(")\n");
            break;
        }
        }
    }

    ast_program_free(&program);
    token_stream_free(&stream);
    fclose(input_file);
}