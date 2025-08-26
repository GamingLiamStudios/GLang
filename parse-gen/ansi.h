#pragma once

#if defined(__cplusplus)
extern "C"
{
#endif

#define ANSI_FMT_BEGIN "\x1b[0"
#define ANSI_FMT_END   "m"
#define ANSI_FMT_RESET "\x1b[0m"

#define ANSI_FMT_BOLD      ";1"
#define ANSI_FMT_ITALIC    ";3"
#define ANSI_FMT_UNDERLINE ";4"
#define ANSI_FMT_STRIKE    ";9"

#define ANSI_FMT_COLOR_FG ";3"
#define ANSI_FMT_COLOR_BG ";4"

#define ANSI_FMT_COLOR_BLACK   "0"
#define ANSI_FMT_COLOR_RED     "1"
#define ANSI_FMT_COLOR_GREEN   "2"
#define ANSI_FMT_COLOR_YELLOW  "3"
#define ANSI_FMT_COLOR_BLUE    "4"
#define ANSI_FMT_COLOR_MAGENTA "5"
#define ANSI_FMT_COLOR_CYAN    "6"
#define ANSI_FMT_COLOR_WHITE   "7"

#define ANSI_FMT_COLOR_RGB24(r, g, b) "8;2;" #r ";" #g ";" #b

#if defined(__cplusplus)
}
#endif