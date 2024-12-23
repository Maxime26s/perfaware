#pragma once
#include "stdint.h"

#define OPCODE_MOV_RM2REG 0b100010
#define OPCODE_MOV_IMM2RM 0b1100011
#define OPCODE_MOV_IMM2REG 0b1011
#define OPCODE_MOV_MEM2ACC 0b101000
#define OPCODE_MOV_ACC2MEM 0b101000

#define OPCODE_ADD_RM2REG 0b000000
#define OPCODE_ADD_IMM2RM 0b100000
#define OPCODE_ADD_IMM2ACC 0b0000010

#define OPCODE_SUB_RM2REG 0b001010
#define OPCODE_SUB_IMM2RM 0b100000
#define OPCODE_SUB_IMM2ACC 0b0010110

#define OPCODE_CMP_RM2REG 0b001110
#define OPCODE_CMP_IMM2RM 0b100000
#define OPCODE_CMP_IMM2ACC 0b0011110

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

enum OpMnemonic {
  Op_None,

  Op_mov,
  Op_add,
  Op_sub,
  Op_cmp,
  Op_je,
  Op_jl,
  Op_jle,
  Op_jb,
  Op_jbe,
  Op_jp,
  Op_jo,
  Op_js,
  Op_jne,
  Op_jnl,
  Op_jg,
  Op_jnb,
  Op_ja,
  Op_jnp,
  Op_jno,
  Op_jns,
  Op_loop,
  Op_loopz,
  Op_loopnz,
  Op_jcxz
};

enum BitType {
  Bit_Literal,
  Bit_Mod,
  Bit_Reg,
  Bit_RM,
  Bit_Data,
  Bit_Displacement,
  Bit_Address,

  Bit_Wide,
  Bit_Signed,
  Bit_Destination,

  Bit_RelativeJmpAddress,

  Bit_Count,
};

struct InstructionBit {
  BitType type;
  uint8_t size;
  uint8_t value;
};

struct InstructionEncoding {

  OpMnemonic op;
  InstructionBit bits[16];
};

enum RegisterName {
  Register_a,
  Register_c,
  Register_d,
  Register_b,
  Register_sp,
  Register_bp,
  Register_si,
  Register_di,
  Register_es,
  Register_cs,
  Register_ss,
  Register_ds,
  Register_is,
};

struct RegisterInfo {
  RegisterName name;
  uint8_t size;
  uint8_t offset;
};

enum EffectiveAddressBase {
  EffectiveAddress_bx_si,
  EffectiveAddress_bx_di,
  EffectiveAddress_bp_si,
  EffectiveAddress_bp_di,
  EffectiveAddress_si,
  EffectiveAddress_di,
  EffectiveAddress_bp,
  EffectiveAddress_bx,
  EffectiveAddress_direct
};

struct EffectiveAddress {
  EffectiveAddressBase base;
  uint16_t displacement;
  uint8_t is_wide;
};

enum OperandType {
  Operand_None,
  Operand_Register,
  Operand_Memory,
  Operand_Immediate,
  Operand_RelativeImmediate,
};

struct Operand {
  OperandType type;
  union {
    RegisterInfo reg;
    EffectiveAddress address;
    uint32_t immediate_u32;
    int32_t immediate_s32;
  };
};

struct Instruction {
  uint32_t address;
  uint32_t size;

  OpMnemonic op;
  Operand operands[2];
};
