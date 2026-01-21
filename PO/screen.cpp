#include <ncurses.h>
#include "screen.h"

void init_screen()
{
    initscr();
    cbreak(), noecho();
    nodelay(stdscr, FALSE);
    nonl();
    leaveok(stdscr, TRUE);
    intrflush(stdscr, FALSE);
    keypad(stdscr, TRUE);
    curs_set(0);
}

void done_screen()
{
    endwin();
}

void gotoyx(int y, int x)
{
    wmove(stdscr, y, x);
}

int ngetch()
{
    return wgetch(stdscr);
}

void getscreensize(int& y, int& x)
{
    getmaxyx(stdscr, y, x);
}
