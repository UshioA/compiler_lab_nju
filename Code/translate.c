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

static void translate_error() {
  fputs(trans_error, stderr);
  exit(0);
}

enum cond { COND_RELOP, COND_NOT, COND_AND, COND_OR, COND_OTHER };

void init_file(FILE *_f) { f = _f; }

void emit_code(intercode *ir) { ir_pushback(ir); }

void dump_code() {
  intercode *pos = calloc(1, sizeof(intercode));
  list_entry *head = &ircode->link;
  list_for_each_item(pos, head, link) { ir_dump(pos, f); }
}

void translate_program(ast_node *root) {
  if (root) {
    translate_extdeflist(get_child_n(root, 0));
  }
}

void translate_extdeflist(ast_node *root) {
  if (root->childnum == 2) {
    translate_extdef(get_child_n(root, 0));
    translate_extdeflist(get_child_n(root, 1));
  }
}

void translate_extdef(ast_node *root) {
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
  ir_pushback(fun);
  if (root->childnum == 4) {
    translate_varlist(get_child_n(root, 2));
  }
}

void translate_compst(ast_node *root) {
  translate_deflist(get_child_n(root, 1));
  translate_stmtlist(get_child_n(root, 2));
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
      ir_pushback(ir);
    } else {
      if (sym->type->ctype == TYPE_ARR) {
        int sz = 0;
        for (int i = 1; i < sym->dimension[0]; ++i) {
          sz += sym->dimension[i];
        }
        intercode *dec = new_dec_ir(new_var(sym->name), new_size(sz));
        ir_pushback(dec);
      }
    }
    return sym;
  } else {
    return translate_vardec(first, isfunc); // can get info from id
  }
}

void translate_deflist(ast_node *root) {
  if (root->childnum == 2) {
    translate_def(get_child_n(root, 0));
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
    translate_expr(get_child_n(root, 2), tmp);
    intercode *assign = new_assign_ir(new_var(sym->name), tmp);
    ir_pushback(assign);
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
  ir_pushback(new_assign_ir(place, new_imm(0)));
  translate_cond(exp, label1, label2, get_cond(exp));
  ir_pushback(new_label_ir(label1));
  ir_pushback(new_assign_ir(place, new_imm(1)));
  ir_pushback(new_label_ir(label2));
}

void translate_cond(ast_node *exp, operand *label_true, operand *label_false,
                    int cond) {
  switch (cond) {
  case COND_RELOP: {
    operand *t1 = new_tempvar(), *t2 = new_tempvar();
    translate_expr(get_child_n(exp, 0), t1);
    translate_expr(get_child_n(exp, 2), t2);
    char *op = get_child_n(exp, 1)->value.str_val;
    ir_pushback(new_ifgoto_ir(op, t1, t2, label_true));
    ir_pushback(new_goto_ir(label_false));
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
    ir_pushback(new_label_ir(label1));
    translate_cond(exp2, label_true, label_false, get_cond(exp2));
  } break;
  case COND_OR: {
    operand *label1 = new_label();
    ast_node *exp1 = get_child_n(exp, 0);
    ast_node *exp2 = get_child_n(exp, 2);
    translate_cond(exp1, label_true, label1, get_cond(exp1));
    ir_pushback(new_label_ir(label1));
    translate_cond(exp2, label_true, label_false, get_cond(exp2));
  } break;
  case COND_OTHER: {
    operand *t1 = new_tempvar();
    translate_expr(exp, t1);
    ir_pushback(new_ifgoto_ir("!=", t1, new_imm(0), label_true));
    ir_pushback(new_goto_ir(label_false));
  } break;
  }
}

void translate_expr(ast_node *root, operand *tmp) {
  switch (root->childnum) {
  case 1: { // ID INT FLOAT
    ast_node *ch = get_child_last(root);
    switch (ch->symbol) {
    case ID: {
      symbol *sym = frame_lookup(global, ch->value.str_val);
      if (sym->type->is_basetype && sym->type->btype->dectype == STRUCT)
        translate_error();
      operand *op = new_var(sym->name);
      if (!sym->type->is_basetype && sym->type->ctype == TYPE_ARR) {
        op->ref = 1;
      }
      intercode *id = new_assign_ir(tmp, op);
      ir_pushback(id);
    } break;
    case INT: {
      int v = ch->value.int_val;
      intercode *integer = new_assign_ir(tmp, new_imm(v));
      ir_pushback(integer);
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
      translate_expr(exp, t1);
      intercode *minus = new_arith_ir(MINUS, new_imm(0), t1, tmp);
      ir_pushback(minus);
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
        ir_pushback(new_read_ir(tmp));
      } else {
        ir_pushback(new_call_ir(new_func(sym->name), tmp));
      }
    } break;
    case LP: { // bullshit =)
      translate_expr(get_child_n(root, 1), tmp);
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
        if (head->child->symbol == ID) {
          symbol *sym = frame_lookup(global, head->child->value.str_val);
          operand *t1 = new_tempvar();
          translate_expr(get_child_n(root, 2), t1);
          operand *var = new_var(sym->name);
          ir_pushback(new_assign_ir(var, t1));
          ir_pushback(new_assign_ir(tmp, var));
        } else
          translate_error(); // TODO
      } break;
      case RELOP: {
        handle_cond(root, tmp);
      } break;
      case PLUS:
      case MINUS:
      case STAR:
      case DIV: {
        operand *t1 = new_tempvar(), *t2 = new_tempvar();
        translate_expr(head, t1);
        translate_expr(get_child_last(root), t2);
        ir_pushback(new_arith_ir(ch2->symbol, t1, t2, tmp));
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
        ir_pushback(new_write_ir(arglist[0]));
        ir_pushback(new_assign_ir(tmp, new_imm(0)));
      } else {
        for (int i = 0; i < sym->type->contain_len; ++i) {
          ir_pushback(new_arg_ir(arglist[i]));
        }
        ir_pushback(new_call_ir(new_func(sym->name), tmp));
      }
    } else if (head->symbol == Exp) {

    } else
      assert(0);
  } break;
  default:
    assert(0);
  }
}

void translate_args(ast_node *root, operand **arg_list, int index) {
  operand *t1 = new_tempvar();
  translate_expr(root->child, t1);
  arg_list[index] = t1;
  if (root->childnum == 3) {
    translate_args(get_child_n(root, 2), arg_list, index - 1);
  } else
    assert(root->childnum == 1);
}

void translate_stmtlist(ast_node* root){
  assert(0);
}