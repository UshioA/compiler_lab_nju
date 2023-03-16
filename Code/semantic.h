#ifndef __CMM_SEMANTIC_H__
#define __CMM_SEMANTIC_H__
#include "ast.h"
#include "common.h"
#include "frame.h"
#include "symtab.h"
#include "type.h"

extern frame *global;
extern frame *currf; // this name is bad =)

static inline void error(int errno, int lineno) {
  fprintf(stdout, "Error type %d at Line %d: .", errno, lineno);
}

static inline void init_global() {
  global = make_frame(NULL, 1);
  currf = global;
}
static inline void init_semantic() { init_global(); }

static inline void enter_scope() { currf = make_frame(currf, 1); }

static inline void exit_scope() { currf = currf->parent; }

void do_semantic(ast_node *root);

void program(ast_node *root);

void extdeflist(ast_node *root);

void extdef(ast_node *root);

cmm_type *specifier(ast_node *root);

void extdeclist(ast_node *root);

cmm_type *fundec(ast_node *root, cmm_type *rtype);

void compst(ast_node *root, cmm_type *rtype);

cmm_type *structspecifier(ast_node *root);



#endif