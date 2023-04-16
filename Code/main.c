#include "ast.h"
#include "ir.h"
#include "semantic.h"
#include "syntax.tab.h"
#include "translate.h"
#include <stdio.h>
void yyrestart(FILE *);
int yyparse(ast_node *);
extern int parerr;
extern int semanerr;
int main(int argc, char **argv) {
  FILE *f;
  FILE *ff;
  if (argc <= 1)
    f = stdin;
  else
    f = fopen(argv[1], "r");
  if (!f) {
    perror(argv[1]);
    return -1;
  }
  yyrestart(f);
  if (!yyparse(ast_root)) {
    if (!parerr) {
      do_semantic(ast_root);
    }
    if (!semanerr) {
      init_ircode();
      translate_program(ast_root);
      if (argc <= 2)
        ff = stdout;
      else
        ff = fopen(argv[2], "w");
      init_file(ff);
      dump_code();
    }
  }
  return 0;
}
