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
    unsigned long long  seq;
    int                 len;
    char               *buf;
};

struct commit {
    uLong   comm_ver;           /* hash code of commit date */
    uLong   comm_pver;          /* parent version           */
    time_t  comm_date;          /* commit date              */
    char    comm_author[64];    /* submitter                */
    char   *comm_msg;           /* description of commit    */
};

#define Commit_version(commit)  ((commit) -> comm_ver)
#define Commit_pversion(commit) ((commit) -> comm_pver)
#define Commit_date(commit)     ((commit) -> comm_date)
#define Commit_author(commit)   ((commit) -> comm_author)
#define Commit_messgae(commit)  ((commit) -> comm_msg)

#ifdef _AIX
int asprintf(char **strp, const char *format, ...);
#endif

/* void destroy(void *data); */


#endif
