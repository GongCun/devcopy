#include "devcopy.h"
#include "error.h"
#include <stdlib.h>
#include <stdio.h>
#include <gdbm.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <regex.h>
#include <time.h>
#include <fcntl.h>

#define MYBUFLEN 1024


static int chk_emp_comm(const char *str)
{
    regex_t regex;
    int i, ret;
    char errbuf[MYBUFLEN];
    char *pattern[] = {
        "^[[:space:]]*$",
        "^[[:space:]]*#"
    };

    for (i = 0;
         i < (int)(sizeof(pattern) / sizeof(pattern[0]));
         i++)
    {
        ret = regcomp(&regex, pattern[i], REG_EXTENDED);
        if (ret) {
            err_quit("Can't compile regex");
        }
        ret = regexec(&regex, str, 0, NULL, 0);
        if (!ret) {
            regfree(&regex);
            return 1; /* match */
        }
        else if (ret == REG_NOMATCH) {
            regfree(&regex);
        }
        else {
            regfree(&regex);
            regerror(ret, &regex, errbuf, sizeof(errbuf));
            err_quit("Regex match failed: %s", errbuf);
        }
    }

    return 0; /* no match */
}

static char *commit_parser(FILE *file)
{
    char *buf = NULL;
    char readin[MYBUFLEN];
    int copylen = 0, alloc = 0;

    
    while (fgets(readin, sizeof(readin), file) != NULL) {
        if (chk_emp_comm(readin)) {
            continue;
        }
        alloc = copylen + strlen(readin) + 1;
        buf = realloc(buf, alloc);
        if (buf == NULL) {
            err_sys("realloc");
        }
        strncpy(buf + copylen, readin, strlen(readin) + 1);
        copylen += strlen(readin);
    }

    return buf;
}

static char *commit_msg(void)
{
    int fd;
    int len;
    int ret;
    char *Editor;
    char buf[MYBUFLEN];
    FILE *Commit;
    char *parse_result;
    char template[] = "/tmp/devcopy-XXXXXX";
    char *hint = "\n\n\n# Please enter a description of the submission";
    

    if ((fd = mkstemp(template)) < 0) {
        err_sys("mkstemp");
    }

    len = strlen(hint);
    if (write(fd, hint, len) != len) {
        err_sys("write");
    }
    close(fd);

    if (!(Editor = getenv("EDITOR"))) {
        Editor = strdup("/usr/bin/vi");
    }

    if (strlen(Editor) + strlen(template) + 2 > sizeof(buf)) {
        err_quit("Buffer too short");
    }
    sprintf(buf, "%s %s", Editor, template);
    ret = system(buf);
    struct stat statbuf;
    if ((ret != -1 && (ret >> 8) == 0) || (ret >> 8) == 1) {
        /*
         * system() fork new process succeed and return code is zero, or run
         * background is okay.
         */
        if (stat(template, &statbuf) < 0) {
            err_sys("Can't stat temporary file (%s)", template);
        }
        if (statbuf.st_size == 0) {
            err_quit("Zero length template file (%s)", template);
        }

        /*
         * Reopen the temporary file and extract the content to description
         * buffer.
         */
        Commit = fopen(template, "r");

        if (Commit == NULL) {
            err_sys("fopen");
        }
        parse_result = commit_parser(Commit);
    }
    else {
        err_sys("Editor (%s) failed with exit status %d", ret);
    }

    free(Editor);
    unlink(template);
    return parse_result;
}

static time_t commit_date() {
    return time(NULL);
}

static uLong commit_ver() {
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

static void input_record(void)
{
    int n;

    struct commit commit;
    uLong current_ver = 0;

    while (1) {
        printf("input your name: ");
        n = scanf("%s\n", commit.comm_author);
        if (n == EOF) {
            break;
        }
        commit.comm_msg = commit_msg();
        if (commit.comm_msg == NULL) {
            err_quit("\nFailed to parse commit message.");
        }
        commit.comm_date = commit_date();
        commit.comm_ver = commit_ver();
        commit.comm_pver = current_ver;
        current_ver = commit.comm_ver;
        printf("author: %s\n"
               "date: %s\n"
               "version: %lx\n"
               "parent version: %lx\n"
               "commit message\n"
               "==============\n%s\n",
               commit.comm_author, ctime(&commit.comm_date),
               commit.comm_ver, commit.comm_pver, commit.comm_msg);
        fflush(stdout);
    }
}

int main(int argc, char *argv[])
{

    input_record();
    return 0;
}
