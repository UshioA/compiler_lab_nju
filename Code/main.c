#include "ast.h"
#include "semantic.h"
#include "syntax.tab.h"
#include <stdio.h>
void yyrestart(FILE *);
int yyparse(ast_node *);
extern int parerr;
int main(int argc, char **argv) {
  if (argc <= 1)
    return 1;
  FILE *f = fopen(argv[1], "r");
  if (!f) {
    perror(argv[1]);
    return -1;
  }
  yyrestart(f);
  if (!yyparse(ast_root))
    if (!parerr) {
      print_ast(ast_root);
      do_semantic(ast_root);
    }
  return 0;
}
