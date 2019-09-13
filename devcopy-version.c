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
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <regex.h>

int insert;
int branch;
int display;
int verbose;

static int match_name(const char *filename, const char *str)
{
    regex_t regex;
    int     ret;
    char    errbuf[MYBUFLEN];
    char    buf1[PATH_MAX];
    char    buf2[PATH_MAX];
    char    *pattern[] = {buf1, buf2};

    snprintf(buf1, sizeof(buf1), "%s.(hash|chg|bk).[0-9]+", str);
    snprintf(buf2, sizeof(buf2), "%s.hash", str);

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

static uLong commit_version(void)
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


static void help(const char *prog)
{
    err_quit("%s -v -i -c -l file-name", prog);
}

int main(int argc, char *argv[])
{
    int c;
    char *fchg, *fhash, *fbk;
    char *fname;

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

    if (argc - optind != 1) {
        help(argv[0]);
    }

    fname = argv[optind];

    int verfd;
    DIR *verdp, *wdp;

    if (mkdir(VERSION_DIR, DIR_MODE) < 0 &&
        errno != EEXIST)
    {
        err_sys("mkdir %s", VERSION_DIR);
    }

    verdp = opendir(VERSION_DIR);
    if (verdp == NULL) {
        err_sys("opendir %s", VERSION_DIR);
    }

    verfd = dirfd(verdp);

    if (verfd < 0)
    {
        err_sys("dirfd");
    }

    /* Create version# */
    struct commit_info commit_info, *pc;
    char *verpath;
    
    pc = &commit_info;
    pc->cm_version = commit_version();

    if (asprintf(&verpath, "%08lx", pc->cm_version) < 0) {
        err_sys("asprintf");
    }

    if (mkdirat(verfd, verpath, DIR_MODE) < 0) {
        err_sys("mkdirat");
    }

    free(verpath);
    

    /* Check and copy the files to the version# dir */

    struct dirent *dirp;

    wdp = opendir(".");
    if (wdp == NULL) {
        err_sys("opendir");
    }

    while ((dirp = readdir(wdp)) != NULL)
    {
        if (match_name(dirp->d_name, fname)) {
            printf("%s\n", dirp->d_name);
        }
    }
}
