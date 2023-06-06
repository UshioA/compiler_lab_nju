#include "array.h"
#include "ast.h"
#include "cfg.h"
#include "fromir.h"
#include "ir.h"
#include "mips.h"
#include "opt.h"
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
  init_ircode();
  funcno = 2;
  fromir(f);
  if (argc <= 2)
    ff = stdout;
  else
    ff = fopen(argv[2], "w");
  init_file(ff);
  // dump_code();
  // yyrestart(f);
  // if (!yyparse(ast_root)) {
  //   if (!parerr) {
  //     do_semantic(ast_root);
  //     if (!semanerr) {
  //       init_ircode();
  //       translate_program(ast_root);
  init_cfg_list();
  build_cfg(make_node_lists(make_func_blk()));
  // init_codefile(ff);
  // gencode();
  // for (int i = 0; i < cfg_list->length; ++i) {
  //   cfg_dump(stdout, arr_get(i, cfg_list));
  // }
  init_live();
  int thing = 0;
  // int cnt = 0;
  do {
    // printf("ROUND %d\n", cnt++);
    thing = 0;
    for (int i = 2; i < cfg_list->length; ++i) {
      if (cfg_list->elem[i] && ((cfg *)cfg_list->elem[i])->reachable)
        do_live(i);
    }
    thing |= live_remove_useless();
    thing |= live_merge_assign();
  } while (thing);
  dump_code();
  return 0;
}
// }
// }
