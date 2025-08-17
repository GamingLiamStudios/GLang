#include "log.h"
#include "ansi.h"

#include <stdio.h>
#include <stddef.h>
#include <stdarg.h>

extern enum LogLevel verbosity;

// TODO: Make this common between bootstrap and parse-gen
void glc_log(enum LogLevel level, const char *restrict format, ...)
{
    if (level > verbosity) return;

    va_list args;
    va_start(args, format);

    switch (level)
    {
    default:
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