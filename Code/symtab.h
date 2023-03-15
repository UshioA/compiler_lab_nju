#ifndef __CMM_SYMTAB_H__
#define __CMM_SYMTAB_H__
#include "common.h"
#include "list.h"
#include "type.h"
#include <cstdint>

#define __CMM_HASH_SIZE__ 0xffff

typedef struct symbol { // variable, function, array, struct
  cmm_type *type;
  char *name;
  union {
    int ival;       // int type
    float fval;     // float type
    int *dimension; // use dimension[0] as len.
  };
} symbol;

static inline uint32_t __fnv_hash(char *str, uint32_t length) {
  const uint32_t fnv_prime = 0x811C9DC5;
  uint32_t hash = 0;
  uint32_t i = 0;

  for (i = 0; i < length; str++, i++) {
    hash *= fnv_prime;
    hash ^= (*str);
  }
  return hash;
}

static inline uint32_t hash(char *str, uint32_t len) {
  return __fnv_hash(str, len) % __CMM_HASH_SIZE__;
}

typedef struct symtab_entry {
  symbol *symbol;
  list_entry link;
} sentry;

// TODO: hash table.

#endif