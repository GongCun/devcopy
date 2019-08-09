/*
 * Copyright (C), 2019, Cun Gong (gong_cun@bocmacau.com)
 *
 * Description: Copy the AIX /dev/rhdisk in a way similar to rsync. Divide the
 * file into pieces, compare each pieces, copy the different piece to target
 * file.
 */

#include "devcopy.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <errno.h>
#include <memory.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <limits.h>
#include <zlib.h>


int fdin, fdout;
int verbose;
int procs = 1;
int hashflg = 0;
int block_size = BUFLEN; /* 4M */
unsigned long long total_size;

int docopy(int fd, char *buf, int len)
{
    if (lseek64(fd, -len, SEEK_CUR) < 0) {
        return(-1);
    }
    if (write(fd, buf, len) != len) {
        return(-1);
    }
    return len;
}

void help(const char *str)
{
    printf("%s -v -f -s size -p procs input output\n", str);
    exit(-1);
}

int main(int argc, char *argv[]) {
    int c, len, tmp, p;
    char *bufin, *bufout;
    char *fchg, *fin, *fout, *fhash;
    unsigned long long n, i;
    pid_t pid;
    off64_t offset;
    FILE *ffchg, *ffhash;
    uLong crc, crc0;


    opterr = 0;

    while ((c = getopt(argc, argv, "fs:p:v")) != EOF)
    {
        switch (c)
        {
            case 'f':
                /* write the hash code to the specific file, suffixed with the
                 * .seq#, same for delta file. */
                hashflg = 1;
                break;

            /* case 'b': */
            /*     block_size = strtoul(optarg, NULL, 10); */
            /*     break; */
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
        printf("copy size can't be zero\n");
        exit(-1);
    }

    bufin = malloc(block_size);
    if (bufin == NULL) {
        printf("malloc bufin error");
        exit(-1);
    }

    bufout = malloc(block_size);
    if (bufout == NULL) {
        printf("malloc bufout error");
        exit(-1);
    }


    if (total_size % block_size)
    {
        printf("total size must be multiple of 4M\n");
        exit(-1);
    }
    n = total_size / block_size;
    if (procs == 0 || n % procs != 0)
    {
        printf("block unit %llu must be multiple of procs\n", n);
        exit(-1);
    }
    printf("total_size = %llu, n = %llu, partial = %llu\n", total_size, n, n/procs);

    for (p = 0; p < procs; p++)
    {
        if ((pid = fork()) < 0) {
            perror("fork");
            exit(-1);
        }

        if (pid == 0)
        {
            /* child process */
            struct record record;

            if (verbose)
            {
                printf("child pid = %ld\n", (long)getpid());
            }

            fdin = open64(fin, O_RDONLY);
            if (fdin < 0) {
                perror("open64 src");
                exit(-1);
            }
            fdout = open64(fout, O_RDWR);
            if (fdout < 0) {
                perror("open64 dst");
                exit(-1);
            }

            if (hashflg)
            {
                if (asprintf(&fhash, "%s.hash.%d", fout, p) < 0)
                {
                    perror("asprintf");
                    exit(-1);
                }
                ffhash = fopen(fhash, "w");
                if (ffhash == NULL)
                {
                    perror("fopen");
                    exit(-1);
                }
                crc0 = crc32(0L, Z_NULL, 0);
            }

            if (asprintf(&fchg, "%s.chg.%d", fout, p) < 0)
            {
                perror("asprintf");
                exit(-1);
            }
            ffchg = fopen(fchg, "w");
            if (ffchg == NULL)
            {
                perror("fopen");
                exit(-1);
            }

            offset = n/procs * p * block_size;
            if (lseek64(fdin, offset, SEEK_SET) < 0) {
                printf("lseek64 fdin");
                exit(-1);
            }
            if (lseek64(fdout, offset, SEEK_SET) < 0) {
                printf("lseek64 fdout");
                exit(-1);
            }

            for (i = 0; i < n/procs; i++) {
                len = read(fdin, bufin, block_size);
                if (len < 0) {
                    perror("read src");
                    exit(-1);
                }
                if (len == 0)
                    break;

                if ((tmp = read(fdout, bufout, len)) < 0) {
                    perror("read dst");
                    exit(-1);
                }
                if (tmp != len) {
                    errno = ENOSPC;
                    perror("read dst");
                    exit(-1);
                }

                /* write the crc code as index */
                if (hashflg)
                {
                    crc = crc32(crc0, (const Bytef *)bufin, len);
                    if (!fwrite(&crc, sizeof(crc), 1, ffhash))
                    {
                        perror("fwrite");
                        exit(-1);
                    }
                }

                if (memcmp(bufin, bufout, len)) {
                    record.seq = i + n/procs * p;
                    record.len = len;
                    record.buf = bufin;
                    if (!fwrite(&record.seq, sizeof(record.seq), 1, ffchg))
                    {
                        perror("fwrite");
                        exit(-1);
                    }
                    if (!fwrite(&record.len, sizeof(record.len), 1, ffchg))
                    {
                        perror("fwrite");
                        exit(-1);
                    }
                    if (!fwrite(record.buf, record.len, 1, ffchg))
                    {
                        perror("fwrite");
                        exit(-1);
                    }
                    if (verbose)
                    {
                        fprintf(stderr, "%llu\n", record.seq);
                    }
                    
                    docopy(fdout, bufin, len);
                }
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
