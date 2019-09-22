/*
 * Copyright (C), 2019, Cun Gong (gong_cun@bocmacau.com)
 *
 * Description: Save the hash, change, and backup files to the specific folder,
 * folder are named by the commit# that managed by the GDBM database. Versions
 * can go forward or backward.
 */

#include "devcopy.h"
#include "ktree.h"
#include "asprintf.h"
#include "error.h"
#include "lockfile.h"
#include "printf_color.h"

int insert;
int branch;
int display;
int verbose;
int showver;
char *gfname;
char VERSION_DIR[PATH_MAX];
char DB_FILE[PATH_MAX];
char LOCKFILE[PATH_MAX];


static int compare_commit(const void *key1, const void *key2)
{
    struct commit_info *p1, *p2;

    p1 = (struct commit_info *)key1;
    p2 = (struct commit_info *)key2;
    
    return p1 -> cm_version == p2 -> cm_version ? 1 : 0;
}

static void help(const char *prog)
{
    err_quit("%s -v -i -b [branch] -c [version] -s [version] -l [file-name]", prog);
}

int main(int argc, char *argv[])
{
    int                 c;
    char               *fname, *branch_name;
    struct commit_info  commit_info;
    struct commit_info *pc;
    DBM                *dbm_db;
    uLong               checkout = 0L;
    uLong               release  = 0L;
    KTree              *tree;
    struct stat         statbuf;
    struct dirent      *dirp;
    DIR                *dp;
    char                buf[MYBUFLEN];


    opterr = 0;

    while ((c = getopt(argc, argv, "ib:c:vls:")) != EOF)
    {
        switch (c)
        {
            case 'i':
                /* Insert a child-version after current version */
                insert = 1;
                break;

            case 'b':
                /* Create a new branch */
                if (strchr(optarg, '/')) {
                    err_exit(EINVAL, "branch name cannot contain the '/' character.");
                }

                branch = 1;
                branch_name = optarg;
                snprintf(VERSION_DIR, sizeof(VERSION_DIR),
                         "%s/%s", VERSION_HOME, optarg);
                break;

            case 'c':
                /* Checkout specific version. */
                checkout = strtoul(optarg, NULL, 16);
                break;

            case 'l':
                display = 1;
                break;

            case 'v':
                verbose = 1;
                break;

            case 's':
                showver = 1;
                release = strtoul(optarg, NULL, 16);
                break;

            case '?':
                help(argv[0]);
        }
    }

    if (argc - optind != 1) {
        help(argv[0]);
    }

    gfname = fname = argv[optind];

    if (stat(fname, &statbuf) < 0) {
        err_sys("stat %s", fname);
    }

    if (!(S_ISREG(statbuf.st_mode) ||
          S_ISCHR(statbuf.st_mode) ||
          S_ISBLK(statbuf.st_mode))) {
        err_quit("File %s: type is not supported.", fname);
    }

    if (!branch) {
        branch_name = "master";
        strcpy(VERSION_DIR, VERSION_STR(master));
    }

    snprintf(LOCKFILE, sizeof(LOCKFILE), "%s/%s.lock",
             VERSION_DIR, basename(fname));

    /* Try to lock file to protect process. */
    int lockfd = open(LOCKFILE, O_RDWR|O_CREAT, LOCK_MODE);
    if (lockfd < 0) {
        err_sys("can't open %s", LOCKFILE);
    }

    FILE *fflock = fdopen(lockfd, "r+");
    if (fflock == NULL) {
        err_sys("fdopen");
    }

    if (lockfile(lockfd) < 0) {
        long pid = 0;

        if (errno == EACCES || errno == EAGAIN) {
            if (fgets(buf, sizeof(buf), fflock) &&
                sscanf(buf, "%ld", &pid)) {
                err_quit("process %ld already running", pid);
            }
            else {
                err_quit("process already running");
            }
        }
        else {
            err_sys("cannot lock %s", LOCKFILE);
        }
    }

    ftruncate(lockfd, 0);
    sprintf(buf, "%ld", (long)getpid());
    write(lockfd, buf, strlen(buf) + 1);
    /* Finish Locking file. */


    /* if (verbose) { */
    /*     printf("VERSION_DIR = %s\n", VERSION_DIR); */
    /* } */

    snprintf(DB_FILE, sizeof(DB_FILE),
             "%s/vc_%s.dbm",
             VERSION_DIR, basename(fname));

    if (branch || insert || checkout) {

        if (mkdir(VERSION_HOME, DIR_MODE) < 0 &&
            errno != EEXIST) {
            err_sys("mkdir %s", VERSION_HOME);
        }

        if (mkdir(VERSION_DIR, DIR_MODE) < 0 &&
            errno != EEXIST) {
            err_sys("mkdir %s", VERSION_DIR);
        }

    }

    if (verbose) {
        if (getcwd(buf, sizeof(buf)) == NULL) {
            err_sys("getcwd");
        }

        if ((dp = opendir(VERSION_HOME)) == NULL) {
            err_sys("opendir %s", VERSION_HOME);
        }

        if (chdir(VERSION_HOME) == -1) {
            err_sys("chdir");
        }

        while ((dirp = readdir(dp)) != NULL) {
            if (strcmp(dirp->d_name, ".") == 0 ||
                strcmp(dirp->d_name, "..") == 0) {
                continue;
            }

            if (stat(dirp->d_name, &statbuf) < 0) {
                err_sys("stat %s", dirp->d_name);
            }

            if (!S_ISDIR(statbuf.st_mode)) {
                continue;
            }

            if (strcmp(dirp->d_name, branch_name) == 0) {
                printf("* ");
                printf_color(stdout, DEVCOPY_GREEN, "%s\n", branch_name);
            }
            else {
                printf("  %s\n", dirp->d_name);
            }
        }

        if (chdir(buf) == -1) {
            err_sys("chdir");
        }

        putchar('\n');
    }


    dbm_db = dbm_open(DB_FILE, O_RDWR | O_CREAT, FILE_MODE);

    if (showver && release) {
        show_release(dbm_db, release);
    }

    tree = malloc(sizeof(KTree));
    if (tree == NULL) {
        err_sys("malloc KTree");
    }
    ktree_init(tree, free, print_commit);
    tree -> kt_compare = compare_commit;
    retrieve_data(dbm_db, tree, NULL, 0L);

    if (display) {
        if (ktree_size(tree) == 0) {
            err_msg("Repository is empty.");
        }
        else {
            ktree_print2d(tree, tree->kt_root, "");
        }
        return 0;
    } /* If just display the version info., we exit here. */

    if (!dbm_db) {
        err_quit("%s", gdbm_strerror(gdbm_errno));
    }

    if (checkout) {

        /* Update the current version information, then rollback the target file
           to the specific version.
        */

        checkout_commit(dbm_db, checkout, tree);
    }

    if (insert) {
        pc = &commit_info;
        memset(pc, 0, sizeof(struct commit_info));
        insert_commit(dbm_db, pc, basename(fname));
    }

    return 0;

}

