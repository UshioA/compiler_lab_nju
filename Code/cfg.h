#ifndef __CMM_CFG_H__
#define __CMM_CFG_H__

#include "array.h"
#include "basic_blk.h"
#define DEFAULT_NODE_NUM 256
extern const int writeno, readno;
typedef struct {
  array *adj;  // adjacency list
  array *radj; // reverse adjacency list
  array *node; // basic blocks
  int funid;
  int reachable;
} cfg;
extern int varno;
extern int funcno;
cfg *new_cfg();

void init_nodelist(cfg *g, array *node);

void add_edge(cfg *g, BB *b1, BB *b2);

array *get_predecessor(cfg *g, BB *b);
array *get_successor(cfg *g, BB *b);

extern array *ir_list;
extern array *cfg_list;

void init_cfg_list();
array *make_func_blk();
array *make_node_list(int beg, int end);
array *make_node_lists(array *funclist);
void build_cfg(array *nodelist_list);

void cfg_dump(FILE* f, cfg* g);
void adj_dump(FILE* f, array* adj);
#endif