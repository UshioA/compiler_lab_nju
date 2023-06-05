#ifndef __CMM_LIVE_H__
#define __CMM_LIVE_H__

#include "array.h"
#include "bitset.h"

typedef struct {
  int idx;
  bitset* IN;
  bitset* OUT;
} live_info;

live_info *new_live_info(int idx, int varcnt);

array *new_live_ctx(int size, int varcnt);

live_info *live_ctx_get_info(array *ctx, int idx);
bitset* live_ctx_get_set(array* ctx, int idx, int var, int is_in);
void live_ctx_add(array *ctx, int idx, int is_in, int var);
void live_ctx_del(array *ctx, int idx, int is_in, int var);

#endif