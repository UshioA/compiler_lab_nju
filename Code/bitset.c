#include "bitset.h"
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static uint32_t map_num(uint32_t index) { return index / (sizeof(uint64_t) << 3); }
static uint32_t map_bit(uint32_t index) { return index % (sizeof(uint64_t) << 3); }

static void bitset_setat(bitset *bs, uint32_t index, uint32_t value) {
  int positive = value != 0;
  assert(index < bs->limit);
  uint32_t numat = map_num(index);
  uint32_t bitat = map_bit(index);
  if (positive) {
    bs->set[numat] |= (0x1ul << bitat);
  } else {
    bs->set[numat] &= (~(0x1ul << bitat));
  }
}
bitset *new_bitset(uint32_t limit) {
  bitset *bs = (void *)calloc(1, sizeof(bitset));
  bs->limit = limit;
  bs->setlen = map_num(limit) + 1;
  bs->set = (void *)calloc(bs->setlen, sizeof(uint64_t));
  assert(bs->set);
  return bs;
}

void bitset_insert(bitset *bs, uint32_t index) { bitset_setat(bs, index, 1); }
void bitset_remove(bitset *bs, uint32_t index) { bitset_setat(bs, index, 0); }
uint64_t bitset_contain(bitset *bs, uint32_t index) {
  uint32_t numat = map_num(index);
  uint32_t bitat = map_bit(index);
  return ((bs->set[numat]) & (0x1ul << bitat));
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