#include <stdio.h>
#include <stdlib.h>
#include <curses.h>
#include <signal.h>

int termx, termy, mapwinx, mapwiny, sidebarx, sidebary, bottombarx, bottombary;

WINDOW *mapwin, *sidebar, *bottombar;

void terminal_start();
void terminal_stop();
void get_window_dimensions();
void draw_window(WINDOW*, int, int);
void resize_handler(int);

void terminal_start() {
  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  refresh();
}

void terminal_stop() {
  endwin();
}

void get_window_dimensions() {
  getmaxyx(stdscr, termy, termx);
  mapwinx = termx - (termx / 10 * 3);
  sidebarx = termx - mapwinx;
  bottombarx = termx;
  mapwiny = termy - (termy / 10 * 3);
  sidebary = mapwiny;
  bottombary = termy - mapwiny;
  mapwin = newwin(mapwiny, mapwinx, 0, 0);
  sidebar = newwin(sidebary, sidebarx, 0, mapwinx);
  bottombar = newwin(bottombary, bottombarx, mapwiny, 0);
}

void draw_window(WINDOW* win, int height, int width) {
  wborder(win, 0, 0, 0, 0, 0, 0, 0, 0);
  wrefresh(win);
}

void resize_handler(int sig) {
  terminal_stop();
  terminal_start();
  clear();
  get_window_dimensions();
  draw_window(mapwin, mapwiny, mapwinx);
  draw_window(sidebar, sidebary, sidebarx);
  draw_window(bottombar, bottombary, bottombarx);
}

int main(int argc, char *argv[]) {
  terminal_start();
  signal(SIGWINCH, resize_handler);

  int c;
  get_window_dimensions();

  while (true) {
    draw_window(mapwin, mapwiny, mapwinx);
    draw_window(sidebar, sidebary, sidebarx);
    draw_window(bottombar, bottombary, bottombarx);

    c = 0;
    c = getch();

    if (c == 'q' || c == 27 /*ESC*/)
      break;
  }

  terminal_stop();
  return 0;
}
