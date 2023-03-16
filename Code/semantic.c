#include "semantic.h"
#include "ast.h"
#include "syntax.tab.h"
#include "type.h"
frame *global;
frame *currf; // this name is bad =)
void do_semantic(ast_node *root) {
  init_semantic();
  if (root)
    program(root);
}

void program(ast_node *root) {
  ast_node *_extdeflist = get_child_n(root, 0);
  extdeflist(_extdeflist);
}

void extdeflist(ast_node *root) {
  ast_node *_extdef = get_child_n(root, 0);
  ast_node *_extdeflist = get_child_n(root, 1);
  extdef(_extdef); // guaranteed by grammar.
  if (_extdeflist) {
    extdeflist(_extdeflist);
  }
}

void extdef(ast_node *root) {
  ast_node *_specifier = get_child_n(root, 0);
  ast_node *ch2 = get_child_n(root, 1);
  ast_node *ch3 = get_child_n(root, 2);
  cmm_type *spec;
  if (ch2->symbol == ExtDecList) {
    spec = specifier(_specifier);
    extdeclist(ch2);
  } else if (ch2->symbol == SEMI) {
    spec = specifier(_specifier);
  } else {
    spec = specifier(_specifier);
    enter_scope();
    cmm_type *paramlist = fundec(ch2, spec);
    compst(ch3, spec);
    exit_scope();
  }
}

cmm_type *specifier(ast_node *root) {
  ast_node *ch1 = get_child_n(root, 0);
  if (ch1->symbol == TYPE) {
    return new_cmm_btype(
        new_literal(!strcmp("int", ch1->value.str_val) ? INT : FLOAT));
  } else {
    return structspecifier(ch1);
  }
}

void extdeclist(ast_node *root);

cmm_type *fundec(ast_node *root, cmm_type *rtype);

void compst(ast_node *root, cmm_type *rtype);

cmm_type *structspecifier(ast_node *root) {
  ast_node *deflist = get_child_n(root, 3);
  if (!deflist) {
    ast_node *opttag = get_child_n(root, 1);
  }
}