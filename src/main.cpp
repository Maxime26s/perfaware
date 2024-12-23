#include "assert.h"
#include "stdint.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#include "decode.cpp"

int main(int argc, char *argv[]) {
  if (argc != 2) {
    printf("needs a file");
    return -1;
  }

  uint32_t buffer_size = 1024 * 1024;
  uint8_t *buffer = (uint8_t *)malloc(buffer_size);
  uint32_t byte_read;

  char *filename = argv[1];
  FILE *file = {};
  if (fopen_s(&file, filename, "rb") == 0) {
    byte_read = fread(buffer, 1, buffer_size, file);
    fclose(file);
  } else {
    fprintf(stderr, "ERROR: Unable to open %s.\n", filename);
  }

  printf("; %s\n", filename);
  printf("bits 16\n");
  MemoryAccess memory_idx = {};
  memory_idx.base = buffer;
  while (memory_idx.base - buffer < byte_read) {
    DecodeInstruction(&memory_idx);
    memory_idx.base += memory_idx.offset;
    memory_idx.offset = 0;

    printf("\n");
  }

  return 0;
}
