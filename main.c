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

typedef enum RegisterEncoding8 {
  al = 0b000,
  cl = 0b001,
  dl = 0b010,
  bl = 0b011,
  ah = 0b100,
  ch = 0b101,
  dh = 0b110,
  bh = 0b111,
} RegisterEncoding8;

void add_8bit_by_register_encoding(RegisterState *state, uint8_t encoding, int value) {
  switch (encoding) {
    case al:
      state->ax += value;
      break;
    case cl:
      state->cx += value;
      break;
    case dl:
      state->dx += value;
      break;
    case bl:
      state->bx += value;
      break;
    case ah:
      state->ax = state->ax + (value << 8);
      break;
    case ch:
      state->cx = state->cx + (value << 8);
      break;
    case dh:
      state->dx = state->dx + (value << 8);
      break;
    case bh:
      state->bx = state->bx + (value << 8);
      break;
  }
}

uint8_t get_8bit_by_register_encoding(RegisterState *state, uint8_t encoding) {
  switch (encoding) {
    case al:
      return (uint8_t)state->ax;
    case cl:
      return (uint8_t)state->cx;
    case dl:
      return (uint8_t)state->dx;
    case bl:
      return (uint8_t)state->bx;
    case ah:
      return state->ax >> 8;
    case ch:
      return state->cx >> 8;
    case dh:
      return state->dx >> 8;
    case bh:
      return state->bx >> 8;
  }
}

typedef enum SegmentOverridePrefix {
  so_es = 0b00,
  so_cs = 0b01,
  so_ss = 0b10,
  so_ds = 0b11,
} SegmentOverridePrefix;

uint16_t get_segment_by_sop(RegisterState *state, uint8_t sop) {
  switch (sop) {
    case so_es:
      return state->es;
    case so_cs:
      return state->cs;
    case so_ss:
      return state->ss;
    case so_ds:
      return state->ds;
  }
}

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
  uint8_t mid;
  uint8_t rm;
} ModRM;

ModRM *makeModRM(uint8_t value) {
  ModRM *result = calloc(1, sizeof(ModRM));
  result->mod = (value & 0b11000000) >> 6;
  result->mid = (value & 0b00111000) >> 3;
  result->rm = value & 0b00000111;
  return result;
}

uint32_t calculate_address(uint16_t segment, uint16_t offset) {
  return segment * 16 + offset;
}

// Just cast instead?
uint16_t sign_extend(uint8_t value) {
  bool sign = (value & 0b10000000) > 0;
  return sign ? 0b1111111111111111 & (uint16_t)value : 0b0000000011111111 & (uint16_t)value;
}

int main(int argc, char *argv[]) {
  assert(calculate_address(0x08F1, 0x0100) == 0x09010);

  uint8_t *ram = calloc(0xFFFFF, sizeof(uint8_t));
  RegisterState *registerState = calloc(1, sizeof(RegisterState));
  registerState->ip = 0x0000;

  // TODO: Fix these instruction comments
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
  // es incw 0x5c(si)
  *(ram + 6) = 0b00100110;
  *(ram + 7) = 0b11111111;
  *(ram + 8) = 0b01000100;
  *(ram + 9) = 0b01011100;
  registerState->es = 0b0000000011110000;
  registerState->si = 0b1010000010000110;
  // incb 0x5af0
  *(ram + 10) = 0b11111110;
  *(ram + 11) = 0b00000110;
  *(ram + 12) = 0b11110000;
  *(ram + 13) = 0b01011010;
  // add ch, bl
  *(ram + 14) = 0b00000010;
  *(ram + 15) = 0b11101011;
  registerState->bx = 5;
  // mov di, 0xf00f
  *(ram + 16) = 0b10111111;
  *(ram + 17) = 0b00001111;
  *(ram + 18) = 0b11110000;
  // movw [di], 0xf00f // TODO: Wrong bit pattern?
  *(ram + 19) = 0b11000111;
  *(ram + 20) = 0b00000111;
  *(ram + 21) = 0b00001111;
  *(ram + 22) = 0b11110000;

  while (true) {
    uint8_t curr_insn = *(ram + registerState->ip);
    uint8_t curr_seg = 0b11;
    bool is_segment_override_prefix = (curr_insn & 0b11100111) == 0b00100110;
    if (is_segment_override_prefix) {
      curr_seg = (curr_insn & 0b00011000) >> 3;
      registerState->ip += 1;
      curr_insn = *(ram + registerState->ip);
    }

    if ((curr_insn & 0b11111000) == 0b01000000) { // inc r16
      uint8_t reg = curr_insn & 0b00000111;
      *((uint16_t *)registerState + reg) += 1;
      registerState->ip += 1;
    } else if ((curr_insn & 0b11111111) == 0b11111110) {
      // TODO: Flags
      ModRM *modRM = makeModRM(*(ram + registerState->ip + 1));
      if (modRM->mid == 0b000) { // inc r/m8
        if (modRM->mod == MOD_REGISTER)
          add_8bit_by_register_encoding(registerState, modRM->rm, 1);
        else if (modRM->mod == MOD_REGISTER_INDIRECT && modRM->rm == bp_na) { // direct memory addressing
          uint16_t segment = get_segment_by_sop(registerState, curr_seg);
          uint16_t offset = (((uint16_t)*(ram + registerState->ip + 3)) << 8) | (uint16_t)(unsigned char)*(ram + registerState->ip + 2);
          ram[calculate_address(segment, offset)] += 1;
          registerState->ip += 2;
        }
      }
      registerState->ip += 2;
    } else if ((curr_insn & 0b11111111) == 0b11111111) {
      // TODO: Flags
      ModRM *modRM = makeModRM(*(ram + registerState->ip + 1));
      if (modRM->mid == 0b000) { // inc r/m16
        if (modRM->mod == MOD_REGISTER) {
          *((uint8_t *)registerState + modRM->rm * 2) += 1;
          registerState->ip += 2;
        } else if (modRM->mod == MOD_ONE_BYTE_DISPLACEMENT) {
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
              uint16_t segment = get_segment_by_sop(registerState, curr_seg);
              uint16_t offset = sign_extend(*(ram + registerState->ip + 2)) + registerState->si;
              ram[calculate_address(segment, offset)] += 1;
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
      }
    } else if (((curr_insn & 0b11110000) >> 4) == 0b00001011) {
      bool word = ((curr_insn & 0b00001000) >> 3) == 1;
      uint8_t reg = curr_insn & 0b00000111;
      if (word) { // mov r16, imm16
        uint16_t data = (((uint16_t)*(ram + registerState->ip + 2)) << 8) | (uint16_t)(unsigned char)*(ram + registerState->ip + 1);
        *((uint16_t *)registerState + reg) += data;
        registerState->ip += 3;
      } else { // mov r8, imm8
        registerState->ip += 2;
      }
    } else if ((curr_insn & 0b11111110) == 0b11000110) {
      bool word = curr_insn & 0b00000001;
      if (word) { // mov [r/m16] imm16
        ModRM *modRM = makeModRM(*(ram + registerState->ip + 1));
        uint16_t data = (((uint16_t)*(ram + registerState->ip + 3)) << 8) | (uint16_t)(unsigned char)*(ram + registerState->ip + 2);
        uint16_t segment = get_segment_by_sop(registerState, curr_seg);
        uint16_t offset = *((uint16_t *)registerState + modRM->rm);
        ram[calculate_address(segment, offset)] += data;
        ram[calculate_address(segment, offset) + 1] += data >> 8;
        registerState->ip += 4;
      } else { // mov [r/m16] imm8 // TODO: [r/m8]?
        ModRM *modRM = makeModRM(*(ram + registerState->ip + 1));
        registerState->ip += 3;
      }
    } else if (registerState->ip >= 0xFFFE) {
      break;
    } else if (curr_insn == 0) { // add r/m8, r8
      registerState->ip += 2;
    } else if (curr_insn == 1) { // add r/m16, r16
      registerState->ip += 2;
    } else if (curr_insn == 2) { // add r8, r/m8
      ModRM *modRM = makeModRM(*(ram + registerState->ip + 1));
      if (modRM->mod == MOD_REGISTER) {
        uint8_t dest = modRM->mid;
        uint8_t src = modRM->rm;
        add_8bit_by_register_encoding(registerState, dest, get_8bit_by_register_encoding(registerState, src));
      }
      registerState->ip += 2;
    } else if (curr_insn == 3) { // add r16, r/m16
      registerState->ip += 2;
    }
  }

  assert(registerState->bp == 1);
  //assert(registerState->cx == 1);
  assert(ram[0b11111010111111100010] == 1);
  assert(ram[0b00001010111111100010] == 1);
  assert(ram[calculate_address(registerState->ds, 0x5af0)] == 1);
  assert(registerState->cx == (5 << 8) + 1);
  assert(registerState->di == 0xf00f);
  assert(ram[calculate_address(0xf0f0, 0xf00f)] == 0x0f);
  assert(ram[calculate_address(0xf0f0, 0xf00f) + 1] == 0xf0);

  return 0;
}
