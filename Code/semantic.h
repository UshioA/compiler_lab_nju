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

static void program(ast_node *root);

static void extdeflist(ast_node *root);

static void extdef(ast_node *root);

static cmm_type *specifier(ast_node *root);

static void extdeclist(ast_node *root, cmm_type *spec);

static cmm_type *fundec(ast_node *root, cmm_type *rtype);

static void compst(ast_node *root, cmm_type *rtype);

static cmm_type *structspecifier(ast_node *root);

static char *opttag(ast_node *root);

static cmm_type *deflist(ast_node *root, int isfield);

static cmm_type *def(ast_node *root, int isfield);

static cmm_type *declist(ast_node *root, cmm_type *spec, int isfield);

static void dec(ast_node *root, cmm_type *spec, int isfield);

static void varlist(ast_node *root, cmm_type *_v);

static symbol *vardec(ast_node *root, cmm_type *type, int isfield);

static cmm_type *exp(ast_node *root);

static void paramdec(ast_node *root, cmm_type *_v);

#endif