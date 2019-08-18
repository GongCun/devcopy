#include "error.h"

int main()
{
    errno = EAGAIN;

    err_msg("");
    errno = 0;
    err_ret("");
    err_msg("test message");
    err_ret("test return");
    /* err_quit("test quit"); */
    /* err_sys("test error"); */
    err_exit(EEXIST, "test exit");
}
