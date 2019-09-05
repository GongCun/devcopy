#include "error.h"
#include <stdio.h>
#include <stdlib.h>
#include <term.h>
#include <curses.h>
#include <unistd.h>
#include <signal.h>

int grow;

static void handler(int signo)
{
    attron(A_BOLD);
    mvprintw(grow, 0, "The process can't be interrupted!");
    attroff(A_BOLD);
}


int main()
{
    int i = 0;
    int row;
    int col;

    initscr();

    getmaxyx(stdscr, row, col);
    grow = row - 1;

    if (signal(SIGINT, handler) == SIG_ERR)
        err_sys("signal");
    

    while (1) {
        mvprintw(0, 0, "%d", i++);
        mvprintw(1, 0, "%d", i++);
        mvprintw(2, 0, "%d", i++);
        mvprintw(3, 0, "%d", i++);
        refresh();
        sleep(1);
    }
    
    getch();
    endwin();
    
}
