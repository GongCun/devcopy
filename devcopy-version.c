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

int insert;
int branch;
int display;
int verbose;
char *gfname;

static int compare_commit(const void *key1, const void *key2)
{
    struct commit_info *p1, *p2;

    p1 = (struct commit_info *)key1;
    p2 = (struct commit_info *)key2;
    
    return p1 -> cm_version == p2 -> cm_version ? 1 : 0;
}

static void help(const char *prog)
{
    err_quit("%s -v -i -b -c [version] -l [file-name]", prog);
}

int main(int argc, char *argv[])
{
    int                 c, ret;
    char               *fchg, *fhash, *fbk;
    char               *fname;
    struct commit_info  commit_info;
    struct commit_info *pc;     /* point to current version */
    struct commit_info *ppc;    /* point to previous commit */
    DBM                *dbm_db;
    uLong               checkout = 0L;
    KTree              *tree;

    opterr = 0;

    while ((c = getopt(argc, argv, "ibc:vl")) != EOF)
    {
        switch (c)
        {
            case 'i':
                /* Insert a child-version after current version */
                insert = 1;
                break;

            case 'b':
                /* Create a new branch */
                branch = 1;
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

            case '?':
                help(argv[0]);
        }
    }

    if (argc - optind != 1) {
        help(argv[0]);
    }

    gfname = fname = argv[optind];

    dbm_db = dbm_open(DB_FILE, O_RDWR | O_CREAT, FILE_MODE);

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
        /* Update the current version flag, then rollback the target file to the
           specific version.
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

