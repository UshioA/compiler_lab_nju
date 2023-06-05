#ifndef __CMM_BITSET_H__
#define __CMM_BITSET_H__
#include <stdint.h>
typedef struct {
  uint32_t limit;
  uint32_t setlen;
  uint64_t *set;
} bitset;

bitset *new_bitset(uint32_t limit);
void bitset_insert(bitset *bs, uint32_t index);
void bitset_remove(bitset *bs, uint32_t index);
uint64_t bitset_contain(bitset *bs, uint32_t index);
int bitset_eq(bitset *b1, bitset *b2);
bitset *bitset_clone(bitset *b);
int bitset_assign(bitset *b1, bitset *b2);
int bitset_intersection(bitset *b1, bitset *b2);
int bitset_union(bitset *b1, bitset *b2);
bitset *bitset_complement(bitset *b);
#endif