#include "basic_blk.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

BB *new_bb(int blkid, int beg, int end, int kind) {
  BB *b = (BB *)calloc(1, sizeof(BB));
  b->blkid = blkid;
  b->beg = beg;
  b->end = end;
  b->kind = kind;
  b->reachable = 0;
  return b;
}

BB *new_bb_node(int blkid, int beg, int end) {
  return new_bb(blkid, beg, end, BB_NODE);
}

BB *new_bb_entry(int blkid) { return new_bb(blkid, 0, 0, BB_ENTRY); }

BB *new_bb_exit(int blkid) { return new_bb(blkid, -1, -1, BB_EXIT); }

void bb_dump(FILE *f, BB *b) {
  switch (b->kind) {
  case BB_ENTRY: {
    fprintf(f, "BB[Entry]\n");
  } break;
  case BB_EXIT: {
    fprintf(f, "BB[Exit]\n");
  } break;
  case BB_NODE: {
    fprintf(f, "BB[%d]: beg=%d, end=%d\n", b->blkid, b->beg, b->end);
  } break;
  default:
    assert(0);
  }
}