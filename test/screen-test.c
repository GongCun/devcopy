#include "error.h"
#include <stdio.h>
#include <stdlib.h>
#include <term.h>
#include <curses.h>
#include <unistd.h>

int main()
{
    int nrows;
    int ncolumns;
    char *cursor, *el;
    

    if (setupterm(NULL, fileno(stdout), (int *)0) == ERR)
        err_sys("setupterm");

    nrows = tigetnum("lines");
    ncolumns = tigetnum("cols");

    printf("rows = %d, columns = %d\n", nrows, ncolumns);

    cursor = tigetstr("cup");
    el = tigetstr("el");
    
    int i = 0;
    
    while (1) {
        putp(tparm(cursor, 0, 0));
        putp(tparm(el));
        printf("%d", i++);
        fflush(stdout);
        sleep(1);
        
    }

    
}
