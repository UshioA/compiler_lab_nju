#ifndef __CMM_AST_H__
#define __CMM_AST_H__
#include "common.h"
#include <stdint.h>
typedef struct cmm_type cmm_type;
typedef enum value_type {
  NONE_TYPE,
  INT_TYPE,
  FLOAT_TYPE,
  STR_TYPE
} value_type;
extern int yylineno;
extern int yyleng;
typedef enum nonterminal_type {
  Program,
  ExtDefList,
  ExtDef,
  ExtDecList,
  Specifier,
  StructSpecifier,
  OptTag,
  Tag,
  VarDec,
  FunDec,
  VarList,
  ParamDec,
  CompSt,
  StmtList,
  Stmt,
  DefList,
  Def,
  DecList,
  Dec,
  Exp,
  Args
} nonterminal_type;

typedef union ast_value {
  uint32_t int_val;
  float float_val;
  char *str_val; // actually it's identifier but haha.
} ast_value;

typedef struct ast_node {
  int symbol;
  value_type value_type;
  ast_value value;
  struct {
    uint32_t lineno;
    uint32_t column;
    uint32_t length;
  };
  struct ast_node *child;
  struct ast_node *father;
  struct ast_node *lpeer;
  struct ast_node *rpeer;
  int childnum;
  cmm_type *arrtype;
  int isarg;
} ast_node;

extern const char *terminal_name_table[];
extern const char *nonterminal_name_table[];
extern ast_node *ast_root;
const char *get_symbol_name(int id);

ast_node *make_ast_node(int sy, int isterminal, ast_value value);

ast_node *make_ast_nonterm(int sy);

ast_node *make_ast_term(int sy, ast_value v);

ast_node *get_child_n(ast_node *fa, int n);

ast_node *get_child_last(ast_node *fa);

void add_ast_child(ast_node *fa, ast_node *c);

void print_ast(ast_node *root);

static void __print_ast(ast_node *root, int indent);
#endif