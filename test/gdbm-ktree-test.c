#include "devcopy.h"
#include "error.h"
#include "ktree.h"
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

static void retrieve_data(DBM *dbm_db, KTree *tree, KTreeNode *node, uLong pv);

static int compare_comm(const void *key1, const void *key2)
{
    struct commit_info *p1, *p2;

    p1 = (struct commit_info *)key1;
    p2 = (struct commit_info *)key2;
    
    return *(uLong *)p1 -> cm_version == *(uLong *)p2 -> cm_version ? 1 : 0;
}

static void print_comm(void *data)
{
    struct commit_info *p;
    char buf[MAX_STR];
    time_t now, between;
    int n;
    
    p = (struct commit_info *)data;

    printf("%lx - ", p -> cm_version);

    now = time(NULL);
    between = now - p -> cm_date;
    /*
     * 1 minute = 60 sec
     * 1 hour   = 60 minutes = 3600 sec
     * 1 day    = 24 hours   = 24 * 3600 sec
     * 1 week   = 7 days     = 7 * 24 * 3600 sec
     */
    if (between < 3600) {
        n = between / 60 + 1;
        printf("(%d minute%s ago)", n, n == 1 ? "" : "s");
    }
    else if (between < 3600 * 24 * 2) {
        n = between / 3600 + 1;
        printf("(%d hour%s ago)", n, n == 1 ? "" : "s");
    }
    else if (between < 3600 * 24 * 7) {
        n = between / (3600 * 24) + 1;
        printf("(%d day%s ago)", n, n == 1 ? "" : "s");
    }
    else {
        n = between / (3600 * 24 * 7) + 1;
        printf("(%d week%s ago)", n, n == 1 ? "" : "s");
    }

    /* sscanf(buf, "%s\n%*s", p -> cm_message); */
    sscanf(p -> cm_message, "%s\n%*s", buf);
    buf[strlen(buf)] = '\0';
    printf(" %s - %s", buf, p -> cm_author);
    if (p -> cm_current_flag) {
        printf(" (HEAD)");
    }
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

static char *commit_message(void)
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

    /* _FIXME_: haven't free the Editor. */
    /* free(Editor); */
    unlink(template);
    return parse_result;
}

static time_t commit_date() {
    return time(NULL);
}

static uLong commit_version() {
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

static uLong current_version(DBM *dbm_db, struct commit_info **cm)
{
    struct commit_info *p;
    datum dbm_key, dbm_data;

    for (dbm_key = dbm_firstkey(dbm_db);
         dbm_key.dptr;
         dbm_key = dbm_nextkey(dbm_db))
        {
            dbm_data = dbm_fetch(dbm_db, dbm_key);
            if (dbm_data.dptr) {
                p = (struct commit_info *)dbm_data.dptr;
                if (p->cm_current_flag) {
                    *cm = p;
                    return p->cm_version;
                }
            }
        }
    return 0;
}

static void store_record(void)
{
    int n;
    struct commit_info comm, *p, *pcv;
    /* uLong current_version = 0; */
    char *s;

    datum dbm_key, dbm_data;
    
    DBM *dbm_db = dbm_open(DB_FILE, O_RDWR|O_CREAT, 0644);
    if (!dbm_db) {
        err_quit("%s", gdbm_strerror(gdbm_errno));
    }

    /* Insert the information to the db. */

    while (1) {
        p = &comm;
        printf("input your name: ");

        s = fgets(p->cm_author, sizeof(p->cm_author), stdin);
        if (s != NULL && strlen(p->cm_author) > 0) {
            p->cm_author[strlen(p->cm_author) - 1] = '\0';
        }

        if (s == NULL) {
            putchar('\n');
            break;
        }
        s = commit_message();
        if (s == NULL) {
            err_quit("\nFailed to parse commit message.");
        }
        memset(p->cm_message, '\0', sizeof(p->cm_message));
        strncpy(p->cm_message, s, sizeof(p->cm_message) - 1);
        free(s);
        p->cm_date = commit_date();
        p->cm_version = commit_version();
        pcv = NULL;
        p->cm_pversion = current_version(dbm_db, &pcv);
        p->cm_current_flag = 1;

        /* Clean the previous version info. */
        if (pcv) {
            pcv->cm_current_flag = 0;
            dbm_key.dptr = (void *)&pcv->cm_version;
            dbm_key.dsize = sizeof(pcv->cm_version);
            dbm_data.dptr = (void *)pcv;
            dbm_data.dsize = sizeof(struct commit_info);
            n = dbm_store(dbm_db, dbm_key, dbm_data, DBM_REPLACE);
            if (n != 0) {
                err_quit(gdbm_strerror(gdbm_errno));
            }
        }

        /* Insert new commit. */
        dbm_key.dptr = (void *)&p->cm_version;
        dbm_key.dsize = sizeof(p->cm_version);
        dbm_data.dptr = (void *)p;
        dbm_data.dsize = sizeof(struct commit_info);
        n = dbm_store(dbm_db, dbm_key, dbm_data, DBM_REPLACE);
        if (n != 0) {
            err_quit(gdbm_strerror(gdbm_errno));
        }
    }

    KTree *tree = malloc(sizeof(KTree));
    if (tree == NULL) {
        err_sys("malloc");
    }
    ktree_init(tree, free, print_comm);
    tree -> kt_compare = compare_comm;

    retrieve_data(dbm_db, tree, NULL, 0L);

    ktree_print2d(tree, tree->kt_root, " ");

}

void retrieve_data(DBM *dbm_db, KTree *tree, KTreeNode *node, uLong pv)
{
    /* Retrieving data & building the tree. */
    datum dbm_key, dbm_data;
    KTreeNode *p;
    struct commit_info *data;

    /* printf("%lx\n", pv); */

    for (dbm_key = dbm_firstkey(dbm_db);
         dbm_key.dptr;
         dbm_key = dbm_nextkey(dbm_db))
        {
            dbm_data = dbm_fetch(dbm_db, dbm_key);
            if (dbm_data.dptr) {
                data = (struct commit_info *)dbm_data.dptr;
                if (data -> cm_pversion == pv) {
                    printf("pv = %lx\n", pv);
                    printf("data -> cm_version = %lx\n", data -> cm_version);
                    printf("data -> cm_pversion = %lx\n", data -> cm_pversion);
                    ktree_ins_child(tree, node, data);
                }
            }
        }


    if (node == NULL) {
        /* printf("%p\n", node); */
        /* node = ktree_root(tree); */
        /* printf("%p\n", node); */
        p = ktree_root(tree);
        data = (struct commit_info *)p -> ktn_data;
        retrieve_data(dbm_db, tree, p, data -> cm_version);
    }
    else {
        

    for (p = node -> ktn_first_child;
         p;
         p = p -> ktn_next_sibling)
        {
            /* printf("xxx hello\n"); */
            data = (struct commit_info *)p -> ktn_data;
            retrieve_data(dbm_db, tree, p, data -> cm_version);
        }
    }

    return;
}

int main(int argc, char *argv[])
{

    store_record();
    return 0;
}
