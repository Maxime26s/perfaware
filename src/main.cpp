#include "assert.h"
#include "stdint.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

// name is a 2 byte buffer;
static uint32_t GetReg(uint8_t reg, uint8_t is_word, char *name) {
  switch (reg) {
  case 0b00000000: {
    name[0] = 'a';
    name[1] = is_word ? 'x' : 'l';
  } break;
  case 0b00000001: {
    name[0] = 'c';
    name[1] = is_word ? 'x' : 'l';
  } break;
  case 0b00000010: {
    name[0] = 'd';
    name[1] = is_word ? 'x' : 'l';
  } break;
  case 0b00000011: {
    name[0] = 'b';
    name[1] = is_word ? 'x' : 'l';
  } break;
  case 0b00000100: {
    strncpy(name, is_word ? "sp" : "ah", 2);
  } break;
  case 0b00000101: {
    strncpy(name, is_word ? "bp" : "ch", 2);
  } break;
  case 0b00000110: {
    strncpy(name, is_word ? "si" : "dh", 2);
  } break;
  case 0b00000111: {
    strncpy(name, is_word ? "di" : "bh", 2);
  } break;
  }
  name[2] = '\0';
  return 3;
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
  printf("; %s\n", filename);
  printf("bits 16\n");
  for (long i = 0; i < fileSize;) {
    uint8_t *byte = &buffer[i];
    // printf("filesize: %ld, i: %ld, byte: %.*s\n", fileSize, i, 6, byte);
    if (((byte[0] & 0b11111100) ^ 0b10001000) == 0) {
      // mov
      printf("mov ");
      if (byte[0] & 0b00000010) {
        // first reg is destination
        printf("reg is dest\n");
      } else {
        // first reg is source
        // printf("reg is source\n");
        if (byte[1] & 0b11000000) {
          // register mode
          uint32_t count;
          uint8_t is_wide = byte[0] & 0b00000001;

          count = GetReg((byte[1] & 0b00000111), is_wide, data_buf);
          printf("%.*s, ", (int)count, data_buf);

          count = GetReg((byte[1] & 0b00111000) >> 3, is_wide, data_buf);
          printf("%.*s", (int)count, data_buf);
        }
      }
      i += 2;
    }
    printf("\n");
  }

  // Clean up
  free(buffer);
  fclose(file);

  return 0;
}
