#ifndef __CMM_SYMTAB_H__
#define __CMM_SYMTAB_H__
#include "common.h"
#include "frame.h"
#include "list.h"
#include "type.h"
#include <stdint.h>

#define __CMM_HASH_SIZE_LARGE__ 0xfff
#define __CMM_HASH_SIZE_NORMAL__ 0x3ff
#define __CMM_HASH_SIZE_SMALL__ 0xff

typedef struct symbol { // variable, function, array, struct
  cmm_type *type;
  struct {
    char *name;
    int no;
  };
  union {
    int ival;            // int type
    float fval;          // float type
    uint32_t *dimension; // use dimension[0] as len.
  };
  int is_arg;
  uint32_t hash;
} symbol;

static inline uint32_t __fnv_hash(char *str, uint32_t length) {
  const uint32_t fnv_prime =
      0x811C9DC5; // 32 bit should be ok, assuming friendly user wont variables
                  // with a long name.
  uint32_t hash = 0;
  uint32_t i = 0;

  for (i = 0; i < length; str++, i++) {
    hash *= fnv_prime;
    hash ^= (*str);
  }
  return hash;
}

static inline uint32_t hash(char *str, uint32_t len, uint32_t size) {
  return __fnv_hash(str, len) % size;
}

typedef struct symtab_entry {
  symbol *symbol;
  int isfield;
  list_entry link;
} sentry;

typedef struct symtab {
  sentry **head;
  uint32_t hsize;
} symtab;

symbol *make_symbol(char *name, cmm_type *ctype);
symbol *make_isymbol(char *name, uint32_t ival);
symbol *make_fsymbol(char *name, float fval);
symbol *make_asymbol(char *name, cmm_type *basetype, uint32_t *dimension);
symbol *make_ssymbol(char *name, cmm_type *ctype);
symbol *make_funsymbol(char *name, cmm_type *ctype);
int symbol_isbtype(symbol *s);
sentry *make_sentry(symbol *);

symbol *symget(symtab *stab, char *name);
void symset(symtab *stab, char *name, symbol *sym);

symtab *make_symtab(uint32_t hsize);
symtab *make_symtabl();
symtab *make_symtabn();
symtab *make_symtabs();
#endif