#include "opt.h"
#include "array.h"
#include "basic_blk.h"
#include "bitset.h"
#include "cfg.h"
#include "ir.h"
#include "live.h"
#include <assert.h>

array *live_ctx;
array *entry_live_ctx;
array *exit_live_ctx;

void init_live() {
  int varcnt = maxtempcnt + varno + 1;
  live_ctx = new_live_ctx(ir_list->length, varcnt);
  entry_live_ctx = new_live_ctx(cfg_list->length, varcnt);
  exit_live_ctx = new_live_ctx(cfg_list->length, varcnt);
}

live_info *get_ir_live(int idx) { return arr_get(idx, live_ctx); }
live_info *get_entry_live(int idx) { return arr_get(idx, entry_live_ctx); }
live_info *get_exit_live(int idx) { return arr_get(idx, exit_live_ctx); }

void do_live(int idx) {
  if (idx < 2)
    return;
  cfg *cg = arr_get(idx, cfg_list);
  int changed;
  do {
    changed = 0;
    for (int i = cg->node->length; i >= 0; --i) {
      changed |= live_transfer_bb(arr_get(i, cg->node), cg);
    }
  } while (changed);
}

int live_transfer_bb(BB *node, cfg *g) {
  int changed = 0;
  if (node->kind == BB_EXIT)
    return 0;
  if (node->kind == BB_ENTRY) {
    //TODO meet succ.
  }

  for (int i = node->end - 1; i >= node->beg; --i) {
    live_transfer_ir(arr_get(i, ir_list), i == node->end - 1,
                     i == node->end - 1 ? get_successor(g, node) : NULL);
  }
  // return changed;
}

int live_transfer_ir(intercode *ir, int end, array *succ) {
  //TODO meet succ.
  if (end) {
    assert(succ);
  } else {
  }
}