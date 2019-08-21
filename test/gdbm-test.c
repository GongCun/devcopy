#include "devcopy.h"
#include "error.h"
#include <stdlib.h>
#include <stdio.h>
#include <ndbm.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <regex.h>
#include <time.h>
#include <fcntl.h>

#define MYBUFLEN 1024
#define DB_FILE "./devcopy.dbm"

static void copy_data(datum *pdatum, struct commit *pcommit)
{
    int tmp;
    
    pdatum->dsize = sizeof(pcommit->comm_ver);
    pdatum->dsize += sizeof(pcommit->comm_pver);
    pdatum->dsize += sizeof(pcommit->comm_date);
    pdatum->dsize += sizeof(pcommit->comm_author);

    tmp = pdatum->dsize;

    pdatum->dptr = malloc(pdatum->dsize);
    if (pdatum->dptr == NULL) {
        err_sys("malloc");
    }
    memcpy(pdatum->dptr, pcommit, pdatum->dsize);
    
    pdatum->dsize += strlen(pcommit->comm_msg) + 1;
    pdatum->dptr = realloc(pdatum->dptr, pdatum->dsize);
    if (!pdatum->dptr) {
        err_sys("realloc");
    }

    strcpy(pdatum->dptr + tmp, pcommit->comm_msg);

}
static int chk_emp_comm(const char *str)
{
    regex_t regex;
    int i, ret;
    char errbuf[MYBUFLEN];
    char *pattern[] = {
        "^[[:space:]]*$",
        "^[[:space:]]*#"
    };

    for (i = 0;
         i < (int)(sizeof(pattern) / sizeof(pattern[0]));
         i++)
    {
        ret = regcomp(&regex, pattern[i], REG_EXTENDED);
        if (ret) {
            err_quit("Can't compile regex");
        }
        ret = regexec(&regex, str, 0, NULL, 0);
        if (!ret) {
            regfree(&regex);
            return 1; /* match */
        }
        else if (ret == REG_NOMATCH) {
            regfree(&regex);
        }
        else {
            regfree(&regex);
            regerror(ret, &regex, errbuf, sizeof(errbuf));
            err_quit("Regex match failed: %s", errbuf);
        }
    }

    return 0; /* no match */
}

static char *commit_parser(FILE *file)
{
    char *buf = NULL;
    char readin[MYBUFLEN];
    int copylen = 0, alloc = 0;

    
    while (fgets(readin, sizeof(readin), file) != NULL) {
        if (chk_emp_comm(readin)) {
            continue;
        }
        alloc = copylen + strlen(readin) + 1;
        buf = realloc(buf, alloc);
        if (buf == NULL) {
            err_sys("realloc");
        }
        strncpy(buf + copylen, readin, strlen(readin) + 1);
        copylen += strlen(readin);
    }

    return buf;
}

static char *commit_msg(void)
{
    int fd;
    int len;
    int ret;
    char *Editor;
    char buf[MYBUFLEN];
    FILE *Commit;
    char *parse_result;
    char template[] = "/tmp/devcopy-XXXXXX";
    char *hint = "\n\n\n# Please enter a description of the submission";
    

    if ((fd = mkstemp(template)) < 0) {
        err_sys("mkstemp");
    }

    len = strlen(hint);
    if (write(fd, hint, len) != len) {
        err_sys("write");
    }
    close(fd);

    if (!(Editor = getenv("EDITOR"))) {
        Editor = strdup("/usr/bin/vi");
    }

    if (strlen(Editor) + strlen(template) + 2 > sizeof(buf)) {
        err_quit("Buffer too short");
    }
    sprintf(buf, "%s %s", Editor, template);
    ret = system(buf);
    struct stat statbuf;
    if ((ret != -1 && (ret >> 8) == 0) || (ret >> 8) == 1) {
        /*
         * system() fork new process succeed and return code is zero, or run
         * background is okay.
         */
        if (stat(template, &statbuf) < 0) {
            err_sys("Can't stat temporary file (%s)", template);
        }
        if (statbuf.st_size == 0) {
            err_quit("Zero length template file (%s)", template);
        }

        /*
         * Reopen the temporary file and extract the content to description
         * buffer.
         */
        Commit = fopen(template, "r");

        if (Commit == NULL) {
            err_sys("fopen");
        }
        parse_result = commit_parser(Commit);
    }
    else {
        err_sys("Editor (%s) failed with exit status %d", ret);
    }

    /* free(Editor); */
    unlink(template);
    return parse_result;
}

static time_t commit_date() {
    return time(NULL);
}

static uLong commit_ver() {
    int fd;
    uLong crc;
    char buf[MYBUFLEN];
    
    fd = open("/dev/urandom", O_RDONLY);
    if (fd < 0) {
        err_sys("open /dev/urandom");
    }
    if (read(fd, buf, sizeof(buf)) != sizeof(buf)) {
        err_sys("read /dev/urandom");
    }
    close(fd);

    crc = crc32(0L, Z_NULL, 0);
    return crc32(crc, (const Bytef *)buf, sizeof(buf));

}

static void input_record(void)
{
    int n;
    struct commit commit, *pcommit;
    uLong current_ver = 0;
    char *s;

#define author (commit.comm_author)
#define msg (commit.comm_msg)
#define version (commit.comm_ver)
#define pversion (commit.comm_pver)
#define date (commit.comm_date)

    datum dbm_key, dbm_data;
    
    DBM *dbm_db = dbm_open(DB_FILE, O_RDWR|O_CREAT, 0644);
    if (!dbm_db) {
        err_quit("%s", gdbm_strerror(gdbm_errno));
    }

    while (1) {
        printf("input your name: ");

        s = fgets(author, sizeof(author), stdin);
        if (s != NULL && strlen(author) > 0) {
            author[strlen(author) - 1] = '\0';
        }

        /* printf("name = %s\n", commit.comm_author); */
        if (s == NULL) {
            break;
        }
        msg = commit_msg();
        if (msg == NULL) {
            err_quit("\nFailed to parse commit message.");
        }
        date = commit_date();
        version = commit_ver();
        pversion = current_ver;
        current_ver = version;
        dbm_key.dptr = (void *)&version;
        dbm_key.dsize = sizeof(version);
        copy_data(&dbm_data, &commit);
        n = dbm_store(dbm_db, dbm_key, dbm_data, DBM_REPLACE);
        if (n != 0) {
            err_quit(gdbm_strerror(gdbm_errno));
        }
        free(msg);
        free(dbm_data.dptr);
    }
    /* Retrieving data */
    for (dbm_key = dbm_firstkey(dbm_db);
            dbm_key.dptr;
            dbm_key = dbm_nextkey(dbm_db))
        {
            dbm_data = dbm_fetch(dbm_db, dbm_key);
            if (dbm_data.dptr) {
                /* printf("Data retrieved\n"); */
                pcommit = (struct commit *)dbm_data.dptr;
                s = ctime(&Commit_date(pcommit));
                s[strlen(s) - 1] = '\0'; /* ctime() end with '\n' */
                /* printf("date = %s\n", s); */

                printf("message: %s\n", Commit_messgae(pcommit));

                printf("author: %s\n"
                       "date: %s\n"
                       "version: %lx\n"
                       "parent version: %lx\n\n",
                       /* "commit message\n" */
                       /* "==============\n%s\n", */
                       Commit_author(pcommit), s, Commit_version(pcommit),
                       Commit_pversion(pcommit));
                       /* Commit_pversion(pcommit), Commit_messgae(pcommit)); */
            }
            else {
                printf("No data found\n");
            }
        }
}

int main(int argc, char *argv[])
{

    input_record();
    return 0;
}
