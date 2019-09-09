/*
 * Copyright (C), 2019, Cun Gong (gong_cun@bocmacau.com)
 *
 * Description: Copy the AIX /dev/rhdisk in a way similar to rsync. Divide the
 * file into pieces, compare each piece, and copy the different piece to
 * target file. Also save the crc32-code of each piece to a hash file for
 * future use.
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
#include <sys/mman.h>
#include <term.h>
#include <curses.h>
#include <time.h>

#define TRACE_FILE "./devcopy.trc"

int fdin, fdout, fdhash;
int verbose;
int procs = 1;
int hashflg = 0;
int chgflg = 0;
int bkflg = 0;
int showflg = 0;
int block_size = BUFLEN; /* 4M */
unsigned long long total_size;
int global_row;


static int isrunning(unsigned long long *progress,
                     int procs,
                     unsigned long long partial)
{
    for (int i = 0; i < procs; i++)
    {
        if (progress[i] != partial - 1)
            return 1;
    }

    return 0;
}


static int check_child_exit(int status, int *signum)
{
    int ret;

    *signum = 0;
    
    if (WIFEXITED(status))
    {
        ret = WEXITSTATUS(status);
    }
    else
    {
        ret = -1;

        if (WIFSIGNALED(status))
            *signum = WTERMSIG(status);
    }

    return ret;
}


static int docopy(int fd, char *buf, int len)
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

static void dofwrite(FILE *file, struct slice slice)
{

    if (!fwrite(&slice.seq, sizeof(slice.seq), 1, file))
    {
        err_sys("fwrite seq");
    }

    if (!fwrite(&slice.len, sizeof(slice.len), 1, file))
    {
        err_sys("fwrite len");
    }

    if (!fwrite(slice.buf, slice.len, 1, file))
    {
        err_sys("fwrite buf");
    }
}

static void help(const char *str)
{
    printf("%s -P -v -b -c -f -s size -p procs input output\n", str);
    exit(-1);
}

int main(int argc, char *argv[]) {
    int                 c, len, tmp, p, ret, status, signum;
    char               *bufin, *bufout;
    char               *fchg, *fin, *fout, *fhash, *fbk;
    unsigned long long  n, i, abs_seq, partial;
    pid_t               pid;
    off64_t             offset;
    FILE               *ffchg, *ffbk;
    uLong               crc, crc0;
    struct slice        slice;
    unsigned long long *progress;
    time_t              daytime;


    opterr = 0;

    while ((c = getopt(argc, argv, "Pbcfs:p:v")) != EOF)
    {
        switch (c)
        {
            case 'P':
                /* Show the progress with curses. In order not to mess with the
                 * screen, redirect output to trace file. */
                showflg = 1;

                if (freopen(TRACE_FILE, "w+", stderr) == NULL)
                {
                    err_sys("freopen %s", TRACE_FILE);
                }

                break;

            case 'b':
                /* Backup the change block. */
                bkflg = 1;
                break;

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

    partial = n / procs;

    printf("total_size = %llu, n = %llu, partial = %llu\n", total_size, n, partial);
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

    /*
     * Share copy progress between child and parent process.
     */
    progress = mmap(0, sizeof(unsigned long long) * procs,
                    PROT_READ | PROT_WRITE,
                    MAP_SHARED | MAP_ANONYMOUS,
                    -1, 0);

    if (progress == MAP_FAILED)
    {
        err_sys("mmap");
    }


    daytime = time(NULL);
    
    /* Fork process to copy. */
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
                fprintf(stderr, "child pid = %ld\n", (long)getpid());
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

            if (bkflg)
            {
                if (asprintf(&fbk, "%s.bk.%d", fout, p) < 0)
                {
                    err_sys("asprintf");
                }

                ffbk = fopen64(fbk, "w");
                if (ffbk == NULL)
                {
                    err_sys("open %s", fbk);
                }
            }

            offset = partial * p * block_size;

            if (lseek64(fdin, offset, SEEK_SET) == -1) {
                err_sys("lseek64 input file %s", fin);
            }

            if (lseek64(fdout, offset, SEEK_SET) == -1) {
                err_sys("lseek64 output file %s", fout);
            }

            for (i = 0; i < partial; i++) {
                /* Write the progress in the shared memory */
                progress[p] = i;

                /* Do the copy work */
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
                    abs_seq = i + partial * p;
                    if (verbose)
                    {
                        /* _FIXME_ - Concurrent write have no locks. */
                        fprintf(stderr, "%llu\n", abs_seq);
                    }

                    if (bkflg)
                    {
                        slice.seq = abs_seq;
                        slice.len = len;
                        slice.buf = bufout;
                        dofwrite(ffbk, slice);
                    }

                    docopy(fdout, bufin, len);

                    if (chgflg)
                    {
                        slice.seq = abs_seq;
                        slice.len = len;
                        slice.buf = bufin;
                        dofwrite(ffchg, slice);

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

            if (bkflg)
            {
                if (fclose(ffbk) == EOF)
                {
                    err_sys("fclose backup file");
                }
                free(fbk);
            }

            exit(0);
        } /* end of child process */
        /* parent process continue */
    }

    if (showflg)
    {
        /* Show the progress */
        int row, col;

        initscr();
        getmaxyx(stdscr, row, col);
        global_row = row - 1;

        while (isrunning(progress, procs, partial))
        {
            for (p = 0; p < procs; p++)
            {
                mvprintw(p, 0, "process %-2d complete: %%%.1f",
                         p, 1.0 * progress[p] / partial * 100);
                refresh();
            }

            sleep(1);
        }

        double elapse;
        elapse = (double)time(NULL) - daytime;
        if (elapse > 60)
        {
            elapse /= 60;
            mvprintw(procs + 1, 0, "Elapse time: %.2f min%s", elapse,
                     (elapse > 1 ? "s" : ""));
        }
        else
        {
            mvprintw(procs + 1, 0, "Elapse time: %llu second%s",
                     (unsigned long long)elapse,
                     (elapse > 1 ? "s" : ""));
        }
        

        mvprintw(procs + 3, 0, "Press any key to continue...");
        getch();
        endwin();
    }

    while ((pid = wait(&status)) > 0)
    {
        ret = check_child_exit(status, &signum);
        if (verbose)
        {
            printf("process %ld exit code = %d\n", (long)pid, ret);
        }

        if (signum)
        {
            printf("process %ld abnormal termination, signal number = %d\n",
                   (long)pid, signum);
        }
    }


    free(bufin);
    free(bufout);
    if (munmap(0, sizeof(unsigned long long) * procs) < 0)
    {
        err_sys("munmap");
    }

    return 0;
}

