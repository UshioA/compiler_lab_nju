#include "basic_blk.h"
#include <stdlib.h>

BB *new_bb(int blkid, int beg, int end, int kind) {
  BB *b = (BB *)calloc(1, sizeof(BB));
  b->blkid = blkid;
  b->beg = beg;
  b->end = end;
  b->kind = kind;
  return b;
}

BB *new_bb_node(int blkid, int beg, int end) {
  return new_bb(blkid, beg, end, BB_NODE);
}

BB *new_bb_entry(int blkid) { return new_bb(blkid, 0, 0, BB_ENTRY); }

BB *new_bb_exit(int blkid) { return new_bb(blkid, -1, -1, BB_EXIT); }