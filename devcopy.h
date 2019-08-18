#ifndef _DEVCOPY
#define _DEVCOPY

#define STDC_WANT_LIB_EXT1 1
#define _GNU_SOURCE
#include <zlib.h>
#include <stdarg.h>

#ifdef _LINUX
#include <ndbm.h>
#endif

#define BUFLEN 0x400000L /* 4M */
#define ULONG_LEN (sizeof(uLong))

#define MAX_STR 1024

struct slice {
    unsigned long long seq;
    int len;
    char *buf;
};

struct record {
    uLong version; /* hash code of commit date */
    uLong parent_version;
    char date[32];
    char author[64];
    char *description;
};

#ifdef _AIX
int asprintf(char **strp, const char *format, ...);
#endif

/* void destroy(void *data); */


#endif
