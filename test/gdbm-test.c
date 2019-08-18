#include "devcopy.h"
#include <stdlib.h>
#include <stdio.h>
#include <gdbm.h>

char template[] = "/tmp/devcopy-XXXXXX";


int main()
{
    int fd;
    uLong version;
    uLong parent_version;

    if ((fd = mkstemp(template)) < 0)
        {
            perror("mkstemp");
            exit(-1);
        }

    do
    {
        /* scanf("in */
    } while ();
    
