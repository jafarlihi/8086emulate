#include "ui.h"
#include "emulate.h"
#include <stdio.h>
#include <stdlib.h>
#include <curses.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

int termx, termy, topbarx, topbary, bottombarx, bottombary;
WINDOW *topbar, *bottombar;
char *objdump;
Emulator *emulator;
int step = 0;

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
  system("sed '0,/00000000/d' /tmp/8086emulate-objdump.0.txt > /tmp/8086emulate-objdump.txt");
  //system("sed '$d' /tmp/8086emulate-objdump.1.txt > /tmp/8086emulate-objdump.txt");

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
  wmove(topbar, step++, 0);
  waddch(topbar, '>');
  wrefresh(topbar);
}

void draw_help(void) {
  move(termy - 1, 2);
  printw("%s", "'q' to quit | 's' to single-step | 'r' to run");
}

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0') 

void draw_registers(void) {
  char *output = calloc(10000, sizeof(char));
  wmove(bottombar, 1, 1);
  sprintf(output, "AX: "BYTE_TO_BINARY_PATTERN" | BX: "BYTE_TO_BINARY_PATTERN" | CX: "BYTE_TO_BINARY_PATTERN" | DX: "BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(emulator->state->ax), BYTE_TO_BINARY(emulator->state->bx), BYTE_TO_BINARY(emulator->state->cx), BYTE_TO_BINARY(emulator->state->dx));
  wprintw(bottombar, "%s\n", output);
  wmove(bottombar, 2, 1);
  sprintf(output, "SP: "BYTE_TO_BINARY_PATTERN" | BP: "BYTE_TO_BINARY_PATTERN" | SI: "BYTE_TO_BINARY_PATTERN" | DI: "BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(emulator->state->sp), BYTE_TO_BINARY(emulator->state->bp), BYTE_TO_BINARY(emulator->state->si), BYTE_TO_BINARY(emulator->state->di));
  wprintw(bottombar, "%s\n", output);
  wmove(bottombar, 3, 1);
  sprintf(output, "CS: "BYTE_TO_BINARY_PATTERN" | DS: "BYTE_TO_BINARY_PATTERN" | SS: "BYTE_TO_BINARY_PATTERN" | ES: "BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(emulator->state->cs), BYTE_TO_BINARY(emulator->state->ds), BYTE_TO_BINARY(emulator->state->ss), BYTE_TO_BINARY(emulator->state->es));
  wprintw(bottombar, "%s\n", output);
  wmove(bottombar, 4, 1);
  sprintf(output, "IP: "BYTE_TO_BINARY_PATTERN" | Flags: "BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(emulator->state->ip), BYTE_TO_BINARY(emulator->state->flags));
  wprintw(bottombar, "%s\n", output);
  wborder(bottombar, 0, 0, 0, 0, 0, 0, 0, 0);
  draw_help();
  wrefresh(bottombar);
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
    draw_help();
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
  draw_registers();
  draw_help();
  refresh();
}

void single_step(void) {
  execute(emulator, true);
  resize_handler(0);
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
  draw_registers();

  while (true) {
    c = 0;
    c = getch();

    if (c == 'q')
      break;
    else if (c == 's') {
      single_step();
    }
  }

  terminal_stop();
}
