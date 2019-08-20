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

#define MYBUFLEN 1024

char template[] = "/tmp/devcopy-XXXXXX";
char *hint = "\n\n\n# Please enter a description of the submission";

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

int main()
{
    int fd;
    int len;
    int ret;
    char *Editor;
    

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

    char buf[MYBUFLEN];
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
        FILE *Commit;
        Commit = fopen(template, "r");

        if (Commit == NULL) {
            err_sys("fopen");
        }
        char *parse_result = commit_parser(Commit);
        if (parse_result == NULL) {
            err_quit("Failed to parse the temporary file (%s)", template);
        }
        printf("%s", parse_result);
        free(parse_result);
        unlink(template);
    }
    else {
        err_sys("Editor (%s) failed with exit status %d", ret);
    }
}



