#include "devcopy.h"
#include "error.h"


uLong current_version(DBM *dbm_db, struct commit_info **cm)
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
                /*
                 *  _Attention_ : dbm_fetch() will change the pointer, so
                 *  memory needs to be allocated if it's to be used in the
                 *  future.
                 */
                *cm = malloc(sizeof(struct commit_info));
                if (*cm == NULL) {
                    err_sys("malloc");
                }
                memcpy(*cm, p, dbm_data.dsize);
                return p->cm_version;
            } /* Found the current version and return. */
        }
    }
    return 0;
}

uLong commit_version(void)
{
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

int match_name(const char *filename, const char *str)
{
    regex_t regex;
    int     ret;
    char    errbuf[MYBUFLEN];
    char    buf1[PATH_MAX];
    char    buf2[PATH_MAX];
    char    *pattern[] = {buf1, buf2};

    snprintf(buf1, sizeof(buf1), "%s.(hash|chg|bk).[0-9]+$", str);
    snprintf(buf2, sizeof(buf2), "%s.hash$", str);

    for (size_t i = 0;
         i < sizeof(pattern) / sizeof(pattern[0]);
         i++)
    {
        ret = regcomp(&regex, pattern[i], REG_EXTENDED);

        if (ret) {
            err_quit("Can't compile regex: %s", pattern);
        }

        ret = regexec(&regex, filename, 0, NULL, 0);
        regfree(&regex);

        switch (ret) {
            case 0:
                return 1;
                break;

            case REG_NOMATCH:
                break;

            default:
                regerror(ret, &regex, errbuf, sizeof(errbuf));
                err_quit("Regex match failed: %s", errbuf);
                break;
        }
    }

    return 0;
}

#if 0
static int match_empty(const char *s)
{
    regex_t regex;
    int ret;
    const char pattern[] = "^[[:space:]]*$";

    ret = regcomp(&regex, pattern, REG_EXTENDED);
    if (ret) {
        char errbuf[MYBUFLEN];
        regerror(ret, &regex, errbuf, sizeof(errbuf));
        err_quit("Can't compile regex \"%s\": %s", s, errbuf);
    }

    ret = regexec(&regex, s, 0, NULL, 0);
    regfree(&regex);

    if (ret == 0) {
        return 1; /* match */
    }

    return 0; /* REG_NOMATCH */

}
#endif


static int match_pattern(const char *s, const char *pattern)
{
    regex_t regex;
    int ret;

    ret = regcomp(&regex, pattern, REG_EXTENDED);
    if (ret) {
        char errbuf[MYBUFLEN];
        regerror(ret, &regex, errbuf, sizeof(errbuf));
        err_quit("Can't compile regex \"%s\": %s", pattern, errbuf);
    }

    ret = regexec(&regex, s, 0, NULL, 0);
    regfree(&regex);

    if (ret == 0) {
        return 1; /* match */
    }

    return 0; /* REG_NOMATCH */
}



void commit_author(struct commit_info *p)
{
    char *s = NULL;

    memset(p->cm_author, '\0', sizeof(p->cm_author));

    do {
        printf("Input username: ");
        s = fgets(p->cm_author, sizeof(p->cm_author), stdin);
        if (s != NULL && strlen(s) > 0) {
            s[strlen(s) - 1] = '\0';
        }
    } while (s == NULL || match_pattern(s, "^[[:space:]]*$"));

}

static char *commit_parser(FILE *file)
{
    char *buf = NULL;
    char readin[MYBUFLEN];
    int copylen = 0, alloc = 0;

    
    while (fgets(readin, sizeof(readin), file) != NULL) {
        if (match_pattern(readin, "^[[:space:]]*$") ||
            match_pattern(readin, "^[[:space:]]*#"))
        {
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

char *commit_message(struct commit_info *p)
{
    int fd;
    int len;
    int ret;
    char *Editor;
    char buf[MYBUFLEN];
    FILE *Commit;
    char *str;
    char template[] = "/tmp/devcopy-XXXXXX";
    char *hint = "\n\n\n# Please enter a description of the submission";
    struct stat statbuf;
    

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
        str = commit_parser(Commit);
    }
    else {
        err_sys("Editor (%s) failed with exit status %d", ret);
    }

    /* _FIXME_: haven't free the Editor. */
    /* free(Editor); */

    unlink(template);

    if (str) {
        memset(p->cm_message, '\0', sizeof(p->cm_message));
        strncpy(p->cm_message, str, sizeof(p->cm_message) - 1);
        free(str);
        return p->cm_message;
    }
    else {
        return NULL;
    }
}

void print_commit(void *data)
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
     * 1 week   = 7  days    = 7 * 24 * 3600 sec
     * 1 month  = 31 days    = 31 * 24 * 3600 sec
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
    else if (between < 3600 * 24 * 31) {
        n = between / (3600 * 24 * 7) + 1;
        printf("(%d week%s ago)", n, n == 1 ? "" : "s");
    }
    else {
        n = between / (3600 * 24 * 31) + 1;
        printf("((%d) month%s ago)", n, n == 1 ? "" : "s");
    }

    sscanf(p -> cm_message, "%[^\n]", buf);
    buf[strlen(buf)] = '\0';
    printf(" %s - %s", buf, p -> cm_author);
    if (p -> cm_current_flag) {
        printf(" (HEAD)");
    }
}

void retrieve_data(DBM *dbm_db, KTree *tree, KTreeNode *node, uLong pv)
{
    /* Retrieving data & building the tree.
     * Insert node as the child of a specific node, and its previous commit is
     * 'pv'. */

    datum dbm_key, dbm_data;
    KTreeNode *p;
    struct commit_info *data, *tmp;

    for (dbm_key = dbm_firstkey(dbm_db);
         dbm_key.dptr;
         dbm_key = dbm_nextkey(dbm_db))
    {
        dbm_data = dbm_fetch(dbm_db, dbm_key);
        if (dbm_data.dptr) {
            tmp = (struct commit_info *)dbm_data.dptr;
            if (tmp -> cm_pversion == pv) {
                data = malloc(sizeof(struct commit_info));
                if (data == NULL)
                    err_sys("malloc");
                memset(data, 0, sizeof(struct commit_info));
                memcpy(data, dbm_data.dptr, dbm_data.dsize);
                ktree_ins_child(tree, node, data);
            }
        }
    }

    if (ktree_size(tree) == 0) {
        /* No match version. */
        return;
    }

    if (node == NULL) {
        p = ktree_root(tree);
        data = (struct commit_info *)p -> ktn_data;
        retrieve_data(dbm_db, tree, p, data -> cm_version);
    }
    else {

        for (p = node -> ktn_first_child;
             p;
             p = p -> ktn_next_sibling)
        {
            data = (struct commit_info *)p -> ktn_data;
            retrieve_data(dbm_db, tree, p, data -> cm_version);
        }
    }

    return;
}

void insert_commit(DBM *dbm_db, struct commit_info *pc, const char *fname)
{
    /*
     * Insert a new commit as a child of current version.
     *   1. Fetch the current version;
     *   2. Insert new version as a branch of current version;
     *   3. Refresh the current version flag;
     *   4. Copy the specific files to the version folder.
     */

    struct commit_info *ppc;    /* point to previous commit */
    struct dirent      *dirp;
    struct stat         statbuf;
    char                verpath[PATH_MAX];
    char                buf[MYBUFLEN];
    DIR                *dp;
    datum               dbm_key, dbm_data;
    int                 ret;

    pc->cm_version = commit_version();
    if (verbose) {
        printf("Insert new version: %lx\n", pc->cm_version);
    }

    pc->cm_date = time(NULL);
    ppc = NULL;
    pc->cm_pversion = current_version(dbm_db, &ppc);
    pc->cm_current_flag = 1;

    if (stat(fname, &statbuf) < 0) {
        err_sys("stat %s", fname);
    }
    pc->cm_mtime = statbuf.st_mtime;

    commit_author(pc);
    if (!commit_message(pc)) {
        err_exit(EINVAL, "fail to parse commit message");
    }

    /* Clean the previous version info. */
    if (ppc) {
        ppc->cm_current_flag = 0;
        dbm_key.dptr = (void *)&ppc->cm_version;
        dbm_key.dsize = sizeof(ppc->cm_version);
        dbm_data.dptr = (void *)ppc;
        dbm_data.dsize = sizeof(struct commit_info);
        ret = dbm_store(dbm_db, dbm_key, dbm_data, DBM_REPLACE);
        if (ret) {
            err_quit(gdbm_strerror(gdbm_errno));
        }
        free(ppc);
    }

    /* Insert new commit. */
    dbm_key.dptr = (void *)&pc->cm_version;
    dbm_key.dsize = sizeof(pc->cm_version);
    dbm_data.dptr = (void *)pc;
    dbm_data.dsize = sizeof(struct commit_info);
    ret = dbm_store(dbm_db, dbm_key, dbm_data, DBM_REPLACE);
    if (ret) {
        err_quit(gdbm_strerror(gdbm_errno));
    }

    /* Create folder ./.devcopy/version# and save files (.hash*, .chg.*,
     * .bk.*) */

    if (mkdir(VERSION_DIR, DIR_MODE) < 0 &&
        errno != EEXIST) {
        err_sys("mkdir %s", VERSION_DIR);
    }

    snprintf(verpath, sizeof(verpath), "%s/%08lx",
             VERSION_DIR,
             pc->cm_version);

    if (mkdir(verpath, DIR_MODE) < 0) {
        err_sys("mkdir");
    }

    dp = opendir(".");
    if (dp == NULL) {
        err_sys("opendir");
    }

    while ((dirp = readdir(dp)) != NULL) {
        if (match_name(dirp->d_name, fname)) {
            if (verbose) {
                printf("Copying %s to %s\n", dirp->d_name, verpath);
            }

            snprintf(buf, sizeof(buf),
                     "/bin/cp -p ./%s %s",
                     dirp->d_name, verpath);

            /* printf("buf = %s\n", buf); */
            ret = system(buf);
            if (ret != 0) {
                err_quit("error: \"%s\"\n", buf);
            }
        }
    }

}

static void compress_path(List *list)
{
    ListElmt *p;
    KTreeNode *t;

    p = list -> head;
    while (p) {
        while (list_find(p -> next, p -> data)) {
            while (1) {
                list_rem_next(list, p, (void **)&t);
                if (t == p -> data)
                    break;
            }
        }
        p = p -> next;
    }
}

static void rollback(List *list)
{
    KTreeNode *n;
    ListElmt *save = NULL, *e;
    struct commit_info *p, *x;
    char *buf, **pbuf;
    char s[MYBUFLEN];
    int i, j;

    printf("list size = %d\n", list_size(list));

    pbuf = malloc(list_size(list) * sizeof(char *));
    for (i = 0; i < list_size(list); i++) {
        *pbuf = NULL; /* ensure free() success. */
    }

    memset(s, '\0', sizeof(s));

    /* Scan first: check files exist, save the copy command. */
    for (e = list->head, i = 0;
         e;
         e = e->next)
    {
        n = (KTreeNode *)e->data;
        p = (struct commit_info *)n->ktn_data;

        if (save) {
            if (verbose) {
                if (strlen(s) + 4 >= sizeof(s)) {
                    err_quit("out of range");
                }
                sprintf(s + strlen(s), " -> ");
            }

            buf = pbuf[i++] = malloc(MYBUFLEN);
            if (buf == NULL) {
                err_sys("malloc");
            }

            KTreeNode *tmp = (KTreeNode *)save->data;
            if (tmp->ktn_parent == n) {
                /* Roll backward. */
                x = (struct commit_info *)tmp->ktn_data;

                snprintf(buf, MYBUFLEN, "/bin/ls %s/%08lx/ | grep -q '.bk.[0-9]*$'",
                         VERSION_DIR, x->cm_version);
                printf("%s\n", buf);
                if (system(buf) != 0) {
                    err_quit("check %08lx .bk.* file failed", x->cm_version);
                }

                snprintf(buf, MYBUFLEN,
                         "/usr/local/bin/devcopy-delta ./%s %s/%08lx/*.bk.*",
                         gfname, VERSION_DIR, x->cm_version);
                /* printf("%s\n", buf); */
            }
            else {
                /* Roll forward. */
                snprintf(buf, MYBUFLEN, "/bin/ls %s/%08lx/ | grep -q '.chg.[0-9]*$'",
                         VERSION_DIR, p->cm_version);
                printf("%s\n", buf);
                if (system(buf) != 0) {
                    err_quit("check %08lx .chg.* file failed", p->cm_version);
                }

                snprintf(buf, MYBUFLEN,
                         "/usr/local/bin/devcopy-delta ./%s %s/%8lx/*.chg.*",
                         gfname, VERSION_DIR, p->cm_version);
                /* printf("%s\n", buf); */
            }
        }

        save = e;

        if (verbose) {
            if (strlen(s) + 8 >= sizeof(s)) {
                err_quit("out of range");
            }
            sprintf(s + strlen(s), "%8lx", p->cm_version);
        }
    }

    if (verbose) {
        puts(s);
        /* printf("%s\n", s); */
    }

    for (j = 0; j < i; j++) {
        /* if (system(pbuf[j]) != 0) { */
            /* err_quit("Error: %s", pbuf[j]); */
        /* } */
        /* printf("%s\n", buf); */
        puts(pbuf[j]);
        free(pbuf[j]);
    }

    free(pbuf);

}

void checkout_commit(DBM *dbm_db, uLong checkout, KTree *tree)
{
    datum dbm_key, dbm_data;
    struct commit_info *p, *v;
    int ret;
    List *list, *miss;
    KTreeNode *n1, *n2;

    /* Fetch the current version for update in the future. */
    p = NULL;
    current_version(dbm_db, &p); /* remember to release pointer at last. */

    if (p == NULL) {
        err_msg("Repository is empty, nothing to checkout.");
        return;
    }

    if (p->cm_version == checkout) {
        err_msg("%lx is the HEAD now!", checkout);
        free(p);
        return;
    }

    /* Fetch the struct from database, then search the struct from the tree */
    dbm_key.dptr = (void *)&checkout;
    dbm_key.dsize = sizeof(uLong);
    dbm_data = dbm_fetch(dbm_db, dbm_key);
    if (dbm_data.dptr) {
        v = (struct commit_info *)dbm_data.dptr;

        /* Change the current version flag. */
        v->cm_current_flag = 1;
        dbm_key.dptr = (void *)&v->cm_version;
        dbm_key.dsize = sizeof(uLong);
        dbm_data.dptr = (void *)v;
        dbm_data.dsize = sizeof(struct commit_info);
        ret = dbm_store(dbm_db, dbm_key, dbm_data, DBM_REPLACE);
        if (ret != 0) {
            err_quit(gdbm_strerror(gdbm_errno));
        }

        /* Clean previous current version flag. */
        p->cm_current_flag = 0;
        dbm_key.dptr = (void *)&p->cm_version;
        dbm_key.dsize = sizeof(uLong);
        dbm_data.dptr = (void *)p;
        dbm_data.dsize = sizeof(struct commit_info);
        ret = dbm_store(dbm_db, dbm_key, dbm_data, DBM_REPLACE);
        if (ret != 0) {
            err_quit(gdbm_strerror(gdbm_errno));
        }
    }
    else {
        err_msg("Can't find the specific version.");
        return;
    }

    /* Generate the rollback path. */
    list = malloc(sizeof(List));
    if (list == NULL) {
        err_sys("malloc list");
    }
    list_init(list, free);

    miss = malloc(sizeof(List));
    if (miss == NULL) {
        err_sys("malloc list");
    }
    list_init(miss, free);

    n1 = ktree_find(tree, ktree_root(tree), p);
    n2 = ktree_find(tree, ktree_root(tree), v);
    ret = ktree_path(tree, n1, n2, miss, list);
    if (ret) {
        compress_path(list);
        rollback(list);
    }
    else {
        err_msg("Can't find path from %lx to %lx",
                p->cm_version, v->cm_version);
    }

    free(p); /* Now we can free the pointer. */

}
