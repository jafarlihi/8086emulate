#include "emulate.h"
#include "ui.h"
#include <stdbool.h>
#include <assert.h>
#include <stdlib.h>

#ifdef TEST_8086EMULATE

int main(int argc, char *argv[]) {
  assert(calculate_address(0x08F1, 0x0100) == 0x09010);

  uint8_t *payload = calloc(0xFFFFF, sizeof(uint8_t));
  Emulator *emulator = makeEmulator(payload);

  // TODO: Fix these instruction comments
  // inc bp
  *payload = 0b01000101;
  // inc cx
  *(payload + 1) = 0b11111111;
  *(payload + 2) = 0b11000001;
  // incw 0x5c(si)
  *(payload + 3) = 0b11111111;
  *(payload + 4) = 0b01000100;
  *(payload + 5) = 0b01011100;
  emulator->state->ds = 0b1111000011110000;
  emulator->state->si = 0b1010000010000110;
  // es incw 0x5c(si)
  *(payload + 6) = 0b00100110;
  *(payload + 7) = 0b11111111;
  *(payload + 8) = 0b01000100;
  *(payload + 9) = 0b01011100;
  emulator->state->es = 0b0000000011110000;
  emulator->state->si = 0b1010000010000110;
  // incb 0x5af0
  *(payload + 10) = 0b11111110;
  *(payload + 11) = 0b00000110;
  *(payload + 12) = 0b11110000;
  *(payload + 13) = 0b01011010;
  // add ch, bl
  *(payload + 14) = 0b00000010;
  *(payload + 15) = 0b11101011;
  emulator->state->bx = 5;
  // mov di, 0xf00f
  *(payload + 16) = 0b10111111;
  *(payload + 17) = 0b00001111;
  *(payload + 18) = 0b11110000;
  // movw [di], 0xf00f // TODO: Wrong bit pattern?
  *(payload + 19) = 0b11000111;
  *(payload + 20) = 0b00000111;
  *(payload + 21) = 0b00001111;
  *(payload + 22) = 0b11110000;
  // addw [di], 0x0f
  *(payload + 23) = 0b10000011;
  *(payload + 24) = 0b00000111;
  *(payload + 25) = 0b00001111;

  change_payload(emulator, payload);

  execute(emulator, false);

  assert(emulator->state->bp == 1);
  //assert(emulator->state->cx == 1);
  assert(emulator->ram[0b11111010111111100010] == 1);
  assert(emulator->ram[0b00001010111111100010] == 1);
  assert(emulator->ram[calculate_address(emulator->state->ds, 0x5af0)] == 1);
  assert(emulator->state->cx == (5 << 8) + 1);
  assert(emulator->state->di == 0xf00f);
  assert(emulator->ram[calculate_address(0xf0f0, 0xf00f)] == 0x1e);
  assert(emulator->ram[calculate_address(0xf0f0, 0xf00f) + 1] == 0xf0);

  free(emulator->state);
  emulator->state = calloc(1, sizeof(RegisterState));
  change_payload(emulator, payload);
  init_ui(emulator);

  return 0;
}

#endif
