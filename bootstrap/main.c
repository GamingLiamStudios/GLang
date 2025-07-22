#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>

enum LogLevel
{
    E_DEBUG,
    E_INFO,
    E_WARN,
    E_ERROR,
};

void glc_log(enum LogLevel level, const char *restrict format, ...)
{
    va_list args;
    va_start(args, format);

    switch (level)
    {
    case E_DEBUG:
    {
        fprintf(stdout, "[DEBUG] ");
        break;
    }
    case E_INFO:
    {
        fprintf(stdout, "[INFO] ");
        break;
    }
    case E_WARN:
    {
        fprintf(stdout, "[WARN] ");
        break;
    }
    case E_ERROR:
    {
        fprintf(stderr, "[ERROR] ");
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
    fprintf(stdout, "USAGE: glc [options] -o OUTPUT_FILE input...\n");
}

void version()
{
    printf("GLang Compiler - Bootstrapper\nVersion: 0.1.0\n");
}

struct CompilerOpts
{
    int         verbosity;
    const char *output_path;
};

int main(const int argc, const char *const *argv)
{
    // Quick and dirty CLI

    if (argc == 1)
    {
        help();
        return 0;
    }

    struct CompilerOpts opts = {
        .verbosity   = 0,
        .output_path = NULL,
    };

    // Parse pre-options
    int index = 0;
    while (++index < argc)
    {
        const char   *opt = argv[index];
        unsigned long len = strlen(opt);

        if (isalpha(opt[0]))
        {
            // We don't have an option (didn't match)
            break;
        }

        if (strncmp(opt, "--", 2))
        {
            // Short Option
            unsigned long cursor = 1;

            while (cursor < len)
            {
                switch (opt[cursor++])
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
                        printf("Invalid usage!\n");
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
                case 'h':
                default:
                {
                    help();
                    return 0;
                }
                }
            }
        }
        else
        {
            // Long Option
            printf("Long: %s\n", opt + 2);

            help();
            return 0;
        }
    }

    if (opts.output_path == NULL)
    {
        glc_log(E_ERROR, "Missing Output!\n");
        help();
        return -1;
    }

    glc_log(E_DEBUG, "Verbosity: %d\n", opts.verbosity);
    glc_log(E_DEBUG, "Output Path: %s\n", opts.output_path);
}