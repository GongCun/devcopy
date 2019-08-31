#include "asprintf.h"

int asprintf(char **strp, const char *format, ...)
{
    int count;
    va_list ap;
    char *buffer;

    *strp = NULL; /* Ensure value can be passed to free() */
    va_start(ap, format);
    count = vsnprintf(NULL, 0, format, ap);
    va_end(ap);

    if (count >= 0)
    {
        buffer = malloc(count + 1);
        if (buffer == NULL)
        {
            return -1;
        }

        va_start(ap, format);
        count = vsnprintf(buffer, count+1, format, ap);
        va_end(ap);

        if (count < 0)
        {
            free(buffer);
            return count;
        }
        *strp = buffer;
    }

    return count;
}
