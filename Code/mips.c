#include "mips.h"
#include "array.h"
#include "basic_blk.h"
#include "bitset.h"
#include "cfg.h"
#include "ir.h"
#include "list.h"
#include "syntax.tab.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
var_desc curr_varlist;

static bitset *vars;
extern int varno, funcno, maxtempcnt;
int local_offset;
int argnum;
int paramnum;
static reg_desc regs[__MIPS_REG_LIMIT__];
static void init_regs() {
  for (int i = 0; i < __MIPS_REG_LIMIT__; ++i) {
    regs[i].kind = i;
    regs[i].state = REG_FREE;
    regs->hang = NULL;
    // list_init(regs[i].hang.link);
  }
}
static FILE *code;
static const char start[] = ".data\n"
                            "_prompt: .asciiz \"Enter an integer:\"\n"
                            "_ret: .asciiz \"\\n\"\n"
                            ".globl main\n"
                            ".text\n"
                            "f1:\n"
                            "li $v0, 4\n"
                            "la $a0, _prompt\n"
                            "syscall\n"
                            "li $v0, 5\n"
                            "syscall\n"
                            "jr $ra\n"
                            "f0:\n"
                            "li $v0, 1\n"
                            "syscall\n"
                            "li $v0, 4\n"
                            "la $a0, _ret\n"
                            "syscall\n"
                            "move $v0, $0\n"
                            "jr $ra\n";

const char *reg_names[__MIPS_REG_LIMIT__] = {
    [0] = "zero", [1] = "at",  [2] = "v0",  [3] = "v1",  [4] = "a0",
    [5] = "a1",   [6] = "a2",  [7] = "a3",  [8] = "t0",  [9] = "t1",
    [10] = "t2",  [11] = "t3", [12] = "t4", [13] = "t5", [14] = "t6",
    [15] = "t7",  [16] = "s0", [17] = "s1", [18] = "s2", [19] = "s3",
    [20] = "s4",  [21] = "s5", [22] = "s6", [23] = "s7", [24] = "t8",
    [25] = "t9",  [26] = "k0", [27] = "k1", [28] = "gp", [29] = "sp",
    [30] = "fp",  [31] = "ra",
};
const mips_op reg_op[__MIPS_REG_LIMIT__] = {
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

const char *get_reg_name(int regno) {
  assert(0 <= regno && regno <= __MIPS_REG_LIMIT__);
  return reg_names[regno];
}

mips_op *new_mips_op(int kind, int value, int label, int reg, int offset) {
  mips_op *mp = calloc(1, sizeof(mips_op));
  mp->kind = kind;
  switch (kind) {
  case MIPS_OP_LABEL: {
    mp->labelno = label;
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
mips_op *new_label_op(int label) {
  return new_mips_op(MIPS_OP_LABEL, 0, label, 0, 0);
}
mips_op *new_reg_op(int reg) { return new_mips_op(MIPS_OP_REG, 0, 0, reg, 0); }
mips_op *new_imm_op(int value) {
  return new_mips_op(MIPS_OP_IMM, value, 0, 0, 0);
}
mips_op *new_addr_op(int reg, int offset) {
  return new_mips_op(MIPS_OP_ADDR, 0, 0, reg, offset);
}

void init_codefile(FILE *f) {
  assert(f);
  code = f;
}

void init_env() {
  init_regs();
  init_cfg_list();
  build_cfg(make_node_lists(make_func_blk()));
  vars = new_bitset(varno + maxtempcnt + 1);
  fprintf(code, start);
}

static void insert_var(var_desc *v) {
  list_add(&v->link, &curr_varlist.link);
  operand *op = v->op;
  if (op->kind == OPR_TMP) {
    bitset_insert(vars, op->tempno + varno);
  } else {
    bitset_insert(vars, op->varno);
  }
}

static var_desc *findv(operand *op) {
  assert(op);
  if (op->kind == OPR_IMM || op->kind == OPR_SIZE)
    return NULL;
  if (op->kind == OPR_TMP) {
    if (!bitset_contain(vars, op->tempno + varno))
      return NULL;
  } else {
    if (!bitset_contain(vars, op->varno))
      return NULL;
  }
  list_entry *pos;
  list_for_each(pos, &curr_varlist.link) {
    var_desc *cur = le2(var_desc, pos, link);
    if (op->kind != cur->op->kind)
      continue;
    switch (op->kind) {
    case OPR_TMP: {
      if (cur->op->tempno == op->tempno)
        return cur;
    } break;
    case OPR_VAR: {
      if (cur->op->varno == op->varno)
        return cur;
    } break;
    default:
      continue;
    }
  }
  return NULL;
}

static void insert_op(operand *op) {
  assert(op);
  if (op->kind == OPR_IMM)
    return;
  if (!findv(op)) {
    local_offset -= 4;
    var_desc *v = calloc(1, sizeof(var_desc));
    v->offset = local_offset;
    v->regno = -1;
    v->op = op;
    insert_var(v);
  }
}

static void make_offset_ir(intercode *ir) {
  assert(ir);
  switch (ir->kind) {
  case IR_LABEL:
  case IR_FUNCTION:
  case IR_GOTO: {
  } break;
  case IR_PARAM: {
    var_desc *v = calloc(1, sizeof(var_desc));
    assert(v);
    v->regno = -1;
    v->op = ir->param.param;
    v->offset = 8 + (paramnum << 2);
    ++paramnum;
    insert_var(v);
  } break;
  case IR_RET:
  case IR_ARG:
  case IR_READ:
  case IR_WRITE: {
    // to specify struct name and member seems tedious, this
    // should do the same work, less graceful though.
    insert_op(ir->ret.ret);
  } break;
  case IR_DEC: {
    local_offset -= 4 * (ir->dec.size->imm - 1);
    insert_op(ir->dec.arr);
  } break;
  case IR_ASSIGN: {
    insert_op(ir->binop.lhs);
    insert_op(ir->binop.rhs);
  } break;
  case IR_CALL: {
    insert_op(ir->call.ret);
  } break;
  case IR_PLUS:
  case IR_MINUS:
  case IR_MUL:
  case IR_DIV: {
    insert_op(ir->arith.dest);
    insert_op(ir->arith.src1);
    insert_op(ir->arith.src2);
  } break;
  case IR_IF_GOTO: {
    insert_op(ir->ifgo.rel1);
    insert_op(ir->ifgo.rel2);
  } break;
  default:;
  }
}

static void make_offset_bb(BB *b) {
  assert(b);
  if (b->kind != BB_NODE)
    return; // filter ENTRY and EXIT
  for (int i = b->beg; i < b->end; ++i) {
    make_offset_ir(arr_get(i, ir_list));
  }
}

static void init_varlist() {
  list_init(&curr_varlist.link);
  local_offset = 0;
}

static void make_offset_func(array *bblist) {
  assert(bblist);
  local_offset = argnum = paramnum = 0;
  for (int i = 0; i < bblist->length; ++i) {
    make_offset_bb(arr_get(i, bblist));
  }
}

static int get_reg(operand *op, int left, FILE *f) {
  assert(op);
  int i;
  for (i = REG_T0; i < REG_T8; ++i) {
    if (regs[i].state == REG_FREE)
      break;
  }
  assert(i < REG_T8);
  regs[i].state = REG_BUSY;
  if (op->kind == OPR_IMM) {
    fprintf(f, "li $%s, %d\n", get_reg_name(i), op->imm);
  } else {
    var_desc *v = findv(op);
    assert(v);
    v->regno = i;
    regs[i].hang = v;
    switch (op->kind) {
    case OPR_TMP:
    case OPR_VAR: {
      if (op->ref) {
        fprintf(code, "addu $%s, $fp, %d\n", get_reg_name(i), v->offset);
      } else {
        if (!left) {
          fprintf(code, "lw $%s, %d($fp)\n", get_reg_name(i), v->offset);
        }
      }
    } break;
    default:
      break;
    }
  }
  return i;
}

static void free_reg(int reg) {
  regs[reg].state = REG_FREE;
  regs[reg].hang = NULL;
}

static void store_reg(int reg, FILE *f) {
  assert(reg > 0);
  if (regs[reg].hang) {
    assert(regs[reg].hang->offset != -1);
    fprintf(code, "sw $%s, %d($fp)\n", get_reg_name(reg),
            regs[reg].hang->offset);
  }
  free_reg(reg);
}

static void gen_ifgoto(intercode *ir) {
  assert(ir && ir->kind == IR_IF_GOTO);
  operand *rel1 = ir->ifgo.rel1, *rel2 = ir->ifgo.rel2;
  int r1 = get_reg(ir->ifgo.rel1, 0, code);
  int r2 = get_reg(ir->ifgo.rel2, 0, code);
  static char *__[6] = {[REL_EQ] = "beq", [REL_GE] = "bge", [REL_GT] = "bgt",
                        [REL_LE] = "ble", [REL_LT] = "blt", [REL_NEQ] = "bne"};
  assert(ir->ifgo.relop <= REL_NEQ && ir->ifgo.relop >= REL_LT);
  fprintf(code, "%s, $%s, $%s, ", __[ir->ifgo.relop], get_reg_name(r1),
          get_reg_name(r2));
  op_dump(ir->ifgo.to, code);
  fprintf(code, "\n");
  free_reg(r1);
  free_reg(r2);
}

static void gen_arith(intercode *ir) {
  assert(ir);
  int dst = get_reg(ir->arith.dest, 1, code);
  int src1 = get_reg(ir->arith.src1, 0, code);
  int src2 = get_reg(ir->arith.src2, 0, code);
  static char *opl[] = {[IR_PLUS] = "add",
                        [IR_MINUS] = "sub",
                        [IR_MUL] = "mul",
                        [IR_DIV] = "div"};
  assert(ir->kind <= IR_DIV && ir->kind >= IR_PLUS);
  char *op = opl[ir->kind];
  if (ir->kind == IR_DIV) {
    fprintf(code, "%s, $%s, $%s\n", op, get_reg_name(src1), get_reg_name(src2));
    fprintf(code, "mflo $%s\n", get_reg_name(dst));
  } else {
    fprintf(code, "%s $%s, $%s, $%s\n", op, get_reg_name(dst),
            get_reg_name(src1), get_reg_name(src2));
  }
  store_reg(dst, code);
  free_reg(src1);
  free_reg(src2);
}

static void gen_call(intercode *ir) {
  assert(ir && ir->kind == IR_CALL);
  fprintf(code, "addu $sp, $sp, -8\n");
  fprintf(code, "sw $fp, 0($sp)\n");
  fprintf(code, "sw $ra, 4($sp)\n");
  fprintf(code, "jal ");
  op_dump(ir->call.func, code);
  fprintf(code, "\n");
  fprintf(code, "move $sp, $fp\n");
  fprintf(code, "lw $ra, 4($sp)\n");
  fprintf(code, "lw $fp, 0($sp)\n");
  fprintf(code, "addu $sp, $sp, %d\n", 8 + (argnum << 2));
  argnum = 0;
  int reg = get_reg(ir->call.ret, 1, code);
  fprintf(code, "move $%s, $v0\n", get_reg_name(reg));
  store_reg(reg, code);
}

static void gen_assign(intercode *ir) {
  assert(ir && ir->kind == IR_ASSIGN);
  int lstar = ir->binop.lhs->deref;
  int rstar = ir->binop.rhs->deref;
  if (lstar) {
    if (rstar) {
      int t7 = REG_T7; // t7 is not used, for i am naive;
      assert(regs[t7].state == REG_FREE);
      int x = get_reg(ir->binop.lhs, 0, code);
      int y = get_reg(ir->binop.rhs, 0, code);
      fprintf(code, "lw $%s, 0($%s)\n", get_reg_name(t7), get_reg_name(y));
      fprintf(code, "sw $%s, 0($%s)\n", get_reg_name(t7), get_reg_name(x));
      free_reg(x);
      free_reg(y);
    } else {
      int x = get_reg(ir->binop.lhs, 0, code);
      int y = get_reg(ir->binop.rhs, 0, code);
      fprintf(code, "sw $%s, 0($%s)\n", get_reg_name(y), get_reg_name(x));
      free_reg(x);
      free_reg(y);
    }
  } else if (rstar) {
    int x = get_reg(ir->binop.lhs, 1, code);
    int y = get_reg(ir->binop.rhs, 0, code);
    fprintf(code, "lw $%s, 0($%s)\n", get_reg_name(x), get_reg_name(y));
    store_reg(x, code);
    free_reg(y);
  } else {
    int x = get_reg(ir->binop.lhs, 0, code);
    int y = get_reg(ir->binop.rhs, 0, code);
    fprintf(code, "move $%s, $%s\n", get_reg_name(x), get_reg_name(y));
    store_reg(x, code);
    free_reg(y);
  }
}

static void gen_dec(intercode *ir) { assert(ir && ir->kind == IR_DEC); }

static void gen_write(intercode *ir) {
  assert(ir && ir->kind == IR_WRITE);
  int reg = get_reg(ir->write.from, 0, code);
  fprintf(code, "move $a0, $%s\n", get_reg_name(reg));
  fprintf(code, "addu $sp, $sp, -4\n");
  fprintf(code, "sw $ra, 0($sp)\n");
  fprintf(code, "jal f0\n");
  fprintf(code, "lw $ra, 0($sp)\n");
  fprintf(code, "addu $sp, $sp, 4\n");
  free_reg(reg);
}

static void gen_read(intercode *ir) {
  assert(ir && ir->kind == IR_READ);
  fprintf(code, "addu $sp, $sp, -4\n");
  fprintf(code, "sw $ra, 0($sp)\n");
  fprintf(code, "jal f1\n");
  fprintf(code, "lw $ra, 0($sp)\n");
  fprintf(code, "addu $sp, $sp, 4\n");
  int reg = get_reg(ir->read.to, 1, code);
  fprintf(code, "move $%s, $v0\n", get_reg_name(reg));
  store_reg(reg, code);
}

static void gen_param(intercode *ir) { assert(ir && ir->kind == IR_PARAM); }

static void gen_arg(intercode *ir) {
  assert(ir && ir->kind == IR_ARG);
  argnum++;
  int reg = get_reg(ir->arg.arg, 0, code);
  fprintf(code, "addu $sp, $sp, -4\n");
  fprintf(code, "sw $%s, 0($sp)\n", get_reg_name(reg));
  free_reg(reg);
}

static void gen_return(intercode *ir) {
  assert(ir && ir->kind == IR_RET);
  int reg = get_reg(ir->ret.ret, 0, code);
  fprintf(code, "move $v0, $%s\n", get_reg_name(reg));
  fprintf(code, "jr $ra\n");
  free_reg(reg);
}

static void gen_goto(intercode *ir) {
  assert(ir && ir->kind == IR_GOTO);
  fprintf(code, "j ");
  op_dump(ir->go.to, code);
  fprintf(code, "\n");
}

static void gen_func(intercode *ir) {
  assert(ir && ir->kind == IR_FUNCTION);
  printf("function: %s\n", ir->func.func->funcname);
  op_dump(ir->func.func, code);
  fprintf(code, ":\n");
  fprintf(code, "move $fp, $sp\n");
  fprintf(code, "addu $sp, $sp, %d\n", local_offset);
}

static void gen_label(intercode *ir) {
  assert(ir && ir->kind == IR_LABEL);
  op_dump(ir->label.label, code);
  fprintf(code, ":\n");
}

static void gen_ir(intercode *ir) {
  assert(ir);
  switch (ir->kind) {
  case IR_LABEL: {
    gen_label(ir);
  } break;
  case IR_FUNCTION: {
    gen_func(ir);
  } break;
  case IR_GOTO: {
    gen_goto(ir);
  } break;
  case IR_RET: {
    gen_return(ir);
  } break;
  case IR_ARG: {
    gen_arg(ir);
  } break;
  case IR_PARAM: {
    gen_param(ir);
  } break;
  case IR_READ: {
    gen_read(ir);
  } break;
  case IR_WRITE: {
    gen_write(ir);
  } break;
  case IR_DEC: {
    gen_dec(ir);
  } break;
  case IR_ASSIGN: {
    gen_assign(ir);
  } break;
  case IR_CALL: {
    gen_call(ir);
  } break;
  case IR_PLUS:
  case IR_MINUS:
  case IR_MUL:
  case IR_DIV: {
    gen_arith(ir);
  } break;
  case IR_IF_GOTO: {
    gen_ifgoto(ir);
  } break;
  case IR_PASS:
    break;
  default:
    assert(0);
  }
}

static void gen_bb(BB *b) {
  assert(b);
  if (b->kind != BB_NODE)
    return;
  argnum = 0;
  for (int i = b->beg; i < b->end; ++i) {
    gen_ir(arr_get(i, ir_list));
  }
}

static void gen_cfg(array *bblist) {
  assert(bblist);
  argnum = 0;
  for (int i = 0; i < bblist->length; ++i) {
    gen_bb(arr_get(i, bblist));
  }
}

void gencode() {
  init_env();
  for (int i = 0; i < cfg_list->length; ++i) {
    cfg *g = arr_get(i, cfg_list);
    if (g) { // read and write are NULL, filter
      init_varlist();
      make_offset_func(g->node);
      gen_cfg(g->node);
    }
  }
}