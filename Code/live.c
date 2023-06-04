#include "live.h"
#include "array.h"
#include "bitset.h"
#include <stdlib.h>

live_info *new_live_info(int idx, int varcnt) {
  live_info *l = calloc(1, sizeof(live_info));
  l->idx = idx;
  l->IN.def = new_bitset(varcnt);
  l->IN.use = new_bitset(varcnt);
  l->OUT.def = new_bitset(varcnt);
  l->OUT.def = new_bitset(varcnt);
  return l;
}

array *new_live_ctx(int size, int varcnt) {
  array *a = new_arr(size);
  for (int i = 0; i < size; ++i) {
    live_info *l = new_live_info(i, varcnt);
    arr_push(a, l);
  }
  return a;
}

live_info *live_ctx_get_info(array *ctx, int idx) { return arr_get(idx, ctx); }

bitset *live_ctx_get_set(array *ctx, int idx, int var, int is_in, int is_def) {
  live_info *l = live_ctx_get_info(ctx, idx);
  if (is_in) {
    if (is_def) {
      return l->IN.def;
    } else {
      return l->IN.use;
    }
  } else {
    if (is_def) {
      return l->OUT.def;
    } else {
      return l->OUT.use;
    }
  }
}

static void live_ctx_add(array *ctx, int idx, int is_in, int is_def, int var,
                         int val) {
  bitset *bs = live_ctx_get_set(ctx, idx, var, is_in, is_def);
  if (val)
    bitset_insert(bs, var);
  else
    bitset_remove(bs, var);
}

void live_ctx_add_def(array *ctx, int idx, int is_in, int var) {
  live_ctx_add(ctx, idx, is_in, 1, var, 1);
}
void live_ctx_add_use(array *ctx, int idx, int is_in, int var) {
  live_ctx_add(ctx, idx, is_in, 0, var, 1);
}
void live_ctx_del_def(array *ctx, int idx, int is_in, int var) {
  live_ctx_add(ctx, idx, is_in, 1, var, 0);
}
void live_ctx_del_use(array *ctx, int idx, int is_in, int var) {
  live_ctx_add(ctx, idx, is_in, 0, var, 0);
}