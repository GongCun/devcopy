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

/* #define MAX_STR 1024 */
#define MAX_STR 4096

struct slice {
    unsigned long long  seq;
    int                 len;
    char               *buf;
};

struct commit_info {
    uLong  cm_version;          /* hash code of commit date     */
    uLong  cm_pversion;         /* parent version               */
    int    cm_current_flag;     /* identify the current version */
    time_t cm_date;             /* commit date                  */
    char   cm_author[64];       /* submitter                    */
    char   cm_message[MAX_STR]; /* description of commit        */
};

#ifdef _AIX
int asprintf(char **strp, const char *format, ...);
#endif


#endif
