#include "error.h"
static void err_doit(int errnoflag, int error, const char *format, va_list args);

void err_sys(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    err_doit(1, errno, format, args);
    va_end(args);

    exit(-1);
    
}

void err_ret(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    err_doit(1, errno, format, args);
    va_end(args);

    return;
    
}


void err_msg(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    err_doit(0, 0, format, args);
    va_end(args);

    return;
    
}


void err_quit(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    err_doit(0, 0, format, args);
    va_end(args);

    exit(-1);
}

void err_exit(int error, const char *format, ...)
{
    va_list args;

    va_start(args, format);
    err_doit(1, error, format, args);
    va_end(args);

    exit(-1);
    
}

static void err_doit(int errnoflag, int error, const char *format, va_list args)
{
    int size;
    char *buf;
    va_list copy;
    va_copy(copy, args); /* C99 give the va_copy() macro */
    

    if (!errnoflag)
        {
            vfprintf(stderr, format, args);
            fputs("\n", stderr);
            fflush(NULL); /* flush all open output streams */
            return;
        }


    /*
     * Allocate buffer to store "string: error message", print buffer to stderr,
     * then release the buffer.
     */

    size = vsnprintf(NULL, 0, format, args);
    if (size == 0 || format == NULL)
        {
            fputs(strerror(error), stderr);
            fputs("\n", stderr);
            fflush(NULL);
            return;
        }

    
    if (size < 0)
        {
            fputs(strerror(errno), stderr);
            fputs("\n", stderr);
            fflush(NULL);
            return;
        }

    size++;
    buf = malloc(size);
    if (buf == NULL)
        {
            perror("malloc");
            fflush(NULL);
            return;
        }

    size = vsnprintf(buf, size, format, copy);
    va_end(copy);
    if (size < 0)
        {
            free(buf);
            fputs(strerror(errno), stderr);
            fputs("\n", stderr);
            fflush(NULL);
            return;
        }

    char *err_str = strerror(error);
    char *tmp = buf;
    buf = realloc(buf, size + strlen(err_str) + 3);
    if (buf == NULL)
        {
            free(tmp);
            perror("realloc");
            fflush(NULL);
            return;
        }

    strcat(buf + strlen(buf), ": ");
    strcat(buf + strlen(buf), err_str);
    fprintf(stderr, "%s\n", buf);
    fflush(NULL);
    free(buf);
    return;
}
