#include "cfg.h"
#include "array.h"
#include "ir.h"
#include <assert.h>
#include <stdlib.h>

array *cfg_list;

cfg *new_cfg() {
  cfg *c = calloc(1, sizeof(cfg));
  c->adj = new_arr(DEFAULT_NODE_NUM);
  c->radj = new_arr(DEFAULT_NODE_NUM);
  c->node = new_arr(DEFAULT_NODE_NUM);
  return c;
}

void init_nodelist(cfg *g, array *node) {
  g->node = node;
  for (int i = 0; i < node->length; ++i) {
    BB *b = arr_get(i, node);
    assert(b->blkid == i);
    array *neighbor = new_arr(DEFAULT_NODE_NUM);
    array *rneighbor = new_arr(DEFAULT_NODE_NUM);
    arr_push(g->adj, neighbor);
    arr_push(g->radj, rneighbor);
  }
}

void add_edge(cfg *g, BB *b1, BB *b2) {
  arr_push(g->adj + b1->blkid, &b2->blkid);
  arr_push(g->adj + b2->blkid, &b1->blkid);
}

array *get_predecessor(cfg *g, BB *b) { return arr_get(b->blkid, g->radj); }
array *get_successor(cfg *g, BB *b) { return arr_get(b->blkid, g->adj); }

void init_cfg_list() {
  cfg_list = new_arr(funcno);
  for (int i = 0; i < funcno; ++i) {
    arr_push(cfg_list, new_cfg);
  }
}

array *make_func_blk() {
  array *func_head_list = new_arr(128);
  for (int i = 0; i < ir_list->length; ++i) {
    intercode *ir = ir_list->elem[i];
    if (ir->kind == IR_FUNCTION) {
      int *idx = malloc(sizeof(int));
      *idx = i;
      arr_push(func_head_list, idx);
    }
  }
  return func_head_list;
}