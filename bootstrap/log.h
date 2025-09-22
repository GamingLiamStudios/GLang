#pragma once

enum LogLevel
{
    E_ERROR = 0,
    E_WARN,
    E_INFO,
    E_DEBUG,
};

extern enum LogLevel verbosity;

void glc_log(enum LogLevel level, const char *format, ...);