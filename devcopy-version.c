/*
 * Copyright (C), 2019, Cun Gong (gong_cun@bocmacau.com)
 *
 * Description: Save the hash, change, and backup files to the specific folder,
 * folder are named by the commit# that managed by the GDBM database. Versions
 * can go forward or backward.
 */

#include "devcopy.h"
#include "ktree.h"
#include "error.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>

int insert;
int branch;
int display;
int verbose;


static void help(const char *prog)
{
    err_quit("%s -v -i -c -l", prog);
}


{
    int c;
    char *fchg, *fhash, *frollback;

    opterr = 0;

    while ((c = getopt(argc, argv, "icvl")) != EOF)
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

    int verfd;

    if (mkdir(VERSION_DIR, DIR_MODE) < 0 &&
        errno != EEXIST)
    {
        err_sys("mkdir %s", VERSION_DIR);
    }

    verfd = open(VERSION_DIR, O_RDWR);

    if (verfd < 0)
    {
        err_sys("open %s", VERSION_DIR);
    }

    /* Create version# */

    /* Check and copy the files to the version# dir */

    struct dirent *dp;
    struct dirent *dirp;

    dp = opendir(".");
    if (dp == NULL) {
        err_sys("opendir");
    }

    while ((dirp = readdir(dp)) != NULL)
    {
        printf("%s\n", dirp->d_name)
    }
}
