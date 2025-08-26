#pragma once

#if defined(__cplusplus)
extern "C"
{
#endif

    enum LogLevel
    {
        E_ERROR = 0,
        E_WARN,
        E_INFO,
        E_DEBUG,
    };

    void glc_log(enum LogLevel level, const char *format, ...);

#if defined(__cplusplus)
}
#endif