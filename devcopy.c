/*
 * Copyright (C), 2019, Cun Gong (gong_cun@bocmacau.com)
 *
 * Description: Copy the AIX /dev/rhdisk in a way similar to rsync. Divide the
 * file into pieces, compare each piece, and copy the different piece to
 * target file. Also save the crc32-code of each piece to a hash file for
 * future use. All generated target files and hash files are versioned.
 */

#include "devcopy.h"
#include "ktree.h"
#include "error.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <memory.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <limits.h>


int fdin, fdout, fdhash;
int verbose;
int procs = 1;
int hashflg = 0;
int chgflg = 0;
int block_size = BUFLEN; /* 4M */
unsigned long long total_size;

int docopy(int fd, char *buf, int len)
{
    /* Negative offset is possible, compare the return value with -1 */
    if (lseek64(fd, -len, SEEK_CUR) == -1) {
        return(-1);
    }
    if (write(fd, buf, len) != len) {
        return(-1);
    }
    return len;
}

void help(const char *str)
{
    printf("%s -v -c -f -s size -p procs input output\n", str);
    exit(-1);
}

int main(int argc, char *argv[]) {
    int                 c, len, tmp, p;
    char               *bufin, *bufout;
    char               *fchg, *fin, *fout, *fhash;
    unsigned long long  n, i, abs_seq;
    pid_t               pid;
    off64_t             offset;
    FILE               *ffchg;
    uLong               crc, crc0;
    struct slice        slice;


    opterr = 0;

    while ((c = getopt(argc, argv, "cfs:p:v")) != EOF)
    {
        switch (c)
        {
            case 'c':
                /* Specify this flag will generate change files (.chg.seq#).
                 * Don't specify this flag if the change rate is greater than 50%.
                 */
                chgflg = 1;
                break;
            case 'f':
                /* Write the hash code to the specific file, suffixed with the
                 * .hash.seq#, similar for delta file (.chg.seq#). */
                hashflg = 1;
                break;
            case 's':
                total_size = strtoull(optarg, NULL, 10);
                break;
            case 'p':
                procs = atoi(optarg);
                break;
            case 'v':
                verbose = 1;
                break;
            case '?':
                help(argv[0]);
        }
    }

    if (argc - optind != 2)
        help(argv[0]);

    fin = argv[optind];
    fout = argv[optind+1];

    if (verbose)
    {
        printf("total size = %llu\n"
               "block size = %d\n"
               "procs = %d\n"
               "source file = %s\ntarget file = %s\n",
               total_size, block_size, procs, fin ,fout);
    }

    if (total_size == 0) {
        err_quit("copy size can't be zero");
    }

    bufin = malloc(block_size);
    if (bufin == NULL) {
        err_sys("malloc input buffer error");
    }

    bufout = malloc(block_size);
    if (bufout == NULL) {
        err_sys("malloc output buffer error");
    }


    if (total_size % block_size)
    {
        err_quit("total size %llu is not multiple of 4M", total_size);
    }

    n = total_size / block_size;
    if (procs == 0 || n % procs != 0)
    {
        err_quit("block unit %llu must be multiple of procs", n);
    }

    printf("total_size = %llu, n = %llu, partial = %llu\n", total_size, n, n/procs);
    fflush(stdout);

    if (access(fout, F_OK) < 0)
    {
        if (errno != ENOENT)
        {
            err_sys("access");
        }

        fdout = creat64(fout, 0644);
        if (fdout < 0)
        {
            err_sys("create target file %s", fout);
        }

        if (ftruncate64(fdout, total_size) < 0)
        {
            err_sys("truncate target file to length %llu", total_size);
        }

        close(fdout);
    }

    for (p = 0; p < procs; p++)
    {
        if ((pid = fork()) < 0) {
            err_sys("fork");
        }

        if (pid == 0)
        {
            /* child process */

            if (verbose)
            {
                printf("child pid = %ld\n", (long)getpid());
            }

            fdin = open64(fin, O_RDONLY);
            if (fdin < 0) {
                err_sys("open input file %s", fin);
            }

            fdout = open64(fout, O_RDWR);
            if (fdout < 0) {
                err_sys("open output file %s", fout);
            }

            if (hashflg)
            {
                if (asprintf(&fhash, "%s.hash.%d", fout, p) < 0)
                {
                    err_sys("asprintf");
                }

                fdhash = open64(fhash, O_WRONLY|O_CREAT|O_TRUNC, 0644);
                if (fdhash < 0)
                {
                    err_sys("open %s", fhash);
                }

                crc0 = crc32(0L, Z_NULL, 0);
            }

            if (chgflg)
            {
                if (asprintf(&fchg, "%s.chg.%d", fout, p) < 0)
                {
                    err_sys("asprintf");
                }

                ffchg = fopen64(fchg, "w");
                if (ffchg == NULL)
                {
                    err_sys("open %s", fchg);
                }
            }

            offset = n / procs * p * block_size;

            if (lseek64(fdin, offset, SEEK_SET) == -1) {
                err_sys("lseek64 input file %s", fin);
            }

            if (lseek64(fdout, offset, SEEK_SET) == -1) {
                err_sys("lseek64 output file %s", fout);
            }

            for (i = 0; i < n / procs; i++) {
                len = read(fdin, bufin, block_size);
                if (len < 0) {
                    err_sys("read input");
                }

                if (len == 0)
                    break;

                if ((tmp = read(fdout, bufout, len)) < 0) {
                    err_sys("read output");
                }

                if (tmp != len) {
                    err_exit(ENOSPC, "read output");
                }

                /* write the crc code as index */
                if (hashflg)
                {
                    crc = crc32(crc0, (const Bytef *)bufin, len);
                    if (write(fdhash, &crc, ULONG_LEN) != ULONG_LEN)
                    {
                        err_sys("write %s", fhash);
                    }
                }

                if (memcmp(bufin, bufout, len)) {
                    abs_seq = i + n / procs * p;
                    if (verbose)
                    {
                        fprintf(stderr, "%llu\n", abs_seq);
                    }
                    docopy(fdout, bufin, len);
                    
                    if (chgflg)
                    {
                        slice.seq = abs_seq;
                        slice.len = len;
                        slice.buf = bufin;

                        if (!fwrite(&slice.seq, sizeof(slice.seq), 1, ffchg))
                        {
                            err_sys("fwrite seq to %s", fchg);
                        }

                        if (!fwrite(&slice.len, sizeof(slice.len), 1, ffchg))
                        {
                            err_sys("fwrite len to %s", fchg);
                        }

                        if (!fwrite(slice.buf, slice.len, 1, ffchg))
                        {
                            err_sys("fwrite buf to %s", fchg);
                        }
                    }
                }
            }

            /* Exit and release resources */
            if (hashflg)
            {
                if (close(fdhash) < 0)
                {
                    err_sys("close hash file");
                }
                free(fhash);
            }

            if (chgflg)
            {
                if (fclose(ffchg) == EOF)
                {
                    err_sys("fclose change file");
                }
                free(fchg);
            }

            exit(0);
        } /* end of child process */
        /* parent process continue */
    }

    while (wait(NULL) > 0)
        ;


    free(bufin);
    free(bufout);

    return 0;
}
