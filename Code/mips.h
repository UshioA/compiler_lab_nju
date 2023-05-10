#ifndef __CMM_MIPS_H__
#define __CMM_MIPS_H__

#include "list.h"
enum mips_reg {
  REG_ZE = 0,
  REG_AT,
  REG_V0,
  REG_V1,
  REG_A0,
  REG_A1,
  REG_A2,
  REG_A3,
  REG_T0,
  REG_T1,
  REG_T2,
  REG_T3,
  REG_T4,
  REG_T5,
  REG_T6,
  REG_T7,
  REG_S0,
  REG_S1,
  REG_S2,
  REG_S3,
  REG_S4,
  REG_S5,
  REG_S6,
  REG_S7,
  REG_T8,
  REG_T9,
  REG_K0,
  REG_K1,
  REG_GP,
  REG_SP,
  REG_FP,
  REG_RA
};

typedef struct mips_op {
  enum { MIPS_OP_LABEL, MIPS_OP_REG, MIPS_OP_IMM, MIPS_OP_ADDR } kind;
  union {
    int value;   // for imm
    char *label; // for LABEL
    struct {
      // if REG, use `reg', else if ADDR, use `reg' and `offset'
      enum mips_reg reg;
      int offset;
    } reg_o;
  };
} mips_op;

typedef struct mips_inst {
  enum {
    MIPS_LABEL,
    MIPS_LI,
    MIPS_LA,
    MIPS_MOVE,
    MIPS_ADD,
    MIPS_ADDI,
    MIPS_SUB,
    MIPS_MUL,
    MIPS_DIV,
    MIPS_MFLO,
    MIPS_LW,
    MIPS_SW,
    MIPS_J,
    MIPS_JAL,
    MIPS_JR,
    MIPS_BEQ,
    MIPS_BNE,
    MIPS_BGT,
    MIPS_BLT,
    MIPS_BGE,
    MIPS_BLE
  } kind;

  list_entry link; // my dear list library

  union {
    struct {
      mips_op *dest, *src1, *src2;
    };
    mips_op *op;
  };
} mips_inst;

mips_op *new_mips_op(int kind, int value, char *label, int reg, int offset);
mips_op *new_label_op(char *label);
mips_op *new_reg_op(int reg);
mips_op *new_imm_op(int value);
mips_op *new_addr_op(int reg, int offset);

extern const mips_op reg_op[32];
extern mips_inst *mips_head;
#endif