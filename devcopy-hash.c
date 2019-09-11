#include "devcopy.h"
#include "error.h"
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
#include <sys/mman.h>
#include <term.h>
#include <curses.h>
#include <time.h>

int fdin, fdout, fdhash;
int verbose;
int chgflg;
int showflg;
int bkflg;
int procs = 1;
int block_size = BUFLEN; /* 4M */
unsigned long long total_size;

static int isrunning(unsigned long long *progress,
                     int procs,
                     unsigned long long partial)
{
    for (int i = 0; i < procs; i++)
    {
        if (progress[i] != partial)
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

static void dofwrite(FILE *file, struct slice slice)
{
    if (!fwrite(&slice.seq, sizeof(slice.seq), 1, file))
    {
        perror("fwrite");
        exit(-1);
    }

    if (!fwrite(&slice.len, sizeof(slice.len), 1, file))
    {
        perror("fwrite");
        exit(-1);
    }

    if (!fwrite(slice.buf, slice.len, 1, file))
    {
        perror("fwrite");
        exit(-1);
    }

}

static void help(const char *str)
{
    printf("%s -P -v -b -c -f [hash-file] -s size -p procs input output\n", str);
    exit(-1);
}

int main(int argc, char *argv[]) {
    int                 c, len, p, status, ret, signum;
    char               *bufin, *bufout = NULL;
    char               *fchg, *fin, *fout, *fhash = NULL, *fbk;
    unsigned long long  n, i, sentry, abs_seq, partial;
    pid_t               pid;
    off64_t             offset;
    uLong               crc0, crc1, crc2;
    FILE               *ffchg, *ffbk;
    struct slice        slice;
    unsigned long long *progress;
    time_t              daytime;


    opterr = 0;

    while ((c = getopt(argc, argv, "Pbcf:s:p:v")) != EOF)
    {
        switch (c)
        {
            case 'P':
                showflg = 1;

                if (freopen(TRACE_FILE, "w+", stderr) == NULL)
                {
                    err_sys("freopen %s", TRACE_FILE);
                }

                break;

            case 'b':
                bkflg = 1;

                bufout = malloc(block_size);
                if (bufout == NULL)
                {
                    err_sys("malloc");
                }

                break;
                
            case 'c':
                chgflg = 1;
                break;

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
        err_sys("malloc bufin error");
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

    partial = n / procs;

    printf("total_size = %llu, n = %llu, partial = %llu\n", total_size, n, partial);
    fflush(stdout);

    /*
     * Share copy progress between child and parent process.
     */
    progress = mmap(0,
                    sizeof(unsigned long long) * procs,
                    PROT_READ | PROT_WRITE,
                    MAP_SHARED | MAP_ANONYMOUS,
                    -1,
                    0);

    if (progress == MAP_FAILED)
    {
        err_sys("mmap");
    }

    daytime = time(NULL); /* save the begin time */

    for (p = 0; p < procs; p++)
    {
        if ((pid = fork()) < 0) {
            perror("fork");
            exit(-1);
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
                err_sys("open64 file %s", fin);
            }

            fdout = open64(fout, O_RDWR);
            if (fdout < 0) {
                err_sys("open64 file %s", fout);
            }

            fdhash = open64(fhash, O_RDWR);
            if (fdhash < 0)
            {
                err_sys("open64 file %s", fhash);
            }

            if (chgflg)
            {
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
                    err_sys("fopen64 %s", fbk);
                }
            }

            crc0 = crc32(0L, Z_NULL, 0);

            offset = partial * p * block_size;

            if (lseek64(fdin, offset, SEEK_SET) == -1) {
                err_sys("lseek64 file %s", fin);
            }

            if (lseek64(fdout, offset, SEEK_SET) == -1) {
                err_sys("lseek64 file %s", fout);
            }

            offset = partial * p * ULONG_LEN;

            if (lseek64(fdhash, offset, SEEK_SET) == -1)
            {
                err_sys("lseek64 file %s", fhash);
            }

            for (i = 0, sentry = 0; i < partial; i++) {
                /* Write the process to the shared memory for parent process to
                   observer */
                progress[p] = i + 1;

                /* Do the copy work */
                len = read(fdin, bufin, block_size);
                if (len < 0) {
                    err_sys("read file %s", fin);
                }

                if (len == 0)
                    break;

                crc1 = crc32(crc0, (const Bytef *)bufin, len);

                if (read(fdhash, (char *)&crc2, ULONG_LEN) != ULONG_LEN)
                {
                    err_sys("read file %s", fhash);
                }

                if (crc1 != crc2)
                {
                    abs_seq = i + partial * p;
                    if (verbose)
                    {
                        /* _FIXME_ - Concurrent write have no protect. */
                        fprintf(stderr, "%llu\n", abs_seq);
                    }

                    /* Update the hash file */
                    if (lseek64(fdhash, -ULONG_LEN, SEEK_CUR) == -1)
                    {
                        err_sys("lseek64 file %s", fhash);
                    }

                    if (write(fdhash, &crc1, ULONG_LEN) != ULONG_LEN)
                    {
                        err_sys("write file %s", fhash);
                    }

                    offset = block_size * (i - sentry);
                    if (offset && lseek64(fdout, offset, SEEK_CUR) == -1)
                    {
                        err_sys("lseek64");
                    }

                    if (bkflg)
                    {
                        if (read(fdout, bufout, len) != len)
                        {
                            err_sys("read");
                        }

                        slice.seq = abs_seq;
                        slice.len = len;
                        slice.buf = bufout;
                        dofwrite(ffbk, slice);

                        /* Rewind for write */
                        if (lseek64(fdout, -len, SEEK_CUR) == -1)
                        {
                            err_sys("lseek64 backward");
                        }

                    }

                    if (write(fdout, bufin, len) != len)
                    {
                        err_sys("write");
                    }

                    sentry = i + 1;

                    if (chgflg)
                    {
                        slice.seq = abs_seq;
                        slice.len = len;
                        slice.buf = bufin;
                        dofwrite(ffchg, slice);
                    }
                } /* End of handling copy & save change block */
            } /* End of copy work */

            /* Release resource before exit. */
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
        } /* End of child process */
        /* Parent process continue */
    }

    if (showflg)
    {
        initscr();

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

        /* Update the complete progress */
        for (p = 0; p < procs; p++)
        {
            mvprintw(p, 0, "process %-2d complete: %%%.1f",
                     p, 1.0 * progress[p] / partial * 100);
            refresh();
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
