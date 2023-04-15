#include "symtab.h"
#include "frame.h"
#include "list.h"
#include "syntax.tab.h"
#include "type.h"
#include <assert.h>
#include <stdint.h>

static symbol *_make_symbol(cmm_type *ctype, char *name, int ival, float fval,
                            uint32_t *dimension, int ftype) {
  symbol *sym = calloc(1, sizeof(symbol));
  sym->name = name;
  sym->type = ctype;
  if (ctype->is_basetype) {
    base_type *btype = ctype->btype;
    if (btype->dectype == INT) {
      sym->ival = ival;
    } else if (btype->dectype == FLOAT) {
      sym->fval = fval;
    }
  } else { // only handle array, store dimensions
    sym->dimension = dimension;
  }
  return sym;
}

// for first define some symbol, without value.
symbol *make_symbol(char *name, cmm_type *ctype) {
  if (ctype->errcode != NO_ERR)
    return NULL;
  if (ctype->is_basetype) {
    switch (ctype->btype->dectype) {
    case INT:
      return make_isymbol(name, 0);
    case FLOAT:
      return make_fsymbol(name, 0.0);
    case STRUCT:
      return make_ssymbol(name, ctype);
    default:
      assert(0);
    }
  } else {
    if (ctype->ctype == TYPE_FUNC) {
      return make_funsymbol(name, ctype);
    }
    uint32_t *dimensions = malloc(sizeof(uint32_t) * (cmm_compute_len(ctype)));
    return make_asymbol(name, ctype, dimensions);
    //! need to complete dimensions by user.
  }
}

symbol *make_isymbol(char *name, uint32_t ival) {
  return _make_symbol(new_cmm_btype(new_literal(INT)), name, ival, 0, NULL, -1);
}
symbol *make_fsymbol(char *name, float fval) {
  return _make_symbol(new_cmm_btype(new_literal(FLOAT)), name, 0, fval, NULL,
                      -1);
}
symbol *make_asymbol(char *name, cmm_type *basetype, uint32_t *dimension) {
  cmm_type *some = new_cmm_ctype(TYPE_ARR, basetype->btype);
  some->contain_len = basetype->contain_len;
  some->dimensions = basetype->dimensions;
  return _make_symbol(some, name, 0, 0, dimension, -1);
}
symbol *make_ssymbol(char *name, cmm_type *ctype) {
  return _make_symbol(ctype, name, 0, 0, 0, 2);
}
symbol *make_funsymbol(char *name, cmm_type *ctype) {
  return _make_symbol(ctype, name, 0, 0, 0, -1);
}

int symbol_isbtype(symbol *s) { return s->type->is_basetype; }

sentry *make_sentry(symbol *s) {
  sentry *se = malloc(sizeof(sentry));
  se->symbol = s;
  list_init(&se->link);
  return se;
}

symbol *symget(symtab *stab, char *name) {
  if (!name)
    return NULL;
  sentry *se = stab->head[hash(name, strlen(name), stab->hsize)];
  if (!se)
    return NULL;
  sentry *pos;
  list_entry *head = &se->link;
  list_for_each_item(pos, head, link) {
    if (!strcmp(pos->symbol->name, name)) {
      return pos->symbol;
    }
  }
  return NULL;
}
void symset(symtab *stab, char *name, symbol *sym) {
  uint32_t index = hash(name, strlen(name), stab->hsize);
  sentry *se = stab->head[index];
  if (!list_empty(&se->link)) {
    sentry *pos;
    list_entry *head = &se->link;
    list_for_each_item(pos, head, link) {
      if (!strcmp(pos->symbol->name, name)) {
        pos->symbol = sym; //! leak =)
        return;
      }
    }
  }
  list_add(&make_sentry(sym)->link, &se->link);
}

symtab *make_symtab(uint32_t hsize) {
  symtab *stab = malloc(sizeof(symtab));
  stab->hsize = hsize;
  stab->head = malloc(sizeof(sentry) * hsize);
  for (int i = 0; i < hsize; ++i) {
    stab->head[i] = make_sentry(NULL);
  }
  return stab;
}

symtab *make_symtabl() { return make_symtab(__CMM_HASH_SIZE_LARGE__); }
symtab *make_symtabn() { return make_symtab(__CMM_HASH_SIZE_NORMAL__); }
symtab *make_symtabs() { return make_symtab(__CMM_HASH_SIZE_SMALL__); }