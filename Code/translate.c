#include "translate.h"
#include "ast.h"
#include "frame.h"
#include "ir.h"
#include "semantic.h"
#include "symtab.h"
#include "syntax.tab.h"

static const char *trans_error =
    "Cannot translate: Code contains variables or parameters of structure "
    "type.";

static void translate_error() {
  fputs(trans_error, stderr);
  exit(0);
}

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