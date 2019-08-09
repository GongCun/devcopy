/*
 * Copyright (C), 2019, Cun Gong (gong_cun@bocmacau.com)
 *
 * Description: Copy the AIX /dev/rhdisk in a way similar to rsync. Divide the
 * file into pieces, compare each pieces by CRC32 (check different) and MD5
 * (check same), copy the different piece to target file.
 */

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

#define STDC_WANT_LIB_EXT1 1
#define BUFLEN 0x400000L

FILE *record_out, *record_in;

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

int main(int argc, char *argv[]) {
    int fdsrc, fddst, fddlt;
    int procs = 1;
    int c, len, tmp;
    unsigned long piece_len = BUFLEN;
    char *read_src, *read_dst;
    unsigned long long total_size = 0, n, i, p;
    pid_t pid;
    off64_t offset;
    int verbose = 0;


    opterr = 0;

    while ((c = getopt(argc, argv, "f:w:o:b:s:p:v")) != EOF)
    {
        switch (c)
        {
        case 'w':
            fddlt = open64(optarg, O_WRONLY|O_CREAT|O_TUNC);
            if (fddlt < 0) {
                perror("open64 fddlt");
                exit(-1);
            }
            break;

            case 'b':
                piece_len = strtoul(optarg, NULL, 10);
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
                printf("unrecognized option: -%c", optopt);
                exit(-1);
        }
    }

    if (argc - optind != 2)
    {
        printf("%s -s size -b blocksize -p procs input output\n", argv[0]);
        exit(-1);
    }

    if (verbose)
    {
        printf("total size = %llu\n"
                "block size = %lu\n"
                "procs = %d\n"
                "source file = %s\ntarget file = %s\n",
                total_size, piece_len, procs, argv[optind], argv[optind+1]);
    }
    if (total_size == 0) {
        printf("copy size is zero\n");
        exit(-1);
    }

    read_src = malloc(piece_len);
    if (read_src == NULL) {
        printf("malloc read_src error");
        exit(-1);
    }

    read_dst = malloc(piece_len);
    if (read_dst == NULL) {
        printf("malloc read_dst error");
        exit(-1);
    }


    if (total_size % piece_len)
    {
        printf("total size must be multiple of block size\n");
        exit(-1);
    }
    n = total_size / piece_len;
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

        if (pid == 0) { /* child process */
            if (verbose)
            {
                printf("child pid = %ld\n", (long)getpid());
            }

            fdsrc = open64(argv[optind], O_RDONLY);
            if (fdsrc < 0) {
                perror("open64 src");
                exit(-1);
            }
            fddst = open64(argv[optind+1], O_RDWR);
            if (fddst < 0) {
                perror("open64 dst");
                exit(-1);
            }

            offset = n/procs * p * piece_len;
            if (lseek64(fdsrc, offset, SEEK_SET) < 0) {
                printf("lseek64 fdsrc");
                exit(-1);
            }
            if (lseek64(fddst, offset, SEEK_SET) < 0) {
                printf("lseek64 fddst");
                exit(-1);
            }
            for (i = 0; i < n/procs; i++) {
                len = read(fdsrc, read_src, piece_len);
                if (len < 0) {
                    perror("read src");
                    exit(-1);
                }
                if (len == 0)
                    break;

                if ((tmp = read(fddst, read_dst, len)) < 0) {
                    perror("read dst");
                    exit(-1);
                }
                if (tmp != len) {
                    errno = ENOSPC;
                    perror("read dst");
                    exit(-1);
                }

                if (memcmp(read_src, read_dst, len)) {
                    if (verbose)
                    {
                        fprintf(stderr, "%llu\n", i + n/procs * p);
                    }
                    docopy(fddst, read_src, len);
                }
            }

            exit(0);
        }
        /* parent process continue */
    }

    while (wait(NULL) > 0)
        ;

    free(read_src);
    free(read_dst);

    return 0;
}
