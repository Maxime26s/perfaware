#include "assert.h"
#include "stdint.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#include "opcode.h"

// https://stackoverflow.com/a/3208376
#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)                                                   \
  ((byte) & 0x80 ? '1' : '0'), ((byte) & 0x40 ? '1' : '0'),                    \
      ((byte) & 0x20 ? '1' : '0'), ((byte) & 0x10 ? '1' : '0'),                \
      ((byte) & 0x08 ? '1' : '0'), ((byte) & 0x04 ? '1' : '0'),                \
      ((byte) & 0x02 ? '1' : '0'), ((byte) & 0x01 ? '1' : '0')

#define INSTRUCTION_NOT_IMPLEMENTED()                                          \
  printf("\n" BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(instruction));            \
  printf(" - INSTRUCTION NOT IMPLEMENTED\n");                                  \
  exit(-1);

#define BIT_MASK(bits) (~(0xff << bits))
#define BIT_SHIFT(byte, bits) (byte >> bits)
#define BIT_SHIFT_MASK(byte, shift, mask)                                      \
  (BIT_SHIFT(byte, shift) & BIT_MASK(mask))

#define BITS(bits) {Bit_Literal, sizeof(#bits) - 1, 0b##bits}

#define MOD {Bit_Mod, 2}
#define REG {Bit_Reg, 3}
#define RM {Bit_RM, 3}
#define DATA {Bit_Data, 0}
#define DISP {Bit_Displacement, 0}
#define ADDR {Bit_Address, 0}
#define W {Bit_Wide, 1}
#define S {Bit_Signed, 1}
#define D {Bit_Destination, 1}

#define FLAG(flag) {flag, 0, 1}

#define IMP_MOD(value) {Bit_Mod, 0, value}
#define IMP_REG(value) {Bit_Reg, 0, value}
#define IMP_RM(value) {Bit_RM, 0, value}
#define IMP_DATA(value) {Bit_Data, 0, value}
#define IMP_DISP(value) {Bit_Displacement, 0, value}
#define IMP_W(value) {Bit_Wide, 0, value}
#define IMP_S(value) {Bit_Signed, 0, value}
#define IMP_D(value) {Bit_Destination, 0, value}

InstructionEncoding instructions[] = {
    {Op_mov, {BITS(100010), D, W, MOD, REG, RM, DISP}},
    {Op_mov, {BITS(1100011), W, MOD, BITS(000), RM, DISP, DATA}},
    {Op_mov, {BITS(1011), W, REG, DATA}},
    {Op_mov,
     {BITS(101000), D, W, ADDR, IMP_MOD(0b00), IMP_REG(0b000), IMP_RM(0b110)}},

    {Op_add, {BITS(000000), D, W, MOD, REG, RM, DISP}},
    {Op_add, {BITS(100000), S, W, MOD, BITS(000), RM, DISP, DATA}},
    {Op_add, {BITS(0000010), DATA, IMP_D(1), IMP_REG(0b000)}},

    {Op_sub, {BITS(001010), D, W, MOD, REG, RM, DISP}},
    {Op_sub, {BITS(100000), S, W, MOD, BITS(101), RM, DISP, DATA}},
    {Op_sub, {BITS(0010110), DATA, IMP_D(1), IMP_REG(0b000)}},

    {Op_cmp, {BITS(001110), D, W, MOD, REG, RM, DISP}},
    {Op_cmp, {BITS(100000), S, W, MOD, BITS(111), RM, DISP, DATA}},
    {Op_cmp, {BITS(0011110), DATA, IMP_D(1), IMP_REG(0b000)}},

    {Op_je, {BITS(01110100), FLAG(Bit_RelativeJmpAddress)}},
    {Op_jl, {BITS(01111100), FLAG(Bit_RelativeJmpAddress)}},
    {Op_jle, {BITS(01111110), FLAG(Bit_RelativeJmpAddress)}},
    {Op_jb, {BITS(01110010), FLAG(Bit_RelativeJmpAddress)}},
    {Op_jbe, {BITS(01110110), FLAG(Bit_RelativeJmpAddress)}},
    {Op_jp, {BITS(01111010), FLAG(Bit_RelativeJmpAddress)}},
    {Op_jo, {BITS(01110000), FLAG(Bit_RelativeJmpAddress)}},
    {Op_js, {BITS(01111000), FLAG(Bit_RelativeJmpAddress)}},
    {Op_jne, {BITS(01110101), FLAG(Bit_RelativeJmpAddress)}},
    {Op_jnl, {BITS(01111101), FLAG(Bit_RelativeJmpAddress)}},
    {Op_jg, {BITS(01111111), FLAG(Bit_RelativeJmpAddress)}},
    {Op_jnb, {BITS(01110011), FLAG(Bit_RelativeJmpAddress)}},
    {Op_ja, {BITS(01110111), FLAG(Bit_RelativeJmpAddress)}},
    {Op_jnp, {BITS(01111011), FLAG(Bit_RelativeJmpAddress)}},
    {Op_jno, {BITS(01110001), FLAG(Bit_RelativeJmpAddress)}},
    {Op_jns, {BITS(01111001), FLAG(Bit_RelativeJmpAddress)}},
    {Op_loop, {BITS(11100010), FLAG(Bit_RelativeJmpAddress)}},
    {Op_loopz, {BITS(11100001), FLAG(Bit_RelativeJmpAddress)}},
    {Op_loopnz, {BITS(11100000), FLAG(Bit_RelativeJmpAddress)}},
    {Op_jcxz, {BITS(11100011), FLAG(Bit_RelativeJmpAddress)}},
};

struct MemoryAccess {
  uint8_t *base;
  uint8_t offset;
};

uint8_t *GetAddress(uint8_t *base, uint8_t offset,
                    uint8_t additional_offset = 0) {
  return base + offset + additional_offset;
}

uint8_t *GetAddress(MemoryAccess memory_idx, uint8_t additional_offset = 0) {
  return GetAddress(memory_idx.base, memory_idx.offset, additional_offset);
}

uint8_t ReadMemory(MemoryAccess memory_idx, uint8_t additional_offset = 0) {
  return *GetAddress(memory_idx, additional_offset);
}

#define ARRAY_SIZE(array) (sizeof(array) / sizeof(0 [array]))

static uint16_t ParseValue(MemoryAccess *memory_idx, bool is_wide,
                           bool is_signed_extended) {
  uint16_t result = {};

  if (is_wide) {
    uint8_t low = ReadMemory(*memory_idx);
    ++memory_idx->offset;

    uint8_t high = ReadMemory(*memory_idx);
    ++memory_idx->offset;

    result = high << 8 | low;
  } else {
    result = ReadMemory(*memory_idx);
    ++memory_idx->offset;

    if (is_signed_extended) {
      result |= (result & 0b10000000) << 8;
    }
  }

  return result;
}

Instruction TryParse(InstructionEncoding instruction, MemoryAccess memory_idx) {
  bool valid = true;
  uint32_t bits[Bit_Count];
  uint32_t has_bits;

  uint8_t remaining_bits = 0;
  uint8_t read_byte = 0;
  for (uint32_t bit_idx = 0; valid && bit_idx < ARRAY_SIZE(instruction.bits);
       ++bit_idx) {
    InstructionBit test_bits = instruction.bits[bit_idx];

    uint8_t read_bits = test_bits.value;
    if (test_bits.size != 0) {
      if (remaining_bits == 0) {
        remaining_bits = 8;
        read_byte = ReadMemory(memory_idx);
        ++memory_idx.offset;
      }

      remaining_bits -= test_bits.size;
      read_bits = BIT_SHIFT_MASK(read_byte, remaining_bits, test_bits.size);
    }

    if (test_bits.type == Bit_Literal) {
      valid = valid && read_bits == test_bits.value;
    } else {
      bits[test_bits.type] = read_bits;
      has_bits |= 1 << test_bits.type;
    }
  }

  Instruction result = {};
  if (valid) {
    result.op = instruction.op;
    result.address = (uint32_t)memory_idx.base;
    result.size = memory_idx.offset;

    uint32_t mod = bits[Bit_Mod];
    uint32_t rm = bits[Bit_RM];
    uint32_t w = bits[Bit_Wide];
    uint32_t s = bits[Bit_Signed];
    uint32_t d = bits[Bit_Destination];

    bool has_direct_address = (mod == 0b00) && (rm == 0b110);
    bool has_displacement =
        has_direct_address || mod == 0b01 || mod == 0b10 || bits[Bit_Address];
    bool has_data = bits[Bit_Data];
    bool is_displacement_wide =
        has_direct_address || mod == 0b10 || bits[Bit_Address];
    bool is_data_wide = w && !s;

    if (has_displacement) {
      bits[Bit_Displacement] |=
          ParseValue(&memory_idx, is_displacement_wide, !is_displacement_wide);
    }
    if (bits[Bit_Data]) {
      ParseValue(&memory_idx, is_data_wide, s);
    }

    printf("; " BYTE_TO_BINARY_PATTERN " - matched\n",
           BYTE_TO_BINARY(instruction.bits[0].value));
  }

  return result;
}

Instruction ParseInstruction(MemoryAccess *memory_idx) {
  Instruction result = {};
  for (uint32_t i = 0; i < ARRAY_SIZE(instructions); ++i) {
    InstructionEncoding instruction = instructions[i];
    result = TryParse(instruction, *memory_idx);
    if (result.op) {
      // memory_idx->offset += result.size;
      break;
    }
  }

  return result;
}

static RegisterInfo ParseRegister(uint8_t register_idx, bool is_wide) {
  RegisterInfo register_table[][2] = {
      {{Register_a, 1, 0}, {Register_a, 2, 0}},
      {{Register_c, 1, 0}, {Register_c, 2, 0}},
      {{Register_d, 1, 0}, {Register_d, 2, 0}},
      {{Register_b, 1, 0}, {Register_b, 2, 0}},
      {{Register_a, 1, 1}, {Register_sp, 2, 0}},
      {{Register_c, 1, 1}, {Register_bp, 2, 0}},
      {{Register_d, 1, 1}, {Register_si, 2, 0}},
      {{Register_b, 1, 1}, {Register_di, 2, 0}},
  };

  uint8_t register_mask = 0x7;
  return register_table[register_idx & register_mask][is_wide];
}

static EffectiveAddress ParseEffectiveAddress(uint8_t rm, uint8_t mod,
                                              MemoryAccess *memory_idx) {
  EffectiveAddress effective_address_table[][3] = {
      {{EffectiveAddress_bx_si, 0},
       {EffectiveAddress_bx_si, 1},
       {EffectiveAddress_bx_si, 2}},
      {{EffectiveAddress_bx_di, 0},
       {EffectiveAddress_bx_di, 1},
       {EffectiveAddress_bx_di, 2}},
      {{EffectiveAddress_bp_si, 0},
       {EffectiveAddress_bp_si, 1},
       {EffectiveAddress_bp_si, 2}},
      {{EffectiveAddress_bp_di, 0},
       {EffectiveAddress_bp_di, 1},
       {EffectiveAddress_bp_di, 2}},
      {{EffectiveAddress_si, 0},
       {EffectiveAddress_si, 1},
       {EffectiveAddress_si, 2}},
      {{EffectiveAddress_di, 0},
       {EffectiveAddress_di, 1},
       {EffectiveAddress_di, 2}},
      {{EffectiveAddress_direct, 2},
       {EffectiveAddress_bp, 1},
       {EffectiveAddress_bp, 2}},
      {{EffectiveAddress_bx, 0},
       {EffectiveAddress_bx, 1},
       {EffectiveAddress_bx, 2}},
  };

  uint8_t mod_mask = 0x3;
  uint8_t rm_mask = 0x7;

  EffectiveAddress address =
      effective_address_table[rm & rm_mask][mod & mod_mask];
  if (mod != 0 || address.base == EffectiveAddress_direct) {
    address.is_wide = address.displacement == 2;
    address.displacement = ParseValue(memory_idx, address.is_wide, false);
  }

  return address;
}

// name is a 3 byte buffer;
static char const *GetRegisterName(RegisterInfo reg) {
  char const *register_table[][3] = {
      {"al", "ah", "ax"}, {"cl", "ch", "cx"}, {"dl", "dh", "dx"},
      {"bl", "bh", "bx"}, {"sp", "sp", "sp"}, {"bp", "bp", "bp"},
      {"si", "si", "si"}, {"di", "di", "di"}, {"es", "es", "es"},
      {"cs", "cs", "cs"}, {"ds", "ds", "ds"}, {"ip", "ip", "ip"},
  };

  uint8_t wide_mask = 0x2;
  return register_table[reg.name][reg.size & wide_mask + reg.offset];
}

// name is a 6 byte buffer;
static char const *GetEffectiveAddressBase(EffectiveAddress effective_address) {
  char const *address_base_table[] = {
      "bx+si", "bx+di", "bp+si", "bp+di", "si", "di", "bp", "bx", "",
  };

  return address_base_table[effective_address.base];
}

void PrintValue(uint16_t value, bool is_wide) {
  if (is_wide) {
    printf("%hd", value);
  } else {
    printf("%hhd", (uint8_t)value);
  }
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

void PrintEffectiveAddress(EffectiveAddress effective_address) {
  printf("[");

  if (effective_address.base != EffectiveAddress_direct) {
    printf("%s", GetEffectiveAddressBase(effective_address));
    if (effective_address.displacement != 0) {
      printf("+");
    }
  }

  if (effective_address.displacement != 0) {
    PrintValue(effective_address.displacement, effective_address.is_wide);
  }

  printf("]");
}

void RegisterOrMemoryWithRegisterToEither(MemoryAccess *memory_idx) {
  uint8_t is_dest = BIT_SHIFT_MASK(ReadMemory(*memory_idx), 1, 1);
  uint8_t is_wide = BIT_SHIFT_MASK(ReadMemory(*memory_idx), 0, 1);
  ++memory_idx->offset;

  uint8_t mod_bits = BIT_SHIFT_MASK(ReadMemory(*memory_idx), 6, 2);
  uint8_t reg_bits = BIT_SHIFT_MASK(ReadMemory(*memory_idx), 3, 3);
  uint8_t rm_bits = BIT_SHIFT_MASK(ReadMemory(*memory_idx), 0, 3);
  ++memory_idx->offset;

  if (is_dest) {
    RegisterInfo reg = ParseRegister(reg_bits, is_wide);
    printf("%s", GetRegisterName(reg));
    printf(", ");
  }

  if (mod_bits == 3) {
    RegisterInfo reg = ParseRegister(rm_bits, is_wide);
    printf("%s", GetRegisterName(reg));
  } else {
    EffectiveAddress effective_address =
        ParseEffectiveAddress(rm_bits, mod_bits, memory_idx);
    PrintEffectiveAddress(effective_address);
  }

  if (!is_dest) {
    printf(", ");
    RegisterInfo reg = ParseRegister(reg_bits, is_wide);
    printf("%s", GetRegisterName(reg));
  }
}

void ImmediateToRegisterOrMemory(MemoryAccess *memory_idx, bool is_signed_op) {
  uint8_t is_signed =
      is_signed_op && BIT_SHIFT_MASK(ReadMemory(*memory_idx), 1, 1);
  uint8_t is_wide = BIT_SHIFT_MASK(ReadMemory(*memory_idx), 0, 1);
  ++memory_idx->offset;

  uint8_t mod_bits = BIT_SHIFT_MASK(ReadMemory(*memory_idx), 6, 2);
  uint8_t rm_bits = BIT_SHIFT_MASK(ReadMemory(*memory_idx), 0, 3);
  ++memory_idx->offset;

  if (mod_bits == 3) {
    RegisterInfo reg = ParseRegister(rm_bits, is_wide);
    printf("%s", GetRegisterName(reg));
  } else {
    printf(is_wide ? "word " : "byte ");

    EffectiveAddress effective_address =
        ParseEffectiveAddress(rm_bits, mod_bits, memory_idx);
    PrintEffectiveAddress(effective_address);
  }
  printf(", ");

  uint16_t value = ParseValue(memory_idx, is_wide && !is_signed, is_signed);
  PrintValue(value, is_wide | is_signed);
}

void ImmediateToRegister(MemoryAccess *memory_idx, uint8_t is_wide,
                         uint8_t reg_idx) {
  RegisterInfo reg = ParseRegister(reg_idx, is_wide);
  printf("%s", GetRegisterName(reg));

  printf(", ");

  uint16_t value = ParseValue(memory_idx, is_wide, false);
  PrintValue(value, is_wide);
}

void ImmediateToAccumulator(MemoryAccess *memory_idx) {
  uint8_t is_wide = BIT_SHIFT_MASK(ReadMemory(*memory_idx), 0, 1);
  ++memory_idx->offset;

  ImmediateToRegister(memory_idx, is_wide, 0);
}

void AddressWithAccumulatorToEither(MemoryAccess *memory_idx) {
  uint8_t is_dest = !BIT_SHIFT_MASK(ReadMemory(*memory_idx), 1, 1);
  uint8_t is_wide = BIT_SHIFT_MASK(ReadMemory(*memory_idx), 0, 1);
  ++memory_idx->offset;

  if (is_dest) {
    RegisterInfo reg = ParseRegister(0, is_wide);
    printf("%s", GetRegisterName(reg));
    printf(", ");
  }

  printf("[");
  uint16_t value = ParseValue(memory_idx, is_wide, false);
  PrintValue(value, is_wide);
  printf("]");

  if (!is_dest) {
    printf(", ");
    RegisterInfo reg = ParseRegister(0, is_wide);
    printf("%s", GetRegisterName(reg));
  }
}

void DecodeInstruction(MemoryAccess *memory_idx) {
  ParseInstruction(memory_idx);
  uint8_t instruction = ReadMemory(*memory_idx);
  if ((instruction >> 2) == OPCODE_MOV_RM2REG) {
    printf("mov ");
    RegisterOrMemoryWithRegisterToEither(memory_idx);
  } else if ((instruction >> 4) == OPCODE_MOV_IMM2REG) {
    printf("mov ");

    uint8_t is_wide = BIT_SHIFT_MASK(instruction, 3, 1);
    uint8_t reg = BIT_SHIFT_MASK(instruction, 0, 3);
    ++memory_idx->offset;

    ImmediateToRegister(memory_idx, is_wide, reg);
  } else if ((instruction >> 1) == OPCODE_MOV_IMM2RM) {
    printf("mov ");
    ImmediateToRegisterOrMemory(memory_idx, false);
  } else if ((instruction >> 2) ==
             ((OPCODE_MOV_MEM2ACC | OPCODE_MOV_ACC2MEM) & 0b11111100)) {
    printf("mov ");
    AddressWithAccumulatorToEither(memory_idx);
  } else if ((instruction >> 2) == OPCODE_ADD_RM2REG ||
             (instruction >> 2) == OPCODE_SUB_RM2REG ||
             (instruction >> 2) == OPCODE_CMP_RM2REG) {
    switch (instruction & 0b00111000) {
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
    RegisterOrMemoryWithRegisterToEither(memory_idx);
  } else if ((instruction >> 2) ==
             (OPCODE_ADD_IMM2RM | OPCODE_SUB_IMM2RM | OPCODE_CMP_IMM2RM)) {
    switch ((ReadMemory(*memory_idx, 1)) & 0b00111000) {
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
    ImmediateToRegisterOrMemory(memory_idx, true);
  } else if ((instruction >> 1) == OPCODE_ADD_IMM2ACC ||
             (instruction >> 1) == OPCODE_SUB_IMM2ACC ||
             (instruction >> 1) == OPCODE_CMP_IMM2ACC) {
    switch (instruction & 0b00111000) {
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
    ImmediateToAccumulator(memory_idx);
  } else if (instruction == OPCODE_JE || instruction == OPCODE_JL ||
             instruction == OPCODE_JLE || instruction == OPCODE_JB ||
             instruction == OPCODE_JBE || instruction == OPCODE_JP ||
             instruction == OPCODE_JO || instruction == OPCODE_JS ||
             instruction == OPCODE_JNE || instruction == OPCODE_JNL ||
             instruction == OPCODE_JG || instruction == OPCODE_JNB ||
             instruction == OPCODE_JA || instruction == OPCODE_JNP ||
             instruction == OPCODE_JNO || instruction == OPCODE_JNS ||
             instruction == OPCODE_LOOP || instruction == OPCODE_LOOPZ ||
             instruction == OPCODE_LOOPNZ || instruction == OPCODE_JCXZ) {
    switch (instruction) {
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
    ++memory_idx->offset;

    int8_t value = ParseValue(memory_idx, false, false) + 2;
    printf("$+0");
    if (value >= 0) {
      printf("+");
    }
    PrintValue(value, false);
  } else {
    INSTRUCTION_NOT_IMPLEMENTED();
  }
}
