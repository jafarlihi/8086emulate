#include "emulate.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

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
  RM rm;
} ModRM;

ModRM *make_modrm(uint8_t value) {
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

void execute(Emulator *emulator, bool single) {
  while (true) {
    uint8_t curr_insn = *(emulator->ram + emulator->state->ip);
    uint8_t curr_seg = 0b11;
    bool is_segment_override_prefix = (curr_insn & 0b11100111) == 0b00100110;
    if (is_segment_override_prefix) {
      curr_seg = (curr_insn & 0b00011000) >> 3;
      emulator->state->ip += 1;
      curr_insn = *(emulator->ram + emulator->state->ip);
    }

    if ((curr_insn & 0b11111000) == 0b01000000) { // inc r16
      uint8_t reg = curr_insn & 0b00000111;
      *((uint16_t *)emulator->state + reg) += 1;
      emulator->state->ip += 1;
    } else if ((curr_insn & 0b11111111) == 0b11111110) {
      // TODO: Flags
      ModRM *modrm = make_modrm(*(emulator->ram + emulator->state->ip + 1));
      if (modrm->mid == 0b000) { // inc r/m8
        if (modrm->mod == MOD_REGISTER)
          add_8bit_by_register_encoding(emulator->state, modrm->rm, 1);
        else if (modrm->mod == MOD_REGISTER_INDIRECT && modrm->rm == bp_na) { // direct memory addressing
          uint16_t segment = get_segment_by_sop(emulator->state, curr_seg);
          uint16_t offset = (((uint16_t)*(emulator->ram + emulator->state->ip + 3)) << 8) | (uint16_t)(unsigned char)*(emulator->ram + emulator->state->ip + 2);
          emulator->ram[calculate_address(segment, offset)] += 1;
          emulator->state->ip += 2;
        }
      }
      emulator->state->ip += 2;
    } else if ((curr_insn & 0b11111111) == 0b11111111) {
      // TODO: Flags
      ModRM *modrm = make_modrm(*(emulator->ram + emulator->state->ip + 1));
      if (modrm->mid == 0b000) { // inc r/m16
        if (modrm->mod == MOD_REGISTER) {
          *((uint8_t *)emulator->state + modrm->rm * 2) += 1;
          emulator->state->ip += 2;
        } else if (modrm->mod == MOD_ONE_BYTE_DISPLACEMENT) {
          switch (modrm->rm) {
            case bx_si:
              break;
            case bx_di:
              break;
            case bp_si:
              break;
            case bp_di:
              break;
            case na_si:
              uint16_t segment = get_segment_by_sop(emulator->state, curr_seg);
              uint16_t offset = sign_extend(*(emulator->ram + emulator->state->ip + 2)) + emulator->state->si;
              emulator->ram[calculate_address(segment, offset)] += 1;
              break;
            case na_di:
              break;
            case bp_na:
              break;
            case bx_na:
              break;
          }
          emulator->state->ip += 3;
        }
      }
    } else if (((curr_insn & 0b11110000) >> 4) == 0b00001011) {
      bool word = ((curr_insn & 0b00001000) >> 3) == 1;
      uint8_t reg = curr_insn & 0b00000111;
      if (word) { // mov r16, imm16
        uint16_t data = (((uint16_t)*(emulator->ram + emulator->state->ip + 2)) << 8) | (uint16_t)(unsigned char)*(emulator->ram + emulator->state->ip + 1);
        *((uint16_t *)emulator->state + reg) += data;
        emulator->state->ip += 3;
      } else { // mov r8, imm8
        emulator->state->ip += 2;
      }
    } else if ((curr_insn & 0b11111110) == 0b11000110) {
      bool word = curr_insn & 0b00000001;
      if (word) { // mov [r/m16] imm16
        ModRM *modrm = make_modrm(*(emulator->ram + emulator->state->ip + 1));
        uint16_t data = (((uint16_t)*(emulator->ram + emulator->state->ip + 3)) << 8) | (uint16_t)(unsigned char)*(emulator->ram + emulator->state->ip + 2);
        uint16_t segment = get_segment_by_sop(emulator->state, curr_seg);
        uint16_t offset = *((uint16_t *)emulator->state + modrm->rm);
        emulator->ram[calculate_address(segment, offset)] += data;
        emulator->ram[calculate_address(segment, offset) + 1] += data >> 8;
        emulator->state->ip += 4;
      } else { // mov [r/m16] imm8 // TODO: [r/m8]?
        ModRM *modrm = make_modrm(*(emulator->ram + emulator->state->ip + 1));
        emulator->state->ip += 3;
      }
    } else if ((curr_insn & 0b10000000) == 0b10000000) {
      bool sign = curr_insn & 0b00000010;
      bool word = curr_insn & 0b00000001;
      if (!sign && word) { // add r/m16 imm16
        ModRM *modrm = make_modrm(*(emulator->ram + emulator->state->ip + 1));
        emulator->state->ip += 4;
      } else { // add r/m16 imm8
        ModRM *modrm = make_modrm(*(emulator->ram + emulator->state->ip + 1));
        uint16_t data = sign_extend(*(emulator->ram + emulator->state->ip + 2));
        uint16_t segment = get_segment_by_sop(emulator->state, curr_seg);
        uint16_t offset = *((uint16_t *)emulator->state + modrm->rm);
        emulator->ram[calculate_address(segment, offset)] += data;
        emulator->ram[calculate_address(segment, offset) + 1] += data >> 8;
        emulator->state->ip += 3;
      }
    } else if (emulator->state->ip >= 0xFFFE) {
      break;
    } else if (curr_insn == 0) { // add r/m8, r8
      emulator->state->ip += 2;
    } else if (curr_insn == 1) { // add r/m16, r16
      emulator->state->ip += 2;
    } else if (curr_insn == 2) { // add r8, r/m8
      ModRM *modrm = make_modrm(*(emulator->ram + emulator->state->ip + 1));
      if (modrm->mod == MOD_REGISTER) {
        uint8_t dest = modrm->mid;
        uint8_t src = modrm->rm;
        add_8bit_by_register_encoding(emulator->state, dest, get_8bit_by_register_encoding(emulator->state, src));
      }
      emulator->state->ip += 2;
    } else if (curr_insn == 3) { // add r16, r/m16
      emulator->state->ip += 2;
    }
    if (single) break;
  }
}

Emulator *makeEmulator(uint8_t *payload) {
  Emulator *result = calloc(1, sizeof(Emulator));
  result->state = calloc(1, sizeof(RegisterState));
  result->ram = calloc(0xFFFFF, sizeof(uint8_t));
  memcpy(result->ram, payload, 0xFFFFF);
  return result;
}

void change_payload(Emulator *emulator, uint8_t *payload) {
  free(emulator->ram);
  emulator->ram = calloc(0xFFFFF, sizeof(uint8_t));
  memcpy(emulator->ram, payload, 0xFFFFF);
}
