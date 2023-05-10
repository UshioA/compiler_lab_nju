#include "ir.h"
#include "array.h"
#include "list.h"
#include "syntax.tab.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int tempcnt;
static int labelcnt;
intercode *ircode;
array *ir_list;

operand *new_v(int kind, int int_val, char *str_val) {
  operand *opr = calloc(1, sizeof(operand));
  opr->kind = kind;
  if (OPR_TMP <= kind && kind <= OPR_SIZE) {
    opr->imm = int_val;
  } else if (OPR_VAR <= kind && kind <= OPR_FUNC) {
    opr->varname = str_val;
  }
  return opr;
}

static intercode *new_ir() {
  intercode *ir = calloc(1, sizeof(intercode));
  list_init(&ir->link);
  return ir;
}

operand *new_tempvar() { return new_v(OPR_TMP, tempcnt++, NULL); }
operand *new_label() { return new_v(OPR_LABEL, labelcnt++, NULL); }
operand *new_imm(int imm) { return new_v(OPR_IMM, imm, NULL); }
operand *new_var(char *varname) { return new_v(OPR_VAR, -1, varname); }
operand *new_size(int size) { return new_v(OPR_SIZE, size, NULL); }
operand *new_func(char *funcname) { return new_v(OPR_FUNC, -1, funcname); }

intercode *new_label_ir(operand *label) {
  intercode *ir = new_ir();
  ir->kind = IR_LABEL;
  ir->label.label = label;
  return ir;
}
intercode *new_func_ir(operand *funcname) {
  intercode *ir = new_ir();
  ir->kind = IR_FUNCTION;
  ir->func.func = funcname;
  return ir;
}
intercode *new_assign_ir(operand *lhs, operand *rhs) {
  intercode *ir = new_ir();
  ir->kind = IR_ASSIGN;
  ir->binop.lhs = lhs;
  ir->binop.rhs = rhs;
  return ir;
}
intercode *new_arith_ir(int symbol, operand *src1, operand *src2,
                        operand *dest) {
  intercode *ir = new_ir();
  switch (symbol) {
  case PLUS: {
    ir->kind = IR_PLUS;
  } break;
  case MINUS: {
    ir->kind = IR_MINUS;
  } break;
  case STAR: {
    ir->kind = IR_MUL;
  } break;
  case DIV: {
    ir->kind = IR_DIV;
  } break;
  default: {
    fprintf(stderr, "invalid arith op\n");
    assert(0);
  }
  }
  ir->arith.dest = dest;
  ir->arith.src1 = src1;
  ir->arith.src2 = src2;
  return ir;
}
intercode *new_goto_ir(operand *to) {
  intercode *ir = new_ir();
  ir->kind = IR_GOTO;
  ir->go.to = to;
  return ir;
}
intercode *new_ifgoto_ir(char *relop, operand *rel1, operand *rel2,
                         operand *to) {
  intercode *ir = new_ir();
  ir->kind = IR_IF_GOTO;
  if (!strcmp(relop, ">")) {
    ir->ifgo.relop = REL_GT;
  } else if (!strcmp(relop, "<")) {
    ir->ifgo.relop = REL_LT;
  } else if (!strcmp(relop, ">=")) {
    ir->ifgo.relop = REL_GE;
  } else if (!strcmp(relop, "<=")) {
    ir->ifgo.relop = REL_LE;
  } else if (!strcmp(relop, "==")) {
    ir->ifgo.relop = REL_EQ;
  } else if (!strcmp(relop, "!=")) {
    ir->ifgo.relop = REL_NEQ;
  } else {
    fprintf(stderr, "invalid relop\n");
    assert(0);
  }
  ir->ifgo.rel1 = rel1;
  ir->ifgo.rel2 = rel2;
  ir->ifgo.to = to;
  return ir;
}
intercode *new_ret_ir(operand *ret) {
  intercode *ir = new_ir();
  ir->kind = IR_RET;
  ir->ret.ret = ret;
  return ir;
}
intercode *new_dec_ir(operand *arr, operand *size) {
  intercode *ir = new_ir();
  ir->kind = IR_DEC;
  ir->dec.arr = arr;
  ir->dec.size = size;
  return ir;
}
intercode *new_arg_ir(operand *arg) {
  intercode *ir = new_ir();
  ir->arg.arg = arg;
  ir->kind = IR_ARG;
  return ir;
}
intercode *new_call_ir(operand *func, operand *ret) {
  intercode *ir = new_ir();
  ir->kind = IR_CALL;
  ir->call.func = func;
  ir->call.ret = ret;
  return ir;
}
intercode *new_param_ir(operand *param) {
  intercode *ir = new_ir();
  ir->kind = IR_PARAM;
  ir->param.param = param;
  return ir;
}
intercode *new_read_ir(operand *to) {
  intercode *ir = new_ir();
  ir->kind = IR_READ;
  ir->read.to = to;
  return ir;
}
intercode *new_write_ir(operand *from) {
  intercode *ir = new_ir();
  ir->kind = IR_WRITE;
  ir->write.from = from;
  return ir;
}

void ir_pushback(intercode *p) {
  arr_push(ir_list, p);
  // list_add_tail(&p->link, &ircode->link);
}

static void op_dump(operand *op, FILE *f) {
  if (op->ref) {
    fprintf(f, "&");
  } else if (op->deref) {
    fprintf(f, "*");
  }
  switch (op->kind) {
  case OPR_TMP: {
    fprintf(f, "t%d", op->tempno);
  } break;
  case OPR_VAR: {
    fprintf(f, "v%s", op->varname);
  } break;
  case OPR_LABEL: {
    fprintf(f, "l%d", op->imm);
  } break;
  case OPR_FUNC: {
    if (strcmp(op->funcname, "main"))
      fprintf(f, "f");
    fprintf(f, "%s", op->funcname);
  } break;
  case OPR_IMM: {
    fprintf(f, "#%d", op->imm);
  } break;
  case OPR_SIZE: {
    fprintf(f, "%d", op->imm * __CMM_INT_SIZE__);
  } break;
  default: {
    fprintf(stderr, "invalid op kind\n");
    assert(0);
  }
  }
}

void ir_dump(intercode *ir, FILE *f) {
  switch (ir->kind) {
  case IR_PASS: {
  } break;
  case IR_LABEL: {
    fprintf(f, "LABEL ");
    op_dump(ir->label.label, f);
    fprintf(f, " :\n");
  } break;
  case IR_FUNCTION: {
    fprintf(f, "FUNCTION ");
    op_dump(ir->func.func, f);
    fprintf(f, " :\n");
  } break;
  case IR_ASSIGN: {
    op_dump(ir->binop.lhs, f);
    fprintf(f, " := ");
    op_dump(ir->binop.rhs, f);
    fprintf(f, "\n");
  } break;
  case IR_PLUS:
  case IR_MINUS:
  case IR_MUL:
  case IR_DIV: {
    static char _[7] = {
        [IR_PLUS] = '+', [IR_MUL] = '*', [IR_DIV] = '/', [IR_MINUS] = '-'};
    char op = _[ir->kind];
    op_dump(ir->arith.dest, f);
    fprintf(f, " := ");
    op_dump(ir->arith.src1, f);
    fprintf(f, " %c ", op);
    op_dump(ir->arith.src2, f);
    fprintf(f, "\n");
  } break;
  case IR_GOTO: {
    fprintf(f, "GOTO ");
    op_dump(ir->go.to, f);
    fprintf(f, "\n");
  } break;
  case IR_IF_GOTO: {
    fprintf(f, "IF ");
    static char *__[6] = {[REL_EQ] = "==", [REL_GE] = ">=", [REL_GT] = ">",
                          [REL_LE] = "<=", [REL_LT] = "<",  [REL_NEQ] = "!="};
    char *relop = __[ir->ifgo.relop];
    op_dump(ir->ifgo.rel1, f);
    fprintf(f, " %s ", relop);
    op_dump(ir->ifgo.rel2, f);
    fprintf(f, " GOTO ");
    op_dump(ir->ifgo.to, f);
    fprintf(f, "\n");
  } break;
  case IR_RET: {
    fprintf(f, "RETURN ");
    op_dump(ir->ret.ret, f);
    fprintf(f, "\n");
  } break;
  case IR_DEC: {
    fprintf(f, "DEC ");
    op_dump(ir->dec.arr, f);
    fprintf(f, " ");
    op_dump(ir->dec.size, f);
    fprintf(f, "\n");
  } break;
  case IR_ARG: {
    fprintf(f, "ARG ");
    op_dump(ir->arg.arg, f);
    fprintf(f, "\n");
  } break;
  case IR_CALL: {
    op_dump(ir->call.ret, f);
    fprintf(f, " := CALL ");
    op_dump(ir->call.func, f);
    fprintf(f, "\n");
  } break;
  case IR_PARAM: {
    fprintf(f, "PARAM ");
    op_dump(ir->arg.arg, f);
    fprintf(f, "\n");
  } break;
  case IR_READ: {
    fprintf(f, "READ ");
    op_dump(ir->arg.arg, f);
    fprintf(f, "\n");
  } break;
  case IR_WRITE: {
    fprintf(f, "WRITE ");
    op_dump(ir->arg.arg, f);
    fprintf(f, "\n");
  } break;
  default: {
    fprintf(stderr, "invalid ir type\n");
    assert(0);
  }
  }
}

void init_ircode() {
  ir_list = new_arr(512);
  // ircode = new_ir();
}