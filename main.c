#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

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

typedef enum RegisterEncoding {
  ax = 0b000,
  cx = 0b001,
  dx = 0b010,
  bx = 0b011,
  sp = 0b100,
  bp = 0b101,
  si = 0b011,
  di = 0b111,
} RegisterEncoding;

uint32_t calculate_address(uint16_t segment, uint16_t offset) {
  return segment * 16 + offset;
}

int main(int argc, char *argv[]) {
  assert(calculate_address(0x08F1, 0x0100) == 0x09010);

  char *ram = calloc(0xFFFFF, sizeof(char));
  RegisterState *registerState = calloc(1, sizeof(RegisterState));

  *ram = 0b01000101;

  if (*ram & 0b01000000 > 0) { // inc r16
    char reg = *ram & 0b00000111;
    *((char *)registerState + reg * 2) += 1;
  }

  assert(registerState->bp == 1);

  return 0;
}
