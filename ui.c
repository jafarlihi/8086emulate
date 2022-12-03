#include "ui.h"
#include "emulate.h"
#include <stdio.h>
#include <stdlib.h>
#include <curses.h>
#include <signal.h>
#include <stdlib.h>

int termx, termy, topbarx, topbary, bottombarx, bottombary;
WINDOW *topbar, *bottombar;
char *objdump;
Emulator *emulator;

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

char *get_objdump(uint8_t *content) {
  FILE *write_ptr = fopen("/tmp/8086emulate-objdump.bin", "wb");
  fwrite(content, sizeof(uint8_t), 0xFFFFF, write_ptr);

  system("objdump -D /tmp/8086emulate-objdump.bin -b binary -m i8086 > /tmp/8086emulate-objdump.0.txt");
  system("sed '0,/0:/d' /tmp/8086emulate-objdump.0.txt > /tmp/8086emulate-objdump.1.txt");
  system("sed '$d' /tmp/8086emulate-objdump.1.txt > /tmp/8086emulate-objdump.txt");

  char *buffer = NULL;
  size_t size = 0;

  FILE *fp = fopen("/tmp/8086emulate-objdump.txt", "r");

  fseek(fp, 0, SEEK_END);
  size = ftell(fp);
  rewind(fp);
  buffer = malloc((size + 1) * sizeof(*buffer));
  fread(buffer, size, 1, fp);
  buffer[size] = '\0';

  return buffer;
}

void draw_objdump(void) {
  objdump = get_objdump(emulator->ram);
  wprintw(topbar, "%s\n", objdump);
  wmove(topbar, 0, 0);
  waddch(topbar, '>');
  wrefresh(topbar);
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
  draw_objdump();
}

void init_ui(Emulator *emu) {
  emulator = emu;
  terminal_start();
  signal(SIGWINCH, resize_handler);

  int c;
  get_window_dimensions();

  draw_window(topbar, topbary, topbarx);
  draw_window(bottombar, bottombary, bottombarx);

  draw_objdump();

  while (true) {
    c = 0;
    c = getch();

    if (c == 'q')
      break;
  }

  terminal_stop();
}
