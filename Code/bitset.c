#include "bitset.h"
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static int map_num(int index) { return index / sizeof(uint64_t); }
static int map_bit(int index) { return index % sizeof(uint64_t); }

static void bitset_setat(bitset *bs, int index, int value) {
  int positive = value != 0;
  assert(index < bs->limit);
  int numat = map_num(index);
  int bitat = map_bit(index);
  if (positive) {
    bs->set[numat] |= (0x1ul << bitat);
  } else {
    bs->set[numat] &= (~(0x1ul << bitat));
  }
}
bitset *new_bitset(int limit) {
  bitset *bs = (void *)calloc(1, sizeof(bitset));
  bs->limit = limit;
  bs->setlen = map_num(limit) + 1;
  bs->set = (void *)calloc(bs->setlen, sizeof(uint64_t));
  assert(bs->set);
  return bs;
}

void bitset_insert(bitset *bs, int index) { bitset_setat(bs, index, 1); }
void bitset_remove(bitset *bs, int index) { bitset_setat(bs, index, 0); }
int bitset_contain(bitset *bs, int index) {
  int numat = map_num(index);
  int bitat = map_bit(index);
  return bs->set[numat] & (0x1ul << bitat);
}
int bitset_eq(bitset *b1, bitset *b2) {
  if (b1->limit != b2->limit)
    return 0;
  for (int i = 0; i < b1->setlen; ++i) {
    if (b1->set[i] != b2->set[i])
      return 0;
  }
  return 1;
}
bitset *bitset_clone(bitset *b) {
  bitset *bc = new_bitset(b->limit);
  memcpy(bc->set, b->set, sizeof(uint64_t) * bc->setlen);
  return bc;
}

bitset *bitset_complement(bitset *b) {
  bitset *bc = new_bitset(b->limit);
  // This and the following for-loops should have been rewritten in
  // functional style, i.e. map. Nonetheless, C does not have lambda or
  // something similar, so i just write some dung pies here
  for (int i = 0; i < bc->setlen; ++i) {
    bc->set[i] = ~(b->set[i]);
  }
  return bc;
}

bitset *bitset_intersection(bitset *b1, bitset *b2) {
  assert(b1->limit == b2->limit);
  bitset *bi = bitset_clone(b1);
  for (int i = 0; i < bi->setlen; ++i) {
    bi->set[i] &= b2->set[i];
  }
  return bi;
}

bitset *bitset_union(bitset *b1, bitset *b2) {
  assert(b1->limit == b2->limit);
  bitset *bu = bitset_clone(b1);
  for (int i = 0; i < b1->setlen; ++i) {
    bu->set[i] |= b2->set[i];
  }
  return bu;
}