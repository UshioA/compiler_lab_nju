#include "opt.h"
#include "array.h"
#include "basic_blk.h"
#include "bitset.h"
#include "cfg.h"
#include "ir.h"
#include "live.h"
#include <assert.h>
#include <stdio.h>

array *live_ctx;
array *entry_live_ctx;
array *exit_live_ctx;

void init_live() {
  int varcnt = maxtempcnt + varno + 1;
  live_ctx = new_live_ctx(ir_list->length, varcnt);
  entry_live_ctx = new_live_ctx(cfg_list->length, varcnt);
  exit_live_ctx = new_live_ctx(cfg_list->length, varcnt);
}

live_info *get_ir_live(int idx) { return arr_get(idx, live_ctx); }
live_info *get_entry_live(int idx) { return arr_get(idx, entry_live_ctx); }
live_info *get_exit_live(int idx) { return arr_get(idx, exit_live_ctx); }

void do_live(int idx) {
  // if (idx < 2)
  //   return;
  cfg *cg = arr_get(idx, cfg_list);
  int changed;
  do {
    changed = 0;
    for (int i = cg->node->length - 1; i >= 0; --i) {
      changed |= live_transfer_bb(arr_get(i, cg->node), cg);
    }
  } while (changed);
}

int live_transfer_bb(BB *node, cfg *g) {
  if (!node->reachable)
    return 0;
  int changed = 0;
  if (node->kind == BB_EXIT)
    return 0;
  array *succ = get_successor(g, node);
  bitset *to;
  if (node->kind == BB_ENTRY) {
    to = get_entry_live(g->funid)->OUT;
  } else {
    to = get_ir_live(node->end - 1)->OUT;
  }
  // printf("merging ");
  for (int i = 0; i < succ->length; ++i) {
    bitset *from;
    BB *s;
    s = arr_get(*(int *)arr_get(i, succ), g->node);
    // printf("%d ", s->blkid);
    if (s->kind == BB_EXIT) {
      from = get_exit_live(g->funid)->IN;
    } else {
      from = get_ir_live(s->beg)->IN;
    }
    bitset_union(to, from);
  }
  // printf("to %d\n", node->blkid);
  for (int i = node->end - 1; i >= node->beg; --i) {
    changed |= live_transfer_ir(arr_get(i, ir_list), i, i == node->end - 1);
  }
  return changed;
}

static int getno(operand *op) {
  // printf("getno: ");
  // op_dump(op, stdout);
  // printf("\n");
  // getchar();
  if (op->kind == OPR_TMP) {
    return op->tempno + varno;
  } else if (op->kind == OPR_VAR) {
    return op->varno;
  } else if (op->kind == OPR_IMM)
    return -1;
  assert(0);
}

static int get_use(operand *op) {
  if (op->kind == OPR_IMM)
    return -1;
  return getno(op);
}

// static int get_def(operand *lhs, operand *rhs1, operand *rhs2) {
//   if (lhs->kind != rhs1->kind || getno(lhs) != getno(rhs1)) {
//     if (rhs2) {
//       if (lhs->kind != rhs2->kind || getno(lhs) != getno(rhs2)) {
//         return getno(lhs);
//       }
//       return -1;
//     } else {
//       return getno(lhs);
//     }
//   } else
//     return -1;
// }

static void remove_insert(bitset *out, int def, int use1, int use2, int use3) {
  // printf("{def: %d, use1: %d, use2: %d, use3: %d}\n", def, use1, use2, use3);
  if (def != -1)
    bitset_remove(out, def);
  if (use1 != -1)
    bitset_insert(out, use1);
  if (use2 != -1)
    bitset_insert(out, use2);
  if (use3 != -1)
    bitset_insert(out, use3);
}

int live_transfer_ir(intercode *ir, int at, int end) {
  if (!end) {
    bitset_assign(get_ir_live(at)->OUT, get_ir_live(at + 1)->IN);
  }
  bitset *out1 = bitset_clone(get_ir_live(at)->OUT);
  bitset *in = get_ir_live(at)->IN;
  int use = -1;
  int def = -1;
  switch (ir->kind) {
  case IR_ASSIGN: {
    // if (2 <= at && at <= 11) {
    //   printf("[%d]: has v0? %ld\n", at, bitset_contain(out1, 0));
    // }
    operand *lhs = ir->binop.lhs;
    operand *rhs = ir->binop.rhs;
    int lclear = !lhs->ref && !lhs->deref;
    int rclear = !rhs->ref && !rhs->deref;
    int easy_opt = lclear && rclear;
    if (easy_opt) {
      use = get_use(rhs);
      def = getno(lhs);
    } else {
      if (lclear) {
        use = get_use(rhs);
        def = getno(lhs);
      } else {
        int u1 = get_use(lhs), u2 = get_use(rhs);
        remove_insert(out1, -1, u1, u2, -1);
        int changed = bitset_assign(in, out1);
        return changed;
      }
    }
    remove_insert(out1, def, use, -1, -1);
    int changed = bitset_assign(in, out1);
    // if (2 <= at && at <= 11) {
    //   printf("[%d]: now out has v0? %ld\n", at, bitset_contain(out1, 0));
    //   printf("[%d]: in has v0? %ld\n", at, bitset_contain(in, 0));
    // }
    return changed;
  } break;
  case IR_PLUS:
  case IR_MINUS:
  case IR_MUL:
  case IR_DIV: {
    operand *dest = ir->arith.dest;
    operand *src1 = ir->arith.src1;
    operand *src2 = ir->arith.src2;
    int use1, use2, use3;
    int def = -1;
    use1 = use2 = use3 = -1;
    if (dest->deref) {
      use3 = getno(dest);
    } else {
      def = getno(dest);
    }
    use1 = get_use(src1);
    use2 = get_use(src2);
    remove_insert(out1, def, use1, use2, use3);
    int changed = bitset_assign(in, out1);
    return changed;
  } break;
  case IR_IF_GOTO: {
    operand *lhs = ir->ifgo.rel1;
    operand *rhs = ir->ifgo.rel2;
    remove_insert(out1, -1, get_use(lhs), get_use(rhs), -1);
    int changed = bitset_assign(in, out1);
    return changed;
  } break;
  case IR_RET: {
    operand *ret = ir->ret.ret;
    remove_insert(out1, -1, get_use(ret), -1, -1);
    return bitset_assign(in, out1);
  } break;
  case IR_DEC: {
    operand *x = ir->dec.arr;
    remove_insert(out1, getno(x), -1, -1, -1);
    return bitset_assign(in, out1);
  } break;
  case IR_ARG: {
    operand *arg = ir->arg.arg;
    remove_insert(out1, -1, get_use(arg), -1, -1);
    return bitset_assign(in, out1);
  } break;
  case IR_CALL: {
    operand *ret = ir->call.ret;
    if (ret->deref) {
      remove_insert(out1, -1, get_use(ret), -1, -1);
    } else {
      remove_insert(out1, getno(ret), -1, -1, -1);
    }
    return bitset_assign(in, out1);
  } break;
  case IR_READ: {
    operand *to = ir->read.to;
    if (to->deref) {
      remove_insert(out1, -1, get_use(to), -1, -1);
    } else {
      remove_insert(out1, getno(to), -1, -1, -1);
    }
    return bitset_assign(in, out1);
  } break;
  case IR_WRITE: {
    operand *from = ir->write.from;
    remove_insert(out1, -1, get_use(from), -1, -1);
    return bitset_assign(in, out1);
  } break;
  default:
    break;
  }
  return bitset_assign(in, out1);
}

int live_remove_useless() {
  int fruitful = 0;
  for (int i = 2; i < cfg_list->length; ++i) {
    int changed = 0;
    do {
      changed = live_remove_useless_cfg(i);
      fruitful |= changed;
      if (changed) {
        do_live(i);
      }
    } while (changed);
  }
  return fruitful;
}

int live_remove_useless_cfg(int idx) {
  cfg *g = arr_get(idx, cfg_list);
  if (!g)
    return 0;
  if (!g->reachable)
    return 0;
  int changed = 0;
  for (int i = 0; i < g->node->length; ++i) {
    changed |= live_remove_useless_bb(arr_get(i, g->node), g);
  }
  return changed;
}

int live_remove_useless_bb(BB *node, cfg *g) {
  if (!node->reachable)
    return 0;
  if (node->kind == BB_EXIT)
    return 0;
  int changed = 0;
  for (int i = node->beg; i < node->end; ++i) {
    intercode *ir = arr_get(i, ir_list);
    switch (ir->kind) {
    case IR_ASSIGN:
    case IR_PLUS:
    case IR_MINUS:
    case IR_MUL:
    case IR_DIV: {
      operand *lhs = ir->kind == IR_ASSIGN ? ir->binop.lhs : ir->arith.dest;
      if (!lhs->deref && !bitset_contain(get_ir_live(i)->OUT, getno(lhs))) {
        // printf("removed:\n\033[33m[%d]\033[0m: ", i);
        // ir_dump(ir, stdout);
        // getchar();
        changed = 1;
        ir->kind = IR_PASS;
      }
    } break;
    case IR_DEC: {
      operand *lhs = ir->dec.arr;
      if (!bitset_contain(get_ir_live(i)->OUT, getno(lhs))) {
        changed = 1;
        ir->kind = IR_PASS;
      }
    } break;
    default:
      break;
    }
  }
  return changed;
}

int live_merge_assign() {
  int fruitful = 0;
  for (int i = 2; i < cfg_list->length; ++i) {
    int changed = 0;
    do {
      changed = live_merge_assign_cfg(i);
      fruitful |= changed;
      if (changed) {
        do_live(i);
      }
    } while (changed);
  }
  return fruitful;
}

int live_merge_assign_cfg(int idx) {
  cfg *g = arr_get(idx, cfg_list);
  if (!g)
    return 0;
  if (!g->reachable)
    return 0;
  int changed = 0;
  for (int i = 0; i < g->node->length; ++i) {
    changed |= live_merge_assign_bb(arr_get(i, g->node), g);
  }
  return changed;
}

int live_merge_assign_bb(BB *node, cfg *g) {
  if (!node->reachable)
    return 0;
  if (node->kind == BB_EXIT)
    return 0;
  int changed = 0;
  for (int i = node->beg; i < node->end; ++i) {
    int j = i + 1;
    while (j < node->end && ((intercode *)ir_list->elem[j])->kind == IR_PASS)
      ++j;
    if (j >= node->end)
      continue;
    intercode *this = ir_list->elem[i];
    intercode *next = ir_list->elem[j];
    if (next->kind != IR_ASSIGN)
      continue;
    if (next->binop.rhs->kind == OPR_IMM)
      continue;
    operand *rhs = next->binop.rhs;
    switch (this->kind) {
    case IR_ASSIGN: {
      // printf("found (%d, %d)\n", i, j);
      // ir_dump(this, stdout);
      // ir_dump(next, stdout);
      // getchar();
      operand *lhs = this->binop.lhs;
      if (lhs->deref || lhs->kind == OPR_IMM)
        continue;
      int lno = getno(lhs);
      if (lhs->deref != rhs->deref || rhs->ref || lno != getno(rhs))
        continue;
      if (next->binop.lhs->deref)
        continue;
      if (lhs->deref || bitset_contain(get_ir_live(j)->OUT, lno))
        continue;
      next->kind = IR_ASSIGN;
      next->binop.rhs = this->binop.rhs;
      this->kind = IR_PASS; // mark as nop
      changed = 1;
    } break;
    case IR_PLUS:
    case IR_MINUS:
    case IR_MUL:
    case IR_DIV: {
      operand *lhs = this->arith.dest;
      int lno = getno(lhs);
      if (lhs->deref != rhs->deref || rhs->ref || lno != getno(rhs))
        continue;
      if (lhs->deref || bitset_contain(get_ir_live(j)->OUT, lno))
        continue;
      operand *v = next->binop.lhs;
      if (v->deref)
        continue;
      next->kind = this->kind;
      // memset(&next->binop, 0, sizeof(next->binop));
      next->arith.src1 = this->arith.src1;
      next->arith.src2 = this->arith.src2;
      next->arith.dest = v;
      this->kind = IR_PASS;
      changed = 1;
    } break;
    case IR_CALL: {
      // TODO
    } break;
    case IR_READ: {
      // TODO
    } break;
    default:
      break;
    }
  }
  return changed;
}