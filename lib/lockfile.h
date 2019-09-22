#ifndef _MY_LOCKFILE_H
#define _MY_LOCKFILE_H

#include <unistd.h>
#include <fcntl.h>

int lockfile(int fd);

#endif
