#ifndef __CMM_IR_H__
#define __CMM_IR_H__

#include "array.h"
#include "ast.h"
#include "basic_blk.h"
#include "list.h"
#include <stdio.h>
#include <stdlib.h>
// #define __DEBUG_IR_LINENO__
#define __CMM_INT_SIZE__ 4

typedef struct {
  enum {
    OPR_TMP,
    OPR_LABEL,
    OPR_IMM,
    OPR_SIZE,
    OPR_VAR,
    OPR_FUNC,
  } kind;

  int deref;
  int ref;
  BB *corres_BB;
  int addr;
  int array;
  cmm_type *arr;

  union {
    int tempno;
    int imm;
    int size;
    struct {
      char *varname;
      int varno;
    };
    struct {
      char *funcname;
      int funcno;
    };
  };
} operand;

typedef struct {
  enum {
    IR_LABEL,
    IR_FUNCTION,
    IR_ASSIGN,
    IR_PLUS,
    IR_MINUS,
    IR_MUL,
    IR_DIV,
    IR_GOTO,
    IR_IF_GOTO,
    IR_RET,
    IR_DEC,
    IR_ARG,
    IR_CALL,
    IR_PARAM,
    IR_READ,
    IR_WRITE,
    IR_PASS,
  } kind;

  union {
    struct {
      operand *label;
    } label;
    struct {
      operand *func;
    } func;
    struct {
      operand *rhs, *lhs;
    } binop;
    struct {
      operand *dest;
      operand *src1, *src2;
    } arith;
    struct {
      operand *to;
    } go;
    struct {
      operand *rel1, *rel2;
      enum {
        REL_LT,
        REL_GT,
        REL_LE,
        REL_GE,
        REL_EQ,
        REL_NEQ,
      } relop;
      operand *to;
    } ifgo;
    struct {
      operand *ret;
    } ret;
    struct {
      operand *arr;
      operand *size;
    } dec;
    struct {
      operand *arg;
    } arg;
    struct {
      operand *func;
      operand *ret;
    } call;
    struct {
      operand *param;
    } param;
    struct {
      operand *to;
    } read;
    struct {
      operand *from;
    } write;
  };
  list_entry link;
} intercode;

extern intercode *ircode;
extern array *ir_list;

void init_ircode();
operand *new_v(int kind, int int_val, char *str_val, int no);
operand *new_tempvar();
operand *new_label();

operand *new_imm(int imm);
operand *new_var(char *varname, int no);
operand *new_size(int size);
operand *new_func(char *funcname, int);

intercode *new_label_ir(operand *label);
intercode *new_func_ir(operand *funcname);
intercode *new_assign_ir(operand *lhs, operand *rhs);
intercode *new_arith_ir(int kind, operand *src1, operand *src2, operand *dest);
intercode *new_goto_ir(operand *to);
intercode *new_ifgoto_ir(char *relop, operand *rel1, operand *rel2,
                         operand *to);
intercode *new_ret_ir(operand *ret);
intercode *new_dec_ir(operand *arr, operand *size);
intercode *new_arg_ir(operand *arg);
intercode *new_call_ir(operand *func, operand *ret);
intercode *new_param_ir(operand *param);
intercode *new_read_ir(operand *to);
intercode *new_write_ir(operand *from);

void ir_pushback(intercode *);
void op_dump(operand *op, FILE *f);
void ir_dump(intercode *p, FILE *f);
#endif