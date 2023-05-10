#ifndef __CMM_BASIC_BLK_H__
#define __CMM_BASIC_BLK_H__

#include "array.h"
extern array *ir_list;

typedef struct BB {
  int blkid;
  // [beg, end)
  int beg;
  int end;
} BB;

#endif