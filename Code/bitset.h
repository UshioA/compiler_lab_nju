#ifndef __CMM_BITSET_H__
#define __CMM_BITSET_H__
#include <stdint.h>
typedef struct {
  int limit;
  int setlen;
  uint64_t *set;
} bitset;

static void bitset_setat(bitset *bs, int index, int value);

bitset *new_bitset(int limit);
void bitset_insert(bitset *bs, int index);
void bitset_remove(bitset *bs, int index);
int bitset_contain(bitset *bs, int index);
int bitset_eq(bitset* b1, bitset* b2);
bitset *bitset_clone(bitset *b);
bitset *bitset_intersection(bitset *b1, bitset *b2);
bitset *bitset_union(bitset *b1, bitset *b2);
bitset *bitset_complement(bitset *b);
#endif