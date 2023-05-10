#include "mips.h"
#include <stdio.h>
#include <stdlib.h>

const mips_op reg_op[32] = {
    {.kind = MIPS_OP_REG, .reg_o = {REG_ZE, 0}},
    {.kind = MIPS_OP_REG, .reg_o = {REG_AT, 0}},
    {.kind = MIPS_OP_REG, .reg_o = {REG_V0, 0}},
    {.kind = MIPS_OP_REG, .reg_o = {REG_V1, 0}},
    {.kind = MIPS_OP_REG, .reg_o = {REG_A0, 0}},
    {.kind = MIPS_OP_REG, .reg_o = {REG_A1, 0}},
    {.kind = MIPS_OP_REG, .reg_o = {REG_A2, 0}},
    {.kind = MIPS_OP_REG, .reg_o = {REG_A3, 0}},
    {.kind = MIPS_OP_REG, .reg_o = {REG_T0, 0}},
    {.kind = MIPS_OP_REG, .reg_o = {REG_T1, 0}},
    {.kind = MIPS_OP_REG, .reg_o = {REG_T2, 0}},
    {.kind = MIPS_OP_REG, .reg_o = {REG_T3, 0}},
    {.kind = MIPS_OP_REG, .reg_o = {REG_T4, 0}},
    {.kind = MIPS_OP_REG, .reg_o = {REG_T5, 0}},
    {.kind = MIPS_OP_REG, .reg_o = {REG_T6, 0}},
    {.kind = MIPS_OP_REG, .reg_o = {REG_T7, 0}},
    {.kind = MIPS_OP_REG, .reg_o = {REG_S0, 0}},
    {.kind = MIPS_OP_REG, .reg_o = {REG_S1, 0}},
    {.kind = MIPS_OP_REG, .reg_o = {REG_S2, 0}},
    {.kind = MIPS_OP_REG, .reg_o = {REG_S3, 0}},
    {.kind = MIPS_OP_REG, .reg_o = {REG_S4, 0}},
    {.kind = MIPS_OP_REG, .reg_o = {REG_S5, 0}},
    {.kind = MIPS_OP_REG, .reg_o = {REG_S6, 0}},
    {.kind = MIPS_OP_REG, .reg_o = {REG_S7, 0}},
    {.kind = MIPS_OP_REG, .reg_o = {REG_T8, 0}},
    {.kind = MIPS_OP_REG, .reg_o = {REG_T9, 0}},
    {.kind = MIPS_OP_REG, .reg_o = {REG_K0, 0}},
    {.kind = MIPS_OP_REG, .reg_o = {REG_K1, 0}},
    {.kind = MIPS_OP_REG, .reg_o = {REG_GP, 0}},
    {.kind = MIPS_OP_REG, .reg_o = {REG_SP, 0}},
    {.kind = MIPS_OP_REG, .reg_o = {REG_FP, 0}},
    {.kind = MIPS_OP_REG, .reg_o = {REG_RA, 0}},
};

mips_op *new_mips_op(int kind, int value, char *label, int reg, int offset) {
  mips_op *mp = calloc(1, sizeof(mips_op));
  mp->kind = kind;
  switch (kind) {
  case MIPS_OP_LABEL: {
    mp->label = label;
  } break;
  case MIPS_OP_REG: {
    free(mp);
    return (void *)(reg_op + kind);
  } break;
  case MIPS_OP_IMM: {
    mp->value = value;
  } break;
  case MIPS_OP_ADDR: {
    mp->reg_o.reg = reg;
    mp->reg_o.offset = offset;
  } break;
  default: {
    fprintf(stderr, "invalid op type: %d\n", kind);
    exit(EXIT_FAILURE);
  } break;
  }
  return mp;
}
mips_op *new_label_op(char *label) {
  return new_mips_op(MIPS_OP_LABEL, 0, label, 0, 0);
}
mips_op *new_reg_op(int reg) {
  return new_mips_op(MIPS_OP_REG, 0, NULL, reg, 0);
}
mips_op *new_imm_op(int value) {
  return new_mips_op(MIPS_OP_IMM, value, NULL, 0, 0);
}
mips_op *new_addr_op(int reg, int offset) {
  return new_mips_op(MIPS_OP_ADDR, 0, NULL, reg, offset);
}