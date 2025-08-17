// Sort of counterintuitively, but we generate a parser using a parser.
#include "token.h"
#include "log.h"
#include "error.h"

#include <string.h>
#include <stdlib.h>

#define INPUTPATHS_GROWTHRATE   2
#define INPUTPATHS_INITCAPACITY 1

struct Options
{
    const char **input_paths;
    const char  *output_path;
};

struct Options opts = {
    .output_path = NULL,
    .input_paths = NULL,
};

enum LogLevel verbosity;

void help()
{
    fprintf(stdout, "USAGE: glc_pg [options] -o OUTPUT_FILE input_files...\n");
}

int __alloc_input_paths(const char ***paths, size_t capacity)
{
    const char **newptr;
    if (paths == NULL) { return E_GLCPG_INVALIDINPUT; }

    if (capacity == 0)
    {
        free(*paths);
        *paths = NULL;
        return 0;
    }

    newptr = realloc(*paths, sizeof(const char **) * capacity);
    if (newptr == NULL)
    {
        free(*paths);
        *paths = NULL;
        return E_GLCPG_MEMORYERROR;
    }

    *paths = newptr;
    return 0;
}

int main(const int argc, const char *const *argv)
{
    // Quick and dirty CLI
    size_t num_paths;
    size_t paths_capacity;
    int    ret, index;

    paths_capacity = INPUTPATHS_INITCAPACITY;
    num_paths      = 0;
    ret            = __alloc_input_paths(&opts.input_paths, paths_capacity);
    if (ret < 0) { return ret; }

    verbosity = E_INFO;

    if (argc == 1)
    {
        help();
        return 0;
    }

    // Parse Input
    index = 0;
    while (++index < argc)
    {
        const char   *arg = argv[index];
        unsigned long len = strlen(arg);

        if (arg[0] != '-')
        {
            // We don't have an option (must be input file)
            if (num_paths == paths_capacity)
            {
                paths_capacity *= INPUTPATHS_GROWTHRATE;
                ret = __alloc_input_paths(&opts.input_paths, paths_capacity);
                if (ret < 0) { return ret; }
            }

            opts.input_paths[num_paths++] = arg;
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
                fprintf(stdout, "GLang Parser Generator\nVersion: 0.1.0\n");
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

    if (opts.input_paths == NULL || num_paths == 0)
    {
        glc_log(E_ERROR, "Missing Input!\n");
        help();
        return -1;
    }

    glc_log(E_DEBUG, "Verbosity: %d\n", verbosity);
    glc_log(E_DEBUG, "Output Path: %s\n", opts.output_path);
    glc_log(E_DEBUG, "Input Paths: \n");

    for (size_t i = 0; i < num_paths; i++) { glc_log(E_DEBUG, "  - %s\n", opts.input_paths[i]); }

    free(opts.input_paths);
}