#include "assert.h"
#include "stdint.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

// https://stackoverflow.com/a/3208376
#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)                                                   \
  ((byte) & 0x80 ? '1' : '0'), ((byte) & 0x40 ? '1' : '0'),                    \
      ((byte) & 0x20 ? '1' : '0'), ((byte) & 0x10 ? '1' : '0'),                \
      ((byte) & 0x08 ? '1' : '0'), ((byte) & 0x04 ? '1' : '0'),                \
      ((byte) & 0x02 ? '1' : '0'), ((byte) & 0x01 ? '1' : '0')

#define INSTRUCTION_NOT_IMPLEMENTED()                                          \
  printf("\n" BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(instruction[0]));         \
  printf(" - INSTRUCTION NOT IMPLEMENTED\n");                                  \
  exit(-1);

#define OPCODE_MOV_REGMEM2REG 0b10001000
#define OPCODE_MOV_IMM2REGMEM 0b11000110
#define OPCODE_MOV_IMM2REG 0b10110000
#define OPCODE_MOV_MEM2ACC 0b10100000
#define OPCODE_MOV_ACC2MEM 0b10100010

#define OPCODE_ADD_REGMEM2REG 0b00000000
#define OPCODE_ADD_IMM2REGMEM 0b10000000
#define OPCODE_ADD_IMM2ACC 0b00000100

#define OPCODE_SUB_REGMEM2REG 0b00101000
#define OPCODE_SUB_IMM2REGMEM 0b10000000
#define OPCODE_SUB_IMM2ACC 0b00101100

#define OPCODE_CMP_REGMEM2REG 0b00111000
#define OPCODE_CMP_IMM2REGMEM 0b10000000
#define OPCODE_CMP_IMM2ACC 0b00111100

#define OPCODE_JE 0b01110100
#define OPCODE_JL 0b01111100
#define OPCODE_JLE 0b01111110
#define OPCODE_JB 0b01110010
#define OPCODE_JBE 0b01110110
#define OPCODE_JP 0b01111010
#define OPCODE_JO 0b01110000
#define OPCODE_JS 0b01111000
#define OPCODE_JNE 0b01110101
#define OPCODE_JNL 0b01111101
#define OPCODE_JG 0b01111111
#define OPCODE_JNB 0b01110011
#define OPCODE_JA 0b01110111
#define OPCODE_JNP 0b01111011
#define OPCODE_JNO 0b01110001
#define OPCODE_JNS 0b01111001
#define OPCODE_LOOP 0b11100010
#define OPCODE_LOOPZ 0b11100001
#define OPCODE_LOOPNZ 0b11100000
#define OPCODE_JCXZ 0b11100011

// name is a 3 byte buffer;
static uint32_t GetReg(uint8_t data, uint8_t is_word, char *buffer) {
  switch (data) {
  case 0b00000000: {
    buffer[0] = 'a';
    buffer[1] = is_word ? 'x' : 'l';
  } break;
  case 0b00000001: {
    buffer[0] = 'c';
    buffer[1] = is_word ? 'x' : 'l';
  } break;
  case 0b00000010: {
    buffer[0] = 'd';
    buffer[1] = is_word ? 'x' : 'l';
  } break;
  case 0b00000011: {
    buffer[0] = 'b';
    buffer[1] = is_word ? 'x' : 'l';
  } break;
  case 0b00000100: {
    strncpy(buffer, is_word ? "sp" : "ah", 2);
  } break;
  case 0b00000101: {
    strncpy(buffer, is_word ? "bp" : "ch", 2);
  } break;
  case 0b00000110: {
    strncpy(buffer, is_word ? "si" : "dh", 2);
  } break;
  case 0b00000111: {
    strncpy(buffer, is_word ? "di" : "bh", 2);
  } break;
  }
  buffer[2] = '\0';
  return 3;
}

// name is a 6 byte buffer;
static uint32_t GetEffectiveAddress(uint8_t data, char *buffer) {
  if ((data & 0b11000111) == 0b00000110) {
    return 0;
  }
  switch (data & 0b00000111) {
  case 0b00000000: {
    strncpy(buffer, "bx + si", 8);
    return 8;
  } break;
  case 0b00000001: {
    strncpy(buffer, "bx + di", 8);
    return 8;
  } break;
  case 0b00000010: {
    strncpy(buffer, "bp + si", 8);
    return 8;
  } break;
  case 0b00000011: {
    strncpy(buffer, "bp + di", 8);
    return 8;
  } break;
  case 0b00000100: {
    strncpy(buffer, "si", 3);
    return 3;
  } break;
  case 0b00000101: {
    strncpy(buffer, "di", 3);
    return 3;
  } break;
  case 0b00000110: {
    strncpy(buffer, "bp", 3);
    return 3;
  } break;
  case 0b00000111: {
    strncpy(buffer, "bx", 3);
    return 3;
  } break;
  }
  return 0;
}

uint32_t GetValue(uint8_t *data, bool is_signed, bool is_wide, uint16_t *value,
                  char *data_buf) {
  *value = data[0];
  if (is_signed) {
    *value |= (*value & 0b10000000) << 8;
    strncpy(data_buf, "%hd", 4);
    return 1;
  } else if (is_wide) {
    *value += (data[1] << 8);
    strncpy(data_buf, "%hd", 4);
    return 2;
  } else {
    strncpy(data_buf, "%hhd", 5);
    return 1;
  }
}

uint32_t PrintEffectiveAddress(uint8_t *instruction, char *data_buf) {
  uint32_t optional_byte_count = 0;
  uint8_t mod = instruction[1] & 0b11000000;

  uint32_t count = GetEffectiveAddress(instruction[1], data_buf);
  if (count == 0) {
    printf("[");
  } else {
    printf("[%.*s", (int)count, data_buf);
  }

  if (mod > 0b00000000 || count == 0) {
    if (mod > 0b00000000) {
      printf(" + ");
    }

    uint16_t value;
    optional_byte_count =
        GetValue(&instruction[2], false, mod > 0b01000000 || count == 0, &value,
                 data_buf);
    printf(data_buf, value);
  }
  printf("]");

  return optional_byte_count;
}

uint32_t RegisterOrMemoryWithRegisterToEither(uint8_t *instruction,
                                              char *buffer) {
  uint8_t is_dest = instruction[0] & 0b00000010;
  uint8_t is_wide = instruction[0] & 0b00000001;
  uint8_t mod = instruction[1] & 0b11000000;

  uint32_t optional_byte_count = 0;
  uint32_t buffer_size = 0;

  if (is_dest) {
    buffer_size = GetReg((instruction[1] & 0b00111000) >> 3, is_wide, buffer);
    printf("%.*s", (int)buffer_size, buffer);
    printf(", ");
  }

  if (mod == 0b11000000) {
    buffer_size = GetReg((instruction[1] & 0b00000111), is_wide, buffer);
    printf("%.*s", (int)buffer_size, buffer);
  } else {
    optional_byte_count += PrintEffectiveAddress(instruction, buffer);
  }

  if (!is_dest) {
    printf(", ");
    buffer_size = GetReg((instruction[1] & 0b00111000) >> 3, is_wide, buffer);
    printf("%.*s", (int)buffer_size, buffer);
  }

  return optional_byte_count;
}

uint32_t ImmediateToRegisterOrMemory(uint8_t *instruction, char *buffer,
                                     bool signed_op) {
  uint8_t is_signed = signed_op ? instruction[0] & 0b00000010 : 0;
  uint8_t is_wide = instruction[0] & 0b00000001;
  uint8_t mod = instruction[1] & 0b11000000;

  uint32_t optional_byte_count = 0;
  uint32_t buffer_size = 0;

  if ((mod ^ 0b11000000) == 0) {
    buffer_size = GetReg((instruction[1] & 0b00000111), is_wide, buffer);
    printf("%.*s", (int)buffer_size, buffer);
  } else {
    printf(is_wide ? "word " : "byte ");
    optional_byte_count += PrintEffectiveAddress(instruction, buffer);
  }
  printf(", ");

  uint16_t value;
  optional_byte_count += GetValue(&instruction[2 + optional_byte_count],
                                  is_signed, is_wide, &value, buffer) -
                         1;
  printf(buffer, value);

  return optional_byte_count;
}

uint32_t ImmediateToRegister(uint8_t *instruction, char *buffer,
                             uint8_t is_wide, uint8_t reg) {

  uint32_t optional_byte_count = 0;
  uint32_t buffer_size = 0;

  buffer_size = GetReg(reg, is_wide, buffer);
  printf("%.*s, ", (int)buffer_size, buffer);

  uint16_t value;
  optional_byte_count +=
      GetValue(&instruction[1], false, is_wide, &value, buffer) - 1;
  printf(buffer, value);

  return optional_byte_count;
}

uint32_t ImmediateToAccumulator(uint8_t *instruction, char *buffer) {
  uint8_t is_wide = instruction[0] & 0b00000001;
  return ImmediateToRegister(instruction, buffer, is_wide, 0);
}

uint32_t AddressWithAccumulatorToEither(uint8_t *instruction, char *buffer) {
  uint8_t is_dest = !(instruction[0] & 0b00000010);
  uint8_t is_wide = instruction[0] & 0b00000001;

  uint32_t optional_byte_count = 0;
  uint32_t buffer_size = 0;

  if (is_dest) {
    buffer_size = GetReg(0, is_wide, buffer);
    printf("%.*s, ", (int)buffer_size, buffer);
  }

  printf("[");
  uint16_t value;
  optional_byte_count +=
      GetValue(&instruction[1], false, is_wide, &value, buffer) - 1;
  printf(buffer, value);
  printf("]");

  if (!is_dest) {
    buffer_size = GetReg(0, is_wide, buffer);
    printf(", %.*s", (int)buffer_size, buffer);
  }

  return optional_byte_count;
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    printf("needs a file");
    return -1;
  }

  char *filename = argv[1];
  FILE *file = fopen(filename, "rb");

  // Determine the file size
  fseek(file, 0, SEEK_END);    // Move to the end of the file
  long fileSize = ftell(file); // Get the size of the file
  rewind(file);                // Reset file pointer to the beginning

  // Allocate memory for the entire file
  unsigned char *buffer = (unsigned char *)malloc(fileSize);
  if (buffer == NULL) {
    perror("Memory allocation failed");
    fclose(file);
    return 1;
  }

  // Read the entire file in one go
  if (fread(buffer, 1, fileSize, file) != fileSize) {
    perror("Error reading file");
    free(buffer);
    fclose(file);
    return 1;
  }

  // Example: Process or print the file contents (optional)

  char data_buf[256];
  uint32_t count;
  printf("; %s\n", filename);
  printf("bits 16\n");
  for (long i = 0; i < fileSize;) {
    uint32_t instruction_byte_count = 0;
    uint8_t *instruction = &buffer[i];

    if ((instruction[0] & 0b11111100) == OPCODE_MOV_REGMEM2REG) {
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
    } else if ((instruction[0] & 0b11111110) == OPCODE_MOV_IMM2REGMEM) {
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
    } else if ((instruction[0] & 0b11111100) == OPCODE_ADD_REGMEM2REG ||
               (instruction[0] & 0b11111100) == OPCODE_SUB_REGMEM2REG ||
               (instruction[0] & 0b11111100) == OPCODE_CMP_REGMEM2REG) {
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
               (OPCODE_ADD_IMM2REGMEM | OPCODE_SUB_IMM2REGMEM |
                OPCODE_CMP_IMM2REGMEM)) {
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
               instruction[0] == OPCODE_LOOP ||
               instruction[0] == OPCODE_LOOPZ ||
               instruction[0] == OPCODE_LOOPNZ ||
               instruction[0] == OPCODE_JCXZ) {
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
    printf("\n");
    i += instruction_byte_count;
  }

  // Clean up
  free(buffer);
  fclose(file);

  return 0;
}
