#include "translate.h"
#include "ast.h"
#include "frame.h"
#include "ir.h"
#include "list.h"
#include "semantic.h"
#include "symtab.h"
#include "syntax.tab.h"
#include <assert.h>
#include <stdint.h>

static const char *trans_error =
    "Cannot translate: Code contains variables or parameters of structure "
    "type.";

FILE *f;

void init_translate(FILE *_f) {
  init_file(_f);
  init_ircode();
}

static void translate_error() {
  fputs(trans_error, stderr);
  exit(0);
}

enum cond { COND_RELOP, COND_NOT, COND_AND, COND_OR, COND_OTHER };

void init_file(FILE *_f) { f = _f; }

void emit_code(intercode *ir) { ir_pushback(ir); }

void dump_code() {
  intercode *pos;
  list_entry *head = &ircode->link;
  list_for_each_item(pos, head, link) { ir_dump(pos, f); }
}

void translate_program(ast_node *root) {
  if (root) {
    translate_extdeflist(get_child_n(root, 0));
  }
}

void translate_extdeflist(ast_node *root) {
  if (!root)
    return;
  translate_extdef(get_child_n(root, 0));
  translate_extdeflist(get_child_n(root, 1));
}

static void translate_specifier(ast_node *root) {
  ast_node *ch1 = get_child_n(root, 0);
  if (ch1->symbol != TYPE)
    translate_error();
}

void translate_extdef(ast_node *root) {
  ast_node *structcheck = get_child_n(root, 0);
  translate_specifier(structcheck);
  ast_node *globalcheck = get_child_n(root, 1);
  if (globalcheck->symbol == ExtDecList) {
    fprintf(stderr,
            "你是一个傻逼, 因为你使用了全局变量\n"); // to make me happy =)
    exit(0);                                         // to make oj happy =)
  }
  if (globalcheck->symbol == FunDec) {
    translate_fundec(globalcheck);
    translate_compst(get_child_n(root, 2));
  }
}

void translate_fundec(ast_node *root) {
  intercode *fun = new_func_ir(new_func(root->child->value.str_val));
  emit_code(fun);
  if (root->childnum == 4) {
    translate_varlist(get_child_n(root, 2));
  }
}

void translate_compst(ast_node *root) {
  ast_node *_deflist = get_child_n(root, 1);
  if (_deflist->symbol == DefList) {
    translate_deflist(_deflist);
    ast_node *_stmtlist = get_child_n(root, 2);
    if (_stmtlist->symbol == StmtList) {
      translate_stmtlist(_stmtlist);
    }
  } else if (_deflist->symbol == StmtList)
    translate_stmtlist(_deflist);
}

void translate_varlist(ast_node *root) {
  translate_paramdec(get_child_n(root, 0));
  if (root->childnum > 1)
    translate_varlist(get_child_n(root, 2));
}

void translate_paramdec(ast_node *root) {
  translate_vardec(get_child_n(root, 1), 1);
}

symbol *translate_vardec(ast_node *root, int isfunc) {
  ast_node *first = get_child_n(root, 0);
  if (first->symbol == ID) {
    symbol *sym = frame_lookup(global, first->value.str_val);
    if (sym->type->is_basetype && sym->type->btype->dectype == STRUCT)
      translate_error();
    if (isfunc) {
      intercode *ir = new_param_ir(new_var(sym->name));
      emit_code(ir);
    } else {
      if (sym->type->ctype == TYPE_ARR) {
        int sz = 1;
        for (int i = 1; i < sym->dimension[0] + 1; ++i) {
          sz = sz * (sym->dimension[i]);
        }
        intercode *dec = new_dec_ir(new_var(sym->name), new_size(sz));
        emit_code(dec);
      }
    }
    return sym;
  } else {
    return translate_vardec(first, isfunc); // can get info from id
  }
}

void translate_deflist(ast_node *root) {
  if (!root)
    return;
  translate_def(get_child_n(root, 0));
  if (root->childnum == 2) {
    translate_deflist(get_child_n(root, 1));
  }
}

void translate_def(ast_node *root) {
  if (root->child->child->symbol == STRUCT)
    translate_error();
  translate_declist(get_child_n(root, 1));
}

void translate_declist(ast_node *root) {
  ast_node *dec = root->child;
  ast_node *declist = get_child_n(root, 2);
  translate_dec(dec);
  if (declist)
    translate_declist(declist);
}

void translate_dec(ast_node *root) {
  symbol *sym = translate_vardec(get_child_n(root, 0), 0);
  if (root->childnum == 3) {
    operand *tmp = new_tempvar();
    translate_expr(get_child_n(root, 2), tmp, 1, NULL);
    if (tmp->array && tmp->addr) {
      tmp = new_v(tmp->kind, tmp->tempno, NULL);
      tmp->deref = 1;
    }
    intercode *assign = new_assign_ir(new_var(sym->name), tmp);
    emit_code(assign);
  }
}

static int get_cond(ast_node *root) {
  switch (root->childnum) {
  case 1: { // ID INT FLOAT
    return COND_OTHER;
  } break;
  case 2: { // MINUS EXP | NOT EXP
    ast_node *op = get_child_n(root, 0);
    if (op->symbol == MINUS) {
      return COND_OTHER;
    } else if (op->symbol == NOT) {
      return COND_NOT;
    } else
      assert(0);
  }
  case 3: {
    ast_node *head = get_child_n(root, 0);
    switch (head->symbol) {
    case ID: { // function call
      return COND_OTHER;
    } break;
    case LP: { // bullshit =)
      return COND_OTHER;
    } break;
    case Exp: {
      ast_node *ch2 = get_child_n(root, 1);
      switch (ch2->symbol) {
      case DOT:
      case ASSIGNOP:
      case PLUS:
      case MINUS:
      case STAR:
      case DIV: {
        return COND_OTHER;
      }
      case AND: {
        return COND_AND;
      }
      case OR: {
        return COND_OR;
      }
      case RELOP: {
        return COND_RELOP;
      }
      default:
        assert(0);
      }
    }
    }
  } break;
  case 4: {
    return COND_OTHER;
  } break;
  default:
    assert(0);
  }
  assert(0);
}

static void handle_cond(ast_node *exp, operand *place) {
  operand *label1 = new_label(), *label2 = new_label();
  emit_code(new_assign_ir(place, new_imm(0)));
  translate_cond(exp, label1, label2, get_cond(exp));
  emit_code(new_label_ir(label1));
  emit_code(new_assign_ir(place, new_imm(1)));
  emit_code(new_label_ir(label2));
}

void translate_cond(ast_node *exp, operand *label_true, operand *label_false,
                    int cond) {
  switch (cond) {
  case COND_RELOP: {
    operand *t1 = new_tempvar(), *t2 = new_tempvar();
    translate_expr(get_child_n(exp, 0), t1, 1, NULL);
    translate_expr(get_child_n(exp, 2), t2, 1, NULL);
    if (t1->addr) {
      if (t2->addr) {
        operand *tt1 = new_v(t1->kind, t1->tempno, NULL);
        operand *tt2 = new_v(t2->kind, t2->tempno, NULL);
        tt1->deref = tt2->deref = 1;
        emit_code(new_assign_ir(t1, tt1));
        emit_code(new_assign_ir(t2, tt2));
      } else {
        operand *tt1 = new_v(t1->kind, t1->tempno, NULL);
        tt1->deref = 1;
        emit_code(new_assign_ir(t1, tt1));
      }
    } else {
      if (t2->addr) {
        operand *tt2 = new_v(t2->kind, t2->tempno, NULL);
        tt2->deref = 1;
        emit_code(new_assign_ir(t2, tt2));
      }
    }
    char *op = get_child_n(exp, 1)->value.str_val;
    emit_code(new_ifgoto_ir(op, t1, t2, label_true));
    emit_code(new_goto_ir(label_false));
  } break;
  case COND_NOT: {
    translate_cond(get_child_last(exp), label_false, label_true,
                   get_cond(get_child_last(exp)));
  } break;
  case COND_AND: {
    operand *label1 = new_label();
    ast_node *exp1 = get_child_n(exp, 0);
    ast_node *exp2 = get_child_n(exp, 2);
    translate_cond(exp1, label1, label_false, get_cond(exp1));
    emit_code(new_label_ir(label1));
    translate_cond(exp2, label_true, label_false, get_cond(exp2));
  } break;
  case COND_OR: {
    operand *label1 = new_label();
    ast_node *exp1 = get_child_n(exp, 0);
    ast_node *exp2 = get_child_n(exp, 2);
    translate_cond(exp1, label_true, label1, get_cond(exp1));
    emit_code(new_label_ir(label1));
    translate_cond(exp2, label_true, label_false, get_cond(exp2));
  } break;
  case COND_OTHER: {
    operand *t1 = new_tempvar();
    translate_expr(exp, t1, 1, NULL);
    if (t1->addr && t1->array) {
      t1 = new_v(t1->kind, t1->tempno, NULL);
      t1->deref = 1;
    }
    emit_code(new_ifgoto_ir("!=", t1, new_imm(0), label_true));
    emit_code(new_goto_ir(label_false));
  } break;
  }
}

static uint32_t prodsuffix(uint32_t *arr, int end) {
  uint32_t p = 1;
  if (!arr)
    return p;
  for (int i = 1; i < arr[0]; ++i) {
    p *= arr[i];
  }
  return p;
}

void translate_expr(ast_node *root, operand *tmp, int pass, operand *reuse) {
  switch (root->childnum) {
  case 1: { // ID INT FLOAT
    ast_node *ch = get_child_last(root);
    switch (ch->symbol) {
    case ID: {
      symbol *sym = frame_lookup(global, ch->value.str_val);
      if (sym->type->is_basetype && sym->type->btype->dectype == STRUCT)
        translate_error();
      operand *op = new_var(sym->name);
      if (!sym->type->is_basetype && sym->type->ctype == TYPE_ARR &&
          !sym->is_arg) {
        op->ref = 1;
      }
      if (pass) {
        intercode *id = new_assign_ir(tmp, op);
        emit_code(id);
      }
    } break;
    case INT: {
      int v = ch->value.int_val;
      if (pass) {
        intercode *integer = new_assign_ir(tmp, new_imm(v));
        emit_code(integer);
      }
    } break;
    case FLOAT: {
      fprintf(stderr, "对不起, 建议你玩原神\n");
      assert(0);
    } break;
    default:
      assert(0);
    }
  } break;
  case 2: { // MINUS EXP | NOT EXP
    ast_node *op = get_child_n(root, 0);
    if (op->symbol == MINUS) {
      operand *t1 = new_tempvar();
      ast_node *exp = get_child_last(root);
      if (exp->child->symbol == INT) {
        emit_code(new_assign_ir(tmp, new_imm(-exp->child->value.int_val)));
        return;
      }
      translate_expr(exp, t1, 1, NULL);
      if (t1->addr) {
        operand *tt1 = new_v(t1->kind, t1->tempno, NULL);
        tt1->deref = 1;
        emit_code(new_assign_ir(t1, tt1));
      }
      intercode *minus = new_arith_ir(MINUS, new_imm(0), t1, tmp);
      emit_code(minus);
    } else if (op->symbol == NOT) {
      handle_cond(root, tmp);
    } else
      assert(0);
  }
  case 3: {
    ast_node *head = get_child_n(root, 0);
    switch (head->symbol) {
    case ID: { // function call
      symbol *sym = frame_lookup(global, head->value.str_val);
      if (!strcmp(sym->name, "read")) {
        emit_code(new_read_ir(tmp));
      } else {
        emit_code(new_call_ir(new_func(sym->name), tmp));
      }
    } break;
    case LP: { // bullshit =)
      translate_expr(get_child_n(root, 1), tmp, 1, NULL);
    } break;
    case Exp: {
      ast_node *ch2 = get_child_n(root, 1);
      switch (ch2->symbol) {
      case DOT: {
        translate_error();
      } break;
      case AND:
      case OR: {
        handle_cond(root, tmp);
      } break;
      case ASSIGNOP: {
        if (head->childnum == 1 && head->child->symbol == ID) {
          symbol *sym = frame_lookup(global, head->child->value.str_val);
          operand *t1 = tmp;
          operand *var = new_var(sym->name);
          ast_node *exp2 = get_child_n(root, 2);
          if (exp2->childnum == 1 && exp2->child->symbol == INT) {
            emit_code(new_assign_ir(var, new_imm(exp2->child->value.int_val)));
            *tmp = *var;
          } else if (exp2->childnum == 1 && exp2->child->symbol == ID) {
            symbol *sym2 = frame_lookup(global, exp2->child->value.str_val);
            operand *op = new_var(sym2->name);
            if (!sym2->type->is_basetype && sym2->type->ctype == TYPE_ARR &&
                !sym2->is_arg)
              op->ref = 1;
            emit_code(new_assign_ir(var, op));
            *tmp = *var;
            // if (pass)
            //   emit_code(new_assign_ir(tmp, var));
          } else {
            translate_expr(exp2, t1, 1, NULL);
            if (t1->array && t1->addr) {
              t1 = new_v(t1->kind, t1->tempno, NULL);
              t1->deref = 1;
            }
            emit_code(new_assign_ir(var, t1));
          }
        } else {
          operand *t1 = tmp;
          operand *t2 = new_tempvar();
          translate_expr(head, t1, 1, NULL);
          translate_expr(get_child_n(root, 2), t2, 1, NULL);
          if (t1->addr) {
            if (t2->addr) {
              operand *tt1 = new_v(OPR_TMP, t1->tempno, NULL);
              operand *tt2 = new_v(OPR_TMP, t2->tempno, NULL);
              tt2->deref = 1;
              emit_code(new_assign_ir(t2, tt2));
              tt1->deref = 1;
              emit_code(new_assign_ir(tt1, t2));
            } else {
              operand *tt1 = new_v(OPR_TMP, t1->tempno, NULL);
              tt1->deref = 1;
              emit_code(new_assign_ir(tt1, t2));
            }
          } else {
            if (t2->addr) {
              operand *tt2 = new_v(OPR_TMP, t2->tempno, NULL);
              tt2->deref = 1;
              emit_code(new_assign_ir(t1, tt2));
            } else {
              emit_code(new_assign_ir(t1, t2));
            }
          }
        }
      } break;
      case RELOP: {
        handle_cond(root, tmp);
      } break;
      case PLUS:
      case MINUS:
      case STAR:
      case DIV: {
        operand *t1 = new_tempvar(), *t2 = new_tempvar();
        translate_expr(head, t1, 1, NULL);
        translate_expr(get_child_last(root), t2, 1, NULL);
        if (pass) {
          if (t1->addr) {
            if (t2->addr) {
              operand *tt1 = new_v(t1->kind, t1->tempno, NULL);
              operand *tt2 = new_v(t2->kind, t2->tempno, NULL);
              tt1->deref = tt2->deref = 1;
              emit_code(new_assign_ir(t1, tt1));
              emit_code(new_assign_ir(t2, tt2));
            } else {
              operand *tt1 = new_v(t1->kind, t1->tempno, NULL);
              tt1->deref = 1;
              emit_code(new_assign_ir(t1, tt1));
            }
          } else {
            if (t2->addr) {
              operand *tt2 = new_v(t2->kind, t2->tempno, NULL);
              tt2->deref = 1;
              emit_code(new_assign_ir(t2, tt2));
            }
          }
          emit_code(new_arith_ir(ch2->symbol, t1, t2, tmp));
        }
      } break;
      default:
        assert(0);
      }
    }
    }
  } break;
  case 4: {
    ast_node *head = get_child_n(root, 0);
    if (head->symbol == ID) {
      symbol *sym = frame_lookup(global, head->value.str_val);
      operand **arglist = calloc(sym->type->contain_len, sizeof(operand));
      translate_args(get_child_n(root, 2), arglist, sym->type->contain_len - 1);
      if (!strcmp(sym->name, "write")) {
        emit_code(new_write_ir(arglist[0]));
        if (pass)
          emit_code(new_assign_ir(tmp, new_imm(0)));
      } else {
        for (int i = 0; i < sym->type->contain_len; ++i) {
          emit_code(new_arg_ir(arglist[i]));
        }
        emit_code(new_call_ir(new_func(sym->name), tmp));
      }
    } else if (head->symbol == Exp) {
      operand *t2 = reuse;
      if (!t2)
        t2 = new_tempvar();
      ast_node *exp = get_child_n(root, 2);
      cmm_type *arrtype = exp->arrtype;
      uint32_t width = arrtype ? 4 * prodsuffix(arrtype->dimensions, 2) : 0;
      translate_expr(head, tmp, 1, t2);
      if (exp->child && exp->child->symbol == INT) {
        emit_code(new_arith_ir(STAR, new_imm(width),
                               new_imm(exp->child->value.int_val), t2));
      } else {
        translate_expr(exp, t2, 1, NULL);
        emit_code(new_arith_ir(STAR, t2, new_imm(width), t2));
      }
      if (pass)
        emit_code(new_arith_ir(PLUS, tmp, t2, tmp));
      tmp->array = 1;
      if (arrtype->contain_len <= 1)
        tmp->addr = 1;
    } else
      assert(0);
  } break;
  default:
    assert(0);
  }
}

void translate_args(ast_node *root, operand **arg_list, int index) {
  operand *t1 = new_tempvar();
  translate_expr(root->child, t1, 1, NULL);
  if (t1->addr) {
    operand *tt1 = new_v(OPR_TMP, t1->tempno, NULL);
    tt1->deref = 1;
    emit_code(new_assign_ir(t1, tt1));
  }
  arg_list[index] = t1;
  if (root->childnum == 3) {
    translate_args(get_child_n(root, 2), arg_list, index - 1);
  } else
    assert(root->childnum == 1);
}

void translate_stmtlist(ast_node *root) {
  if (!root)
    return;
  ast_node *stmt = get_child_n(root, 0);
  ast_node *stmtl = get_child_n(root, 1);
  translate_stmt(stmt);
  translate_stmtlist(stmtl);
}

void translate_stmt(ast_node *root) {
  if (!root)
    return;
  ast_node *head = get_child_n(root, 0);
  switch (head->symbol) {
  case Exp: {
    operand *tmp = new_tempvar();
    translate_expr(head, tmp, 0, NULL);
  } break;
  case CompSt: {
    translate_compst(head);
  } break;
  case RETURN: {
    operand *t1 = new_tempvar();
    translate_expr(get_child_n(root, 1), t1, 1, NULL);
    if (t1->array && t1->addr) {
      t1 = new_v(t1->kind, t1->tempno, NULL);
      t1->deref = 1;
    }
    emit_code(new_ret_ir(t1));
  } break;
  case WHILE: {
    operand *label1 = new_label(), *label2 = new_label(), *label3 = new_label();
    emit_code(new_label_ir(label1));
    translate_cond(get_child_n(root, 2), label2, label3,
                   get_cond(get_child_n(root, 2)));
    emit_code(new_label_ir(label2));
    translate_stmt(get_child_last(root));
    emit_code(new_goto_ir(label1));
    emit_code(new_label_ir(label3));
  } break;
  case IF: {
    operand *label1 = new_label(), *label2 = new_label();
    operand *label3;
    if (get_child_last(root) != get_child_n(root, 4))
      label3 = new_label();
    translate_cond(get_child_n(root, 2), label1, label2,
                   get_cond(get_child_n(root, 2)));
    emit_code(new_label_ir(label1));
    translate_stmt(get_child_n(root, 4));
    if (get_child_last(root) != get_child_n(root, 4))
      emit_code(new_goto_ir(label3));
    emit_code(new_label_ir(label2));
    if (get_child_last(root) != get_child_n(root, 4)) {
      translate_stmt(get_child_last(root));
      emit_code(new_label_ir(label3));
    }
  } break;
  default: {
    fprintf(stderr, "what is this %s\n", head->value.str_val);
    assert(0);
  } break;
  }
}