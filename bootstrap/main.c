#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

void help()
{
    printf("GLang Compiler!\n");
}

void version()
{
    printf("GLang Compiler - Bootstrapper\nVersion: 0.1.0\n");
}

int main(const int argc, const char *const *argv)
{
    // Quick and dirty CLI

    if (argc == 1)
    {
        help();
        return 0;
    }

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
            char *opt2 = opt + 1;
            printf("Short: %s\n", opt2 + 1);

            switch (opt[1])
            {
            case 'v':
            {
                version();
                return 0;
            }
            case 'h':
            default:
            {
                help();
                return 0;
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
}