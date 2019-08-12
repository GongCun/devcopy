#include "devcopy.h"
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
    FILE *ffchg;
    size_t readlen;
    int i, block_size = BUFLEN;
    struct record record;

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

    record.buf = malloc(block_size);
    if (record.buf == NULL)
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
            readlen = fread(&record.seq, sizeof(record.seq), 1, ffchg);
            if (!readlen)
            {
                break;
            }
            if (!fread(&record.len, sizeof(record.len), 1, ffchg))
            {
                perror("fread");
                exit(-1);
            }
            if (fread(record.buf, 1, record.len, ffchg) != record.len)
            {
                perror("fread");
                exit(-1);
            }

            if (lseek64(fdout, block_size * record.seq, SEEK_SET) < 0)
            {
                perror("lseek64");
                exit(-1);
            }
            if (write(fdout, record.buf, record.len) != record.len)
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
