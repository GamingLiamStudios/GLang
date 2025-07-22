#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>

#include "log.h"
#include "ansi.h"

struct InputFilePath
{
    const char           *path;
    struct InputFilePath *next;
};

unsigned long inputfiles_len(struct InputFilePath *restrict list)
{
    unsigned long count = 0;
    while (list != NULL)
    {
        count++;
        list = list->next;
    }

    return count;
}

struct CompilerOpts
{
    struct InputFilePath *input_files;
    const char           *output_path;
    int                   verbosity;
};

struct CompilerOpts opts = {
    .verbosity   = E_INFO,
    .output_path = NULL,
    .input_files = NULL,
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
    fprintf(stdout, "USAGE: glc [options] -o OUTPUT_FILE input...\n");
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

        if (isalnum(arg[0]))
        {
            // We don't have an option (must be input file)
            // man i miss vectors...
            struct InputFilePath *current = malloc(sizeof(struct InputFilePath));
            current->path                 = arg;
            current->next                 = opts.input_files;

            opts.input_files = current;
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

    if (opts.input_files == NULL)
    {
        glc_log(E_ERROR, "Missing Input!\n");
        help();
        return -1;
    }

    glc_log(E_DEBUG, "Verbosity: %d\n", opts.verbosity);
    glc_log(E_INFO, "Output Path: %s\n", opts.output_path);
    glc_log(E_DEBUG, "Input Files (%d);\n", inputfiles_len(opts.input_files));

    struct InputFilePath *path = opts.input_files;
    while (path != NULL)
    {
        glc_log(E_DEBUG, "\t%s\n", path->path);
        path = path->next;
    }
}