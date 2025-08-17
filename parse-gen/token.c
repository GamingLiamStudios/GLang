#include "token.h"
#include "log.h"
#include "error.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#define TOKENSTREAM_GROWTHFACTOR 2
#define TOKENSTREAM_INITCAPACITY 128

#define MIN(a, b) ((a) > (b) ? (b) : (a))

int __alloc_token_stream(struct glcpg_token **stream, size_t capacity)
{
    struct glcpg_token *newptr;
    if (stream == NULL) { return E_GLCPG_INVALIDINPUT; }

    if (capacity == 0)
    {
        free(*stream);
        *stream = NULL;
        return 0;
    }

    newptr = realloc(*stream, sizeof(struct glcpg_token) * capacity);
    if (newptr == NULL)
    {
        free(*stream);
        *stream = NULL;
        return E_GLCPG_MEMORYERROR;
    }

    *stream = newptr;
    return 0;
}

int glcpg_token_debug(char *restrict string, size_t maxlen, const struct glcpg_token token)
{
    int ret;
    if (string == NULL && maxlen != 0) { return E_GLCPG_INVALIDINPUT; }

    switch (token.type)
    {
    case E_PGTOK_EOF: ret = snprintf(string, maxlen, "EOF"); break;
    case E_PGTOK_EQUAL: ret = snprintf(string, maxlen, "EQ"); break;
    case E_PGTOK_IDENT: ret = snprintf(string, maxlen, "Ident(%s)", token.value.ident); break;
    case E_PGTOK_TERMINAL:
        ret = snprintf(string, maxlen, "Terminal(%s)", token.value.terminal.name);
        break;
    }

    return ret;
}

ptrdiff_t glcpg_lexer_file(
  struct glcpg_token *restrict *result,
  FILE                         *file,
  const struct glcpg_terminal *restrict const terminals,
  size_t num_terminals)
{
    // TODO: Heap Allocate instead of Stack
    char                cur, current_string[1024];
    struct glcpg_token *stream;
    size_t              capacity, num_elems, cur_len;
    int                 ret;

    if (result == NULL) { return E_GLCPG_INVALIDINPUT; }
    memset(current_string, 0, sizeof(current_string));

    num_elems = 0;
    capacity  = TOKENSTREAM_INITCAPACITY;
    ret       = __alloc_token_stream(&stream, TOKENSTREAM_INITCAPACITY);
    if (ret < 0) { return ret; }

    cur_len = 0;
    while (1)
    {
        cur = fgetc(file);
        if (cur == EOF && !feof(file))
        {
            glc_log(E_ERROR, "Error while reading file: %s\n", strerror(errno));
            return E_GLCPG_IOERROR;
        }

        if (capacity == num_elems)
        {
            capacity *= TOKENSTREAM_GROWTHFACTOR;
            ret = __alloc_token_stream(&stream, capacity);
            if (ret < 0) { return ret; }
        }

        if (isspace(cur) || cur == EOF)
        {
            if (3 == cur_len && strncmp(current_string, "::=", 3) == 0)
            {
                stream[num_elems].type = E_PGTOK_EQUAL;
                num_elems += 1;
                cur_len = 0;
                continue;
            }

            for (size_t i = 0; i < num_terminals; i++)
            {
                const struct glcpg_terminal terminal = terminals[i];
                size_t                      len      = strlen(terminal.value);

                if (len == cur_len && strncmp(current_string, terminal.value, cur_len) == 0)
                {
                    stream[num_elems].type           = E_PGTOK_TERMINAL;
                    stream[num_elems].value.terminal = terminal;

                    num_elems += 1;
                    cur_len = 0;
                    continue;
                }
            }

            stream[num_elems].type        = E_PGTOK_IDENT;
            stream[num_elems].value.ident = calloc(cur_len + 1, sizeof(char));
            strncpy((char *) stream[num_elems].value.ident, current_string, cur_len);

            num_elems += 1;
            cur_len = 0;

            if (cur == EOF) { break; }
        }
        else { current_string[cur_len++] = cur; }
    }

    // shrink to fit
    stream[num_elems++].type = E_PGTOK_EOF;
    ret                      = __alloc_token_stream(&stream, num_elems);
    if (ret < 0) { return ret; }

    return num_elems;
}