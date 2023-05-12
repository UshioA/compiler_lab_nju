#include "cfg.h"
#include "array.h"
#include "basic_blk.h"
#include "bitset.h"
#include "ir.h"
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
const int writeno = 0, readno = 1;
array *cfg_list;
array *reachable;
BB *_exit;
int blkcnt = 1;
void adj_dump(FILE *f, array *adj) {
  if (!adj)
    return;
  fprintf(f, "[");
  for (int i = 0; i < adj->length; ++i) {
    fprintf(f, "%d", *(int *)arr_get(i, adj));
    if (i < adj->length - 1)
      fprintf(f, ", ");
  }
  fprintf(f, "]\n");
}

void cfg_dump(FILE *f, cfg *g) {
  if (!g)
    return;
  for (int i = 0; i < g->node->length; ++i) {
    BB *node = arr_get(i, g->node);
    bb_dump(f, node);
    adj_dump(f, get_successor(g, node));
    adj_dump(f, get_predecessor(g, node));
  }
}

cfg *new_cfg() {
  cfg *c = calloc(1, sizeof(cfg));
  c->adj = new_arr(DEFAULT_NODE_NUM);
  c->radj = new_arr(DEFAULT_NODE_NUM);
  c->node = new_arr(DEFAULT_NODE_NUM);
  c->reachable = 1;
  return c;
}

void init_nodelist(cfg *g, array *node) {
  g->node = node;
  for (int i = 0; i < node->length; ++i) {
    BB *b = arr_get(i, node);
    assert(b->kind == BB_EXIT || b->blkid == i);
    array *neighbor = new_arr(DEFAULT_NODE_NUM);
    array *rneighbor = new_arr(DEFAULT_NODE_NUM);
    arr_push(g->adj, neighbor);
    arr_push(g->radj, rneighbor);
  }
}

void add_edge(cfg *g, BB *b1, BB *b2) {
  arr_push(arr_get(b1->blkid, g->adj), &b2->blkid);
  arr_push(arr_get(b2->blkid, g->radj), &b1->blkid);
}

array *get_predecessor(cfg *g, BB *b) { return arr_get(b->blkid, g->radj); }
array *get_successor(cfg *g, BB *b) { return arr_get(b->blkid, g->adj); }

void init_cfg_list() {
  cfg_list = new_arr(funcno);
  reachable = new_arr(funcno);
  for (int i = 0; i < funcno; ++i) {
    arr_push(reachable, 0);
  }
  if (!_exit)
    _exit = new_bb_exit(-1);
  for (int i = 0; i < funcno; ++i) {
    arr_push(cfg_list, new_cfg());
  }
}

array *make_func_blk() {
  array *func_head_list = new_arr(128);
  for (int i = 0; i < ir_list->length; ++i) {
    intercode *ir = ir_list->elem[i];
    if (ir->kind == IR_FUNCTION) {
      arr_push(func_head_list, (void *)((uint64_t)i));
    }
  }
  arr_push(func_head_list, (void *)(uint64_t)ir_list->length);
  return func_head_list;
}

array *make_node_lists(array *funclist) {
  array *nodelists = new_arr(64);
  for (int i = 0; i < funclist->length - 1; ++i) {
    array *nl = make_node_list(((int)(uint64_t)funclist->elem[i]),
                               ((int)(uint64_t)funclist->elem[i + 1]));
    arr_push(nodelists, nl);
  }
  return nodelists;
}

array *make_node_list(int beg, int end) {
  // assert(((intercode *)arr_get(beg, ir_list))->kind == IR_FUNCTION);
  array *leader = new_arr(64);
  // use a bitset for unique.
  bitset *bs = new_bitset(end + 1);
  arr_push(leader, (void *)(uint64_t)beg);
  for (int i = beg; i < end; ++i) {
    intercode *ir = arr_get(i, ir_list);
    switch (ir->kind) {
    case IR_LABEL: {
      if (!bitset_contain(bs, i)) {
        arr_push(leader, (void *)(uint64_t)i);
        bitset_insert(bs, i);
      }
    } break;
    case IR_GOTO:
    case IR_IF_GOTO:
    case IR_RET: {
      if (!bitset_contain(bs, i + 1)) {
        arr_push(leader, (void *)(uint64_t)(i + 1));
        bitset_insert(bs, i + 1);
      }
    } break;
    case IR_CALL: {
      arr_set(ir->call.func->funcno, (void *)1, reachable);
    } break;
    case IR_READ: {
      arr_set(readno, (void *)1, reachable);
    } break;
    case IR_WRITE: {
      arr_set(writeno, (void *)1, reachable);
    } break;
    default:
      break;
    }
  }
  // return statement is guranteed, so no need
  // if (!bitset_contain(bs, end)) {
  //   arr_push(leader, (void *)(uint64_t)(end));
  //   bitset_insert(bs, end);
  // }
  array *nodelist = new_arr(64);
  arr_push(nodelist, new_bb_entry(0));
  int nodeid = 0;
  for (int i = 0; i < leader->length; ++i) {
    int at = (int)((uint64_t)arr_get(i, leader));
    int ed = i < leader->length - 1 ? ((int)(uint64_t)(arr_get(i + 1, leader)))
                                    : end;
    if (at != ed)
      arr_push(nodelist, new_bb_node(++nodeid, at, ed));
  }
  arr_push(nodelist, new_bb_exit(++nodeid));
  // for (int i = 0; i < nodelist->length; ++i) {
  //   BB *b = nodelist->elem[i];
  //   bb_dump(stdout, b);
  // }
  return nodelist;
}

static BB *find_head(array *nodelist, int labelno) {
  for (int i = 0; i < nodelist->length; ++i) {
    BB *bb = (BB *)arr_get(i, nodelist);
    if (bb->kind != BB_NODE)
      continue;
    intercode *label = arr_get(bb->beg, ir_list);
    if (label->kind != IR_LABEL)
      continue;
    if (label->label.label->tempno == labelno)
      return bb;
  }
  return NULL;
}

void build_cfg(array *nodelist_list) {
  // reserved for READ and WRITE, no cfg so mark NULL
  cfg_list->elem[0] = cfg_list->elem[1] = NULL;
  for (int i = 0; i < nodelist_list->length; ++i) {
    if (!(uint64_t)arr_get(i, reachable))
      continue;
    array *nl = arr_get(i, nodelist_list);
    cfg *g = arr_get(i + 2, cfg_list);
    init_nodelist(g, nl);
    for (int j = 0; j < nl->length; ++j) {
      BB *node = arr_get(j, nl);
      BB *next;
      BB *ext = arr_get(nl->length - 1, nl);
      if (j < nl->length - 1)
        next = arr_get(j + 1, nl);
      else
        next = ext;
      // find all successors and build edges.
      switch (node->kind) {
      case BB_NODE: {
        intercode *tail = arr_get(node->end - 1, ir_list);
        switch (tail->kind) {
        case IR_IF_GOTO:
        case IR_GOTO: {
          operand *to = tail->kind == IR_GOTO ? tail->go.to : tail->ifgo.to;
          BB *target = find_head(nl, to->tempno);
          assert(target);
          add_edge(g, node, target);
          add_edge(g, node, next);
        } break;
        case IR_RET: {
          add_edge(g, node, ext);
          if (next != ext)
            add_edge(g, node, next);
        } break;
        default: {
          add_edge(g, node, next);
        } break;
        }
      } break;
      case BB_ENTRY: {
        add_edge(g, node, next);
      } break;
      case BB_EXIT: {
      } break;
      defalut : { assert(0); } break;
      }
    }
  }
}