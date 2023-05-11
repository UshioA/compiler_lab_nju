#ifndef __CMM_BASIC_BLK_H__
#define __CMM_BASIC_BLK_H__

#include "array.h"
extern array *ir_list;

typedef struct BB {
  enum { BB_ENTRY, BB_EXIT, BB_NODE } kind;
  int blkid;
  // [beg, end)
  int beg;
  int end;
} BB;

BB *new_bb(int blkid, int beg, int end, int kind);
BB *new_bb_node(int blkid, int beg, int end);
BB *new_bb_entry(int blkid);
BB *new_bb_exit(int blkid);
#endif