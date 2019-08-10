#ifndef _DEVCOPY
#define _DEVCOPY

#define STDC_WANT_LIB_EXT1 1
#define _GNU_SOURCE
#include <zlib.h>
#include <stdarg.h>
#define BUFLEN 0x400000L /* 4M */
#define ULONG_LEN (sizeof(uLong))

struct record {
    unsigned long long seq;
    int len;
    char *buf;
};

#ifdef _AIX
int asprintf(char **strp, const char *format, ...);
#endif

#endif
