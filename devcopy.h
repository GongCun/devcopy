#ifndef _DEVCOPY
#define _DEVCOPY

#define STDC_WANT_LIB_EXT1 1
#define _GNU_SOURCE
#include "ktree.h"
#include "list.h"
#include <zlib.h>
#include <ndbm.h>
#include <stdlib.h>
#include <stdio.h>
#include <regex.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <time.h>
#include <limits.h>
#include <libgen.h>

#define BUFLEN      0x400000L /* 4M */
#define MAX_STR     4096
#define MAX_NODE    1024
#define MAX_AUTHOR  64
#define ULONG_LEN   (sizeof(uLong))
#define VERSION_DIR "./.devcopy"
#define TRACE_FILE  "./devcopy.trc"
#define FILE_MODE   (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH) /* 0644 */
#define DIR_MODE    (FILE_MODE | S_IXUSR | S_IXGRP | S_IXOTH) /* 0755 */
#define DB_FILE     VERSION_DIR	"/devcopy.dbm" /* ./.devcopy/devcopy.dbm */
#define MYBUFLEN    1024

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

extern int verbose;
extern char *gfname;

struct slice {
    unsigned long long  seq;
    int                 len;
    char               *buf;
};

struct commit_info {
    uLong  cm_version;            /* hash code of commit date     */
    uLong  cm_pversion;           /* parent version               */
    int    cm_current_flag;       /* identify the current version */
    time_t cm_mtime;              /* time of last modification    */
    time_t cm_date;               /* commit date                  */
    char   cm_author[MAX_AUTHOR]; /* submitter                    */
    char   cm_message[MAX_STR];   /* description of commit        */
};

int match_name(const char *filename, const char *str);
uLong commit_version(void);
uLong current_version(DBM *dbm_db, struct commit_info **cm);
void commit_author(struct commit_info *p);
char *commit_message(struct commit_info *p);
void print_commit(void *data);
void retrieve_data(DBM *dbm_db, KTree *tree, KTreeNode *node, uLong pv);
void insert_commit(DBM *dbm_db, struct commit_info *pc, const char *fname);
void checkout_commit(DBM *dbm_db, uLong checkout, KTree *tree);


#endif
