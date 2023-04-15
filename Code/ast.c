#include "ast.h"
#include "syntax.tab.h"
#include <stddef.h>

const char *terminal_name_table[] = {
    "YYEMPTY",  "YYEOF", "YYerror", "YYUNDEF", "TYPE", "INT",   "FLOAT", "ID",
    "ASSIGNOP", "SEMI",  "COMMA",   "RELOP",   "PLUS", "MINUS", "STAR",  "DIV",
    "AND",      "OR",    "DOT",     "NOT",     "LP",   "RP",    "LB",    "RB",
    "LC",       "RC",    "STRUCT",  "RETURN",  "IF",   "ELSE",  "WHILE", NULL,
};

const char *nonterminal_name_table[] = {
    "Program",   "ExtDefList",      "ExtDef",  "ExtDecList",
    "Specifier", "StructSpecifier", "OptTag",  "Tag",
    "VarDec",    "FunDec",          "VarList", "ParamDec",
    "CompSt",    "StmtList",        "Stmt",    "DefList",
    "Def",       "DecList",         "Dec",     "Exp",
    "Args",
};

ast_node *ast_root;

const char *get_symbol_name(int id) {
  if (id < TYPE)
    return nonterminal_name_table[id];
  return terminal_name_table[id - TYPE + 4];
}

ast_node *make_ast_node(int sy, int isterminal, ast_value value) {
  ast_node *a = calloc(1, sizeof(ast_node));
  a->lineno = yylloc.first_line;
  a->column = yylloc.first_column;
  a->symbol = sy;
  a->length = yyleng;
  if (isterminal)
    a->value = value;
  a->child = a->father = a->lpeer = a->rpeer = NULL;
  a->childnum = 0;
  return a;
}

ast_node *make_ast_nonterm(int sy) {
  // printf("build nonterm %s\n", get_symbol_name(sy));
  return make_ast_node(sy, 0, (ast_value)(0xffffffff));
}

ast_node *make_ast_term(int sy, ast_value v) {
  // printf("term: %d\n", sy);
  // printf("build term %s\n", get_symbol_name(sy));
  return make_ast_node(sy, 1, v);
}

ast_node *get_child_n(ast_node *fa, int n) {
  if (n < 0)
    return NULL;
  ast_node *chn = fa->child;
  while (n--) {
    chn = chn->lpeer;
    if (!chn)
      return NULL;
  }
  return chn;
}

ast_node *get_child_last(ast_node *fa) {
  return get_child_n(fa, fa->childnum - 1);
}

void add_ast_child(ast_node *fa, ast_node *c) {
  if (c) {
    // printf("add node %s to %s\n", get_symbol_name(c->symbol),
    // get_symbol_name(fa->symbol));
    ast_node *lc = get_child_last(fa);
    if (!lc) {
      fa->child = c;
      c->lpeer = c->rpeer = NULL;
      fa->lineno = c->lineno;
      fa->column = c->column;
      fa->length = c->length;
    } else {
      lc->lpeer = c;
      c->rpeer = lc;
      c->lpeer = NULL;
      fa->length += c->length;
    }
    c->father = fa;
    ++fa->childnum;
  }
}

int get_symbol_type(int sy) {
  switch (sy) {
  case INT: {
    return INT_TYPE;
  } break;
  case FLOAT: {
    return FLOAT_TYPE;
  } break;
  case RELOP:
  case TYPE:
  case ID: {
    return STR_TYPE;
  } break;
  default: {
    return NONE_TYPE;
  }
  }
}

void print_ast(ast_node *root) { __print_ast(root, 0); }

static void __print_ast(ast_node *root, int space) {
  if (!root)
    return;
  for (int i = 0; i < space; ++i) {
    printf("  ");
  }
  printf("%s", get_symbol_name(root->symbol));
  switch (get_symbol_type(root->symbol)) {
  case INT_TYPE: {
    printf(": %u", root->value.int_val);
  } break;
  case FLOAT_TYPE: {
    printf(": %f", root->value.float_val);
  } break;
  case STR_TYPE: {
    if (root->symbol != RELOP)
      printf(": %s", root->value.str_val);
  } break;
  case NONE_TYPE: {
  } break;
  }
  if (root->symbol < TYPE) {
    printf(" (%d)", root->lineno);
  }
  puts("");
  ast_node *p = root->child;
  while (p) {
    __print_ast(p, space + 1);
    p = p->lpeer;
  }
}