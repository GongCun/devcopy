#ifndef _MY_ERROR_H
#define _MY_ERROR_H

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>

void err_sys(const char *format, ...);
void err_quit(const char *format, ...);
void err_ret(const char *format, ...);
void err_msg(const char *format, ...);
void err_exit(int error, const char *format, ...);

#endif
