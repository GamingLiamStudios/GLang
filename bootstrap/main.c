#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>

#include "log.h"
#include "bootstrap/parser.h"
#include "bootstrap/lexer.h"

#include <llvm-c/Core.h>
#include <llvm-c/Target.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Analysis.h>

struct Options
{
    const char *input_paths;
    const char *output_path;
};

struct Options opts = {
    .output_path = NULL,
    .input_paths = NULL,
};

void help()
{
    fprintf(stdout, "USAGE: glc [options] -o OUTPUT_FILE input_file\n");
}

int main(const int argc, const char *const *argv)
{
    // Quick and dirty CLI
    verbosity = E_INFO;

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
            if (opts.input_paths != NULL)
            {
                glc_log(E_ERROR, "Input File already specified (%s)\n", opts.input_paths);
                return -1;
            }
            opts.input_paths = arg;
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
                    verbosity++;
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

    if (opts.input_paths == NULL)
    {
        glc_log(E_ERROR, "Missing Input!\n");
        help();
        return -1;
    }

    glc_log(E_DEBUG, "Verbosity: %d\n", verbosity);
    glc_log(E_DEBUG, "Output Path: %s\n", opts.output_path);
    glc_log(E_DEBUG, "Input Path: %s\n", opts.input_paths);

    // Compilation pipeline;
    // Tokenization; Convert input file into stream of tokens
    // Parsing; Parse stream of tokens into Abstract Syntax Tree
    // Validation; Check AST contains valid code
    // Compile; Generate LLVM IR from AST

    // First step in compiling; Tokenization
    FILE *input_file = fopen(opts.input_paths, "r");
    if (input_file == NULL)
    {
        glc_log(
          E_ERROR,
          "Error reading Input File (\"%s\"); %s\n",
          opts.input_paths,
          strerror(errno));
        return errno;
    }

    yyscan_t scanner;
    if (yylex_init(&scanner)) return -1;
    yyset_in(input_file, scanner);

    // yydebug = 1;
    // yyset_debug(1, scanner);

    struct glc_ast_root root = {
        .decls     = NULL,
        .num_decls = 0,
    };

    int result = yyparse(&root, scanner);
    if (result) { glc_log(E_ERROR, "ruhoh\n"); }

    yylex_destroy(scanner);
    fclose(input_file);

    glc_log(E_DEBUG, "num decls %lu\n", root.num_decls);
    for (size_t i = 0; i < root.num_decls; i++)
    {
        struct glc_ast_statement *decl = root.decls + i;

        // TODO: Display expr tree
        switch (decl->type)
        {
        case E_AST_STMT_VARIABLE:
            glc_log(E_DEBUG, "%s (%s)\n", decl->v_variable.ident, decl->v_variable.type.ident);
            break;
        case E_AST_STMT_FUNCTION:
            glc_log(E_DEBUG, "%s (%lu: ", decl->v_func.ident, decl->v_func.num_args);
            for (int i = 0; i < decl->v_func.num_args; i++)
            {
                fprintf(stdout, "%s,", decl->v_func.args[i].ident);
            }
            fprintf(stdout, ") -> %s\n", decl->v_func.type.ident);
            break;
        }
    }

    LLVMContextRef ctx     = LLVMContextCreate();
    LLVMBuilderRef builder = LLVMCreateBuilderInContext(ctx);
    LLVMModuleRef  module  = LLVMModuleCreateWithNameInContext("the_bees", ctx);

    LLVMTypeRef int32_type = LLVMIntTypeInContext(ctx, 32);

    LLVMTypeRef params_type[2] = { int32_type, int32_type };

    LLVMTypeRef  func_type = LLVMFunctionType(int32_type, params_type, 2, 0);
    LLVMValueRef func      = LLVMAddFunction(module, "mul_add", func_type);
    LLVMSetFunctionCallConv(func, LLVMFastCallConv);
    // LLVMCCallConv - C ABI Compat
    // LLVMFastCallConv - Speedy
    // LLVMColdCallConv - Cold Functions
    // LLVMTailCallConv - Speedy + Tail Call Optim guarentee

    LLVMBasicBlockRef block = LLVMCreateBasicBlockInContext(ctx, "entry");
    LLVMAppendExistingBasicBlock(func, block);
    LLVMPositionBuilderAtEnd(builder, block);

    LLVMValueRef param_a = LLVMGetParam(func, 0);
    LLVMValueRef param_b = LLVMGetParam(func, 1);
    LLVMValueRef sum     = LLVMBuildAdd(builder, param_a, param_b, "sum");
    LLVMBuildRet(builder, sum);

    char *error = NULL;
    if (LLVMVerifyModule(module, LLVMAbortProcessAction, &error))
    {
        fprintf(stderr, "Error: %s\n", error);
        LLVMDisposeMessage(error);
        return 1;
    }

    LLVMPrintModuleToFile(module, "output.ll", &error);
    printf("Generated IR:\n");
    LLVMDumpModule(module);

    // LLVMBuildMul(module, LLVMValueRef LHS, LLVMValueRef RHS, const char *Name)

    LLVMDisposeBuilder(builder);
    LLVMContextDispose(ctx);
}