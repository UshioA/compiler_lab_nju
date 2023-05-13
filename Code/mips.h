#ifndef __CMM_MIPS_H__
#define __CMM_MIPS_H__

#include "ir.h"
#include "list.h"

#define __MIPS_REG_LIMIT__ 32

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
    int value; // for imm
    int labelno;
    struct {
      // if REG, use `reg', else if ADDR, use `reg' and `offset'
      enum mips_reg reg;
      int offset;
    } reg_o;
  };
} mips_op;

typedef struct mips_inst {
  enum {
    // one op
    MIPS_LABEL,
    MIPS_JAL,
    MIPS_J,
    MIPS_MFLO,
    MIPS_JR,
    // two op
    MIPS_LI,
    MIPS_MOVE,
    MIPS_DIV,
    MIPS_LW,
    MIPS_SW,
    MIPS_LA,
    // three op
    MIPS_ADD,
    MIPS_ADDI,
    MIPS_SUB,
    MIPS_MUL,
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

typedef struct {
  operand *op; // corresponding operand
  int offset;
  int regno;
  list_entry link;
  // my dear list library =). actually i thought about using newly implemented
  // array, nonetheless my array library can only be pushed and i dont think
  // it's worthy to implement `pop' and `shrink' since i am lazy. so i just
  // stick to my dear old list library =)
} var_desc;

typedef struct {
  enum mips_reg kind;
  enum {
    REG_FREE,
    REG_BUSY,
  } state;
  var_desc* hang;
} reg_desc;

const char *get_reg_name(int regno);

mips_op *new_mips_op(int kind, int value, int labelno, int reg, int offset);
mips_op *new_label_op(int labelno);
mips_op *new_reg_op(int reg);
mips_op *new_imm_op(int value);
mips_op *new_addr_op(int reg, int offset);

void init_codefile(FILE*);
void gencode();
void init_env();

extern const mips_op reg_op[__MIPS_REG_LIMIT__];
extern mips_inst *mips_head;
extern const char *reg_names[__MIPS_REG_LIMIT__];
extern var_desc curr_varlist;
extern int local_offset;
extern int argnum;
extern int paramnum;

#endif