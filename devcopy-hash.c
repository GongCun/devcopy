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


int fdin, fdout, fdhash;
int verbose;
int procs = 1;
int block_size = BUFLEN; /* 4M */
unsigned long long total_size;

int docopy(int fd, char *buf, int len, int seek)
{
    if (seek != 0) {
        if (lseek64(fd, seek, SEEK_CUR) < 0) {
            return(-1);
        }
    }
    if (write(fd, buf, len) != len) {
        return(-1);
    }
    return len;
}

void help(const char *str)
{
    printf("%s -v -f [hash-file] -s size -p procs input output\n", str);
    exit(-1);
}

int main(int argc, char *argv[]) {
    int c, len, p;
    char *bufin;
    char *fchg, *fin, *fout, *fhash = NULL;
    unsigned long long n, i, seek, sentry;
    pid_t pid;
    off64_t offset, offset2;
    uLong crc0, crc1, crc2;
    FILE *ffchg;


    opterr = 0;

    while ((c = getopt(argc, argv, "f:s:p:v")) != EOF)
    {
        switch (c)
        {
            case 'f':
                fhash = optarg;
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
    if (fhash == NULL)
    {
        printf("Specify a hash file\n");
        exit(-1);
    }

    fin = argv[optind];
    fout = argv[optind+1];

    if (verbose)
    {
        printf("total size = %llu\n"
               "block size = %d\n"
               "procs = %d\n"
               "hash file = %s\n"
               "source file = %s\n"
               "target file = %s\n",
               total_size, block_size, procs, fhash, fin ,fout);
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

            fdhash = open64(fhash, O_RDWR);
            if (fdhash < 0)
            {
                perror("open64");
                exit(-1);
            }

            if (asprintf(&fchg, "%s.chg.%d", fout, p) < 0)
            {
                perror("asprintf");
                exit(-1);
            }
            ffchg = fopen64(fchg, "w");
            if (ffchg == NULL)
            {
                perror("fopen64");
                exit(-1);
            }

            crc0 = crc32(0L, Z_NULL, 0);

            offset = n/procs * p * block_size;
            if (lseek64(fdin, offset, SEEK_SET) < 0) {
                printf("lseek64 fdin");
                exit(-1);
            }
            if (lseek64(fdout, offset, SEEK_SET) < 0) {
                printf("lseek64 fdout");
                exit(-1);
            }

            offset2 = n/procs * p * ULONG_LEN;
            if (lseek64(fdhash, offset2, SEEK_SET) < 0)
            {
                perror("lseek64");
                exit(-1);
            }

            for (i = 0, sentry = 0; i < n/procs; i++) {
                len = read(fdin, bufin, block_size);
                if (len < 0) {
                    perror("read src");
                    exit(-1);
                }
                if (len == 0)
                    break;

                crc1 = crc32(crc0, (const Bytef *)bufin, len);

                if (read(fdhash, (char *)&crc2, ULONG_LEN) != ULONG_LEN)
                {
                    perror("read hash");
                    exit(-1);
                }

                if (crc1 != crc2)
                {
                    if (lseek64(fdhash, -ULONG_LEN, SEEK_CUR) < 0)
                    {
                        perror("lseek64");
                        exit(-1);
                    }
                    if (write(fdhash, &crc1, ULONG_LEN) != ULONG_LEN)
                    {
                        perror("write");
                        exit(-1);
                    }

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
                    
                    seek = block_size * (i - sentry);
                    docopy(fdout, bufin, len, seek);
                    sentry = i + 1;
                }
            }

            exit(0);
        } /* end of child process */
        /* parent process continue */
    }

    while (wait(NULL) > 0)
        ;

    free(bufin);
    /* free(bufout); */

    return 0;
}
