#include "assert.h"
#include "stdint.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#include "opcode.h"

uint32_t SimulateInstruction(uint8_t *instruction, char *data_buf) {
  uint32_t instruction_byte_count = 0;
  if ((instruction[0] & 0b11111100) == OPCODE_MOV_RM2REG) {
    printf("mov ");
    instruction_byte_count = 2;
    instruction_byte_count +=
        RegisterOrMemoryWithRegisterToEither(instruction, data_buf);
  } else if ((instruction[0] & 0b11110000) == OPCODE_MOV_IMM2REG) {
    printf("mov ");
    instruction_byte_count = 2;
    uint8_t is_wide = instruction[0] & 0b00001000;
    uint8_t reg = instruction[0] & 0b00000111;
    instruction_byte_count +=
        ImmediateToRegister(instruction, data_buf, is_wide, reg);
  } else if ((instruction[0] & 0b11111110) == OPCODE_MOV_IMM2RM) {
    printf("mov ");
    instruction_byte_count = 3;
    instruction_byte_count +=
        ImmediateToRegisterOrMemory(instruction, data_buf, false);
  } else if ((instruction[0] & 0b11111100) ==
             ((OPCODE_MOV_MEM2ACC | OPCODE_MOV_ACC2MEM) & 0b11111100)) {
    printf("mov ");
    instruction_byte_count = 2;
    instruction_byte_count +=
        AddressWithAccumulatorToEither(instruction, data_buf);
  } else if ((instruction[0] & 0b11111100) == OPCODE_ADD_RM2REG ||
             (instruction[0] & 0b11111100) == OPCODE_SUB_RM2REG ||
             (instruction[0] & 0b11111100) == OPCODE_CMP_RM2REG) {
    switch (instruction[0] & 0b00111000) {
    case 0b00000000: {
      printf("add ");
    } break;
    case 0b00101000: {
      printf("sub ");
    } break;
    case 0b00111000: {
      printf("cmp ");
    } break;
    default:
      INSTRUCTION_NOT_IMPLEMENTED();
    }
    instruction_byte_count = 2;
    instruction_byte_count +=
        RegisterOrMemoryWithRegisterToEither(instruction, data_buf);
  } else if ((instruction[0] & 0b11111100) ==
             (OPCODE_ADD_IMM2RM | OPCODE_SUB_IMM2RM | OPCODE_CMP_IMM2RM)) {
    switch (instruction[1] & 0b00111000) {
    case 0b00000000: {
      printf("add ");
    } break;
    case 0b00101000: {
      printf("sub ");
    } break;
    case 0b00111000: {
      printf("cmp ");
    } break;
    default:
      INSTRUCTION_NOT_IMPLEMENTED();
    }
    instruction_byte_count = 3;
    instruction_byte_count +=
        ImmediateToRegisterOrMemory(instruction, data_buf, true);
  } else if ((instruction[0] & 0b11111110) == OPCODE_ADD_IMM2ACC ||
             (instruction[0] & 0b11111110) == OPCODE_SUB_IMM2ACC ||
             (instruction[0] & 0b11111110) == OPCODE_CMP_IMM2ACC) {
    switch (instruction[0] & 0b00111000) {
    case 0b00000000: {
      printf("add ");
    } break;
    case 0b00101000: {
      printf("sub ");
    } break;
    case 0b00111000: {
      printf("cmp ");
    } break;
    default:
      INSTRUCTION_NOT_IMPLEMENTED();
    }
    instruction_byte_count = 2;
    instruction_byte_count += ImmediateToAccumulator(instruction, data_buf);
  } else if (instruction[0] == OPCODE_JE || instruction[0] == OPCODE_JL ||
             instruction[0] == OPCODE_JLE || instruction[0] == OPCODE_JB ||
             instruction[0] == OPCODE_JBE || instruction[0] == OPCODE_JP ||
             instruction[0] == OPCODE_JO || instruction[0] == OPCODE_JS ||
             instruction[0] == OPCODE_JNE || instruction[0] == OPCODE_JNL ||
             instruction[0] == OPCODE_JG || instruction[0] == OPCODE_JNB ||
             instruction[0] == OPCODE_JA || instruction[0] == OPCODE_JNP ||
             instruction[0] == OPCODE_JNO || instruction[0] == OPCODE_JNS ||
             instruction[0] == OPCODE_LOOP || instruction[0] == OPCODE_LOOPZ ||
             instruction[0] == OPCODE_LOOPNZ || instruction[0] == OPCODE_JCXZ) {
    switch (instruction[0]) {
    case OPCODE_JE: {
      printf("je ");
    } break;
    case OPCODE_JL: {
      printf("jl ");
    } break;
    case OPCODE_JLE: {
      printf("jle ");
    } break;
    case OPCODE_JB: {
      printf("jb ");
    } break;
    case OPCODE_JBE: {
      printf("jbe ");
    } break;
    case OPCODE_JP: {
      printf("jp ");
    } break;
    case OPCODE_JO: {
      printf("jo ");
    } break;
    case OPCODE_JS: {
      printf("js ");
    } break;
    case OPCODE_JNE: {
      printf("jne ");
    } break;
    case OPCODE_JNL: {
      printf("jnl ");
    } break;
    case OPCODE_JG: {
      printf("jg ");
    } break;
    case OPCODE_JNB: {
      printf("jnb ");
    } break;
    case OPCODE_JA: {
      printf("ja ");
    } break;
    case OPCODE_JNP: {
      printf("jnp ");
    } break;
    case OPCODE_JNO: {
      printf("jno ");
    } break;
    case OPCODE_JNS: {
      printf("jns ");
    } break;
    case OPCODE_LOOP: {
      printf("loop ");
    } break;
    case OPCODE_LOOPZ: {
      printf("loopz ");
    } break;
    case OPCODE_LOOPNZ: {
      printf("loopnz ");
    } break;
    case OPCODE_JCXZ: {
      printf("jcxz ");
    } break;
    default:
      INSTRUCTION_NOT_IMPLEMENTED();
    }
    instruction_byte_count = 2;
    int8_t value = instruction[1] + 2;
    if (value < 0) {
      printf("$+0%hhd", value);
    } else {
      printf("$+0+%hhd", value);
    }
  } else {
    INSTRUCTION_NOT_IMPLEMENTED();
  }
  return instruction_byte_count;
}
