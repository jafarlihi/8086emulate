#ifndef i8086EMULATE_EMULATE_H
#define i8086EMULATE_EMULATE_H

#include <stdint.h>
#include <stdbool.h>

typedef struct RegisterState {
  uint16_t ax;
  uint16_t cx;
  uint16_t dx;
  uint16_t bx;
  uint16_t sp;
  uint16_t bp;
  uint16_t si;
  uint16_t di;
  uint16_t cs;
  uint16_t ds;
  uint16_t ss;
  uint16_t es;
  uint16_t ip;
  uint16_t flags;
} RegisterState;

typedef struct Emulator {
  RegisterState *state;
  uint8_t *ram;
} Emulator;

Emulator *makeEmulator(uint8_t *payload);
void execute(Emulator *emulator, bool single);
void change_payload(Emulator *emulator, uint8_t *payload);
uint32_t calculate_address(uint16_t segment, uint16_t offset);

#endif
