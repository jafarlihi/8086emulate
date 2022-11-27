#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
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

typedef enum Mod {
  MOD_REGISTER_INDIRECT = 0b00,
  MOD_ONE_BYTE_DISPLACEMENT = 0b01,
  MOD_TWO_BYTE_DISPLACEMENT = 0b10,
  MOD_REGISTER = 0b11,
} Mod;

typedef enum RM {
  bx_si = 0b000,
  bx_di = 0b001,
  bp_si = 0b010,
  bp_di = 0b011,
  na_si = 0b100,
  na_di = 0b101,
  bp_na = 0b110,
  bx_na = 0b111,
} RM;

typedef struct ModRM {
  Mod mod;
  uint8_t opcode;
  uint8_t rm;
} ModRM;

ModRM *makeModRM(uint32_t value) {
  ModRM *result = calloc(1, sizeof(ModRM));
  result->mod = (value & 0b11000000) >> 6;
  result->opcode = (value & 0b00111000) >> 3;
  result->rm = value & 0b00000111;
  return result;
}

uint32_t calculate_address(uint16_t segment, uint16_t offset) {
  return segment * 16 + offset;
}

uint16_t sign_extend(uint8_t value) {
  bool sign = (value & 0b10000000) > 0;
  return sign ? 0b1111111111111111 & (uint16_t)value : 0b0000000011111111 & (uint16_t)value;
}

int main(int argc, char *argv[]) {
  assert(calculate_address(0x08F1, 0x0100) == 0x09010);

  char *ram = calloc(0xFFFFF, sizeof(char));
  RegisterState *registerState = calloc(1, sizeof(RegisterState));
  registerState->ip = 0x0000;

  // inc bp
  *ram = 0b01000101;
  // inc cx
  *(ram + 1) = 0b11111111;
  *(ram + 2) = 0b11000001;
  // incw 0x5c(si)
  *(ram + 3) = 0b11111111;
  *(ram + 4) = 0b01000100;
  *(ram + 5) = 0b01011100;
  registerState->ds = 0b1111000011110000;
  registerState->si = 0b1010000010000110;

  while (true) {
    if ((*(ram + registerState->ip) & 0b11111000) == 0b01000000) { // inc r16
      char reg = *ram & 0b00000111;
      *((char *)registerState + reg * 2) += 1;
      registerState->ip += 1;
    } else if ((*(ram + registerState->ip) & 0b11111111) == 0b11111110) {
      // TODO: Flags
      ModRM *modRM = makeModRM(*(ram + registerState->ip + 1));
      if (modRM->mod == MOD_REGISTER && modRM->opcode == 0b000) // inc r/m8
        *((char *)registerState + modRM->rm * 2) += 1;
      registerState->ip += 2;
    } else if ((*(ram + registerState->ip) & 0b11111111) == 0b11111111) {
      // TODO: Flags
      ModRM *modRM = makeModRM(*(ram + registerState->ip + 1));
      if (modRM->opcode == 0b000) // inc r/m16
        if (modRM->mod == MOD_REGISTER) {
          *((char *)registerState + modRM->rm * 2) += 1;
          registerState->ip += 2;
        }
        else if (modRM->mod == MOD_ONE_BYTE_DISPLACEMENT) {
          switch (modRM->rm) {
            case bx_si:
              break;
            case bx_di:
              break;
            case bp_si:
              break;
            case bp_di:
              break;
            case na_si:
              uint16_t offset = sign_extend(*(ram + registerState->ip + 2)) + registerState->si;
              ram[calculate_address(registerState->ds, offset)] += 1;
              break;
            case na_di:
              break;
            case bp_na:
              break;
            case bx_na:
              break;
          }
          registerState->ip += 3;
        }
    } else if (*(ram + registerState->ip) == 0b00000000) {
      break;
    }
  }

  assert(registerState->bp == 1);
  assert(registerState->cx == 1);
  assert(ram[0b11111010111111100010] == 1);

  return 0;
}
