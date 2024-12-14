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
  if (((data & 0b11000111) ^ 0b00000110) == 0) {
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

uint32_t GetValue(uint8_t *data, bool is_wide, uint16_t *value,
                  char *data_buf) {
  *value = data[0];
  if (is_wide) {
    *value += (data[1] << 8);
    strncpy(data_buf, "%hd", 4);
    return 2;
  } else {
    strncpy(data_buf, "%hhd", 5);
    return 1;
  }
}

uint32_t PrintEffectiveAddress(uint8_t *instruction, char *data_buf) {
  uint32_t additional_bytes = 0;
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
    additional_bytes = GetValue(&instruction[2], mod > 0b01000000 || count == 0,
                                &value, data_buf);
    printf(data_buf, value);
  }
  printf("]");

  return additional_bytes;
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
    uint32_t additional_bytes = 0;
    uint8_t *instruction = &buffer[i];
    // printf("filesize: %ld, i: %ld, byte: %.*s\n", fileSize, i, 6, byte);
    if (((instruction[0] & 0b11111100) ^ 0b10001000) == 0) {
      // mov
      printf("mov ");

      uint8_t is_wide = instruction[0] & 0b00000001;
      uint8_t mod = instruction[1] & 0b11000000;
      uint8_t is_dest = instruction[0] & 0b00000010;
      if (is_dest) {
        // first reg is destination
        count = GetReg((instruction[1] & 0b00111000) >> 3, is_wide, data_buf);
        printf("%.*s", (int)count, data_buf);
        printf(", ");
      }

      if ((mod ^ 0b11000000) == 0) {
        // register mode
        count = GetReg((instruction[1] & 0b00000111), is_wide, data_buf);
        printf("%.*s", (int)count, data_buf);
      } else {
        i += PrintEffectiveAddress(instruction, data_buf);
      }

      if (!is_dest) {
        // first reg is source
        printf(", ");
        count = GetReg((instruction[1] & 0b00111000) >> 3, is_wide, data_buf);
        printf("%.*s", (int)count, data_buf);
      }
      i += 2;
    } else if (((instruction[0] & 0b11110000) ^ 0b10110000) == 0) {
      // mov
      printf("mov ");
      uint8_t is_wide = instruction[0] & 0b00001000;

      count = GetReg((instruction[0] & 0b00000111), is_wide, data_buf);
      printf("%.*s, ", (int)count, data_buf);

      uint16_t value;
      i += GetValue(&instruction[1], is_wide, &value, data_buf);
      printf(data_buf, value);
      ++i;
    } else if (((instruction[0] & 0b11111110) ^ 0b11000110) == 0) {
      // mov
      printf("mov ");

      uint8_t is_wide = instruction[0] & 0b00000001;
      uint8_t mod = instruction[1] & 0b11000000;

      if ((mod ^ 0b11000000) == 0) {
        // register mode
        count = GetReg((instruction[1] & 0b00000111), is_wide, data_buf);
        printf("%.*s", (int)count, data_buf);
      } else {
        additional_bytes = PrintEffectiveAddress(instruction, data_buf);
      }
      printf(", ");

      uint16_t value;
      i += GetValue(&instruction[2 + additional_bytes], is_wide, &value,
                    data_buf);
      printf(is_wide ? "word " : "byte ");
      printf(data_buf, value);

      i += additional_bytes;
      i += 2;
    } else if (((instruction[0] & 0b11111100) ^ 0b10100000) == 0) {
      // mov
      printf("mov ");

      uint8_t is_dest = !(instruction[0] & 0b00000010);
      uint8_t is_wide = instruction[0] & 0b00000001;

      if (is_dest) {
        // first reg is destination
        printf("ax, ");
      }

      printf("[");
      uint16_t value;
      i += GetValue(&instruction[1], is_wide, &value, data_buf);
      printf(data_buf, value);
      printf("]");

      if (!is_dest) {
        printf(", ax");
      }
      ++i;
    } else {
    unhandled:
      printf(BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(instruction[0]));
      printf("unsupported op code\n");
      break;
    }
    printf("\n");
  }

  // Clean up
  free(buffer);
  fclose(file);

  return 0;
}
