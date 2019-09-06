#include "devcopy.h"
#include "error.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

void help(void)
{
    printf("devcopy-delta [target] [delta-file] ...\n");
    exit(-1);
}

int fdout;

int main(int argc, char *argv[]) {
    FILE         *ffchg;
    size_t        readlen;
    int           i, block_size = BUFLEN;
    struct slice  slice;

    if (argc < 3)
    {
        help();
    }

    if (access(argv[1], W_OK) < 0)
    {
        perror("access");
        exit(-1);
    }
    fdout = open64(argv[1], O_WRONLY);
    if (fdout < 0)
    {
        perror("open");
        exit(-1);
    }

    slice.buf = malloc(block_size);
    if (slice.buf == NULL)
    {
        perror("malloc");
        exit(-1);
    }

    for (i = 2; i < argc; i++)
    {
        fprintf(stderr, "%s\n", argv[i]);
        ffchg = fopen64(argv[i], "r");
        if (!ffchg)
        {
            perror("fopen64");
            exit(-1);
        }

        while (1)
        {
            readlen = fread(&slice.seq, sizeof(slice.seq), 1, ffchg);
            if (!readlen)
            {
                break;
            }
            if (!fread(&slice.len, sizeof(slice.len), 1, ffchg))
            {
                perror("fread");
                exit(-1);
            }
            if (fread(slice.buf, 1, slice.len, ffchg) != (size_t)slice.len)
            {
                perror("fread");
                exit(-1);
            }

            if (lseek64(fdout, block_size * slice.seq, SEEK_SET) < 0)
            {
                perror("lseek64");
                exit(-1);
            }
            if (write(fdout, slice.buf, slice.len) != slice.len)
            {
                perror("write");
                exit(-1);
            }
        }

        if (ferror(ffchg))
        {
            perror("fread");
            exit(-1);
        }
        fclose(ffchg);
    }

    return 0;
}
