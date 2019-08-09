#ifndef _DEVCOPY
#define _DEVCOPY

#define STDC_WANT_LIB_EXT1 1
#define _GNU_SOURCE
#define BUFLEN 0x400000L /* 4M */

struct record {
    unsigned long long seq;
    int len;
    char *buf;
};

#endif
