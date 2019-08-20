#include "error.h"

int main()
{
    errno = EAGAIN;

    /* err_msg(NULL); */
    /* err_msg(""); */
    /* err_sys(""); */
    /* err_sys(NULL); */
    err_msg("");
    char *temp = "/tmp/xxx";
    err_ret("can't find file %s", temp);
    errno = 0;
    err_ret("");
    err_msg("test message");
    err_ret("test return");
    /* err_quit("test quit"); */
    err_sys("test error");
    /* err_exit(EEXIST, "test exit"); */
}
