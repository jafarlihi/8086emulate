#include <stdio.h>
#include <stdlib.h>
#include <curses.h>
#include <signal.h>

int termx, termy, topbarx, topbary, bottombarx, bottombary;

WINDOW *topbar, *bottombar;

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
  curs_set(0);
  refresh();
}

void terminal_stop() {
  endwin();
}

void get_window_dimensions() {
  getmaxyx(stdscr, termy, termx);
  topbarx = termx;
  bottombarx = termx;
  topbary = termy - (termy / 10 * 3);
  bottombary = termy - topbary;
  topbar = newwin(topbary, topbarx, 0, 0);
  bottombar = newwin(bottombary, bottombarx, topbary, 0);
}

void draw_window(WINDOW* win, int height, int width) {
  if (win == bottombar) {
    wborder(win, 0, 0, 0, 0, 0, 0, 0, 0);
    move(termy - 1, 2);
    printw("%s", "'q' to quit | 's' to single-step | 'r' to run");
  }
  wrefresh(win);
  refresh();
}

void resize_handler(int sig) {
  terminal_stop();
  terminal_start();
  clear();
  get_window_dimensions();
  draw_window(topbar, topbary, topbarx);
  draw_window(bottombar, bottombary, bottombarx);
}

int main(int argc, char *argv[]) {
  terminal_start();
  signal(SIGWINCH, resize_handler);

  int c;
  get_window_dimensions();

  draw_window(topbar, topbary, topbarx);
  draw_window(bottombar, bottombary, bottombarx);

  while (true) {
    c = 0;
    c = getch();

    if (c == 'q')
      break;
  }

  terminal_stop();
  return 0;
}
