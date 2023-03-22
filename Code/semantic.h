#ifndef __CMM_SEMANTIC_H__
#define __CMM_SEMANTIC_H__
#include "ast.h"
#include "common.h"
#include "frame.h"
#include "list.h"
#include "symtab.h"
#include "type.h"

extern frame *global;
extern frame *currf; // this name is bad =)
extern cmm_type *temphead;
static inline void error(int errno, int lineno) {
  fprintf(stdout, "Error type %d at Line %d: .\n", errno, lineno);
}

static inline void init_global() {
  global = make_frame(NULL, 1);
  currf = global;
}

static inline void init_tempnode() {
  temphead = new_cmm_btype(new_literal(INT));
  list_init(&temphead->link);
}

static inline void init_semantic() {
  init_tempnode();
  init_global();
}

static inline void enter_scope() { currf = make_frame(currf, 1); }

static inline void exit_scope() { currf = currf->parent; }

void do_semantic(ast_node *root);

static void program(ast_node *root);

static void extdeflist(ast_node *root);

static void extdef(ast_node *root);

static cmm_type *specifier(ast_node *root, cmm_type *_struct);

static void extdeclist(ast_node *root, cmm_type *spec);

static cmm_type *fundec(ast_node *root, cmm_type *rtype);

static void compst(ast_node *root, cmm_type *rtype);

static cmm_type *structspecifier(ast_node *root, cmm_type *_struct);

static char *opttag(ast_node *root);

static cmm_type *deflist(ast_node *root, int isfield, cmm_type *_struct);

static cmm_type *def(ast_node *root, int isfield, cmm_type *_struct);

static cmm_type *declist(ast_node *root, cmm_type *spec, int isfield,
                         cmm_type *_struct);

static void dec(ast_node *root, cmm_type *spec, int isfield, cmm_type *_struct);

static void varlist(ast_node *root, cmm_type *_v);

static symbol *vardec(ast_node *root, cmm_type *type, int isfield,
                      cmm_type *_struct);

static cmm_type *expr(ast_node *root);

static void paramdec(ast_node *root, cmm_type *_v);

static cmm_type* args(ast_node *root, cmm_type *functype);

static void stmtlist(ast_node *, cmm_type *);

static cmm_type *args_ass(ast_node *root, cmm_type *paramtypes);

static void stmt(ast_node *, cmm_type *);

#endif