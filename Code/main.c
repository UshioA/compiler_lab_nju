#include "ast.h"
#include "semantic.h"
#include "translate.h"
#include "syntax.tab.h"
#include <stdio.h>
void yyrestart(FILE *);
int yyparse(ast_node *);
extern int parerr;
extern int semanerr;
int main(int argc, char **argv) {
  FILE *f;
  if (argc <= 1)
    f = stdin;
  else
    f = fopen(argv[1], "r");
  if (!f) {
    perror(argv[1]);
    return -1;
  }
  yyrestart(f);
  if (!yyparse(ast_root)){
    if (!parerr) {
      do_semantic(ast_root);
    }
    if(!semanerr){
      init_translate(stdout);
      translate_program(ast_root);
      dump_code();
    }
  }
  return 0;
}
