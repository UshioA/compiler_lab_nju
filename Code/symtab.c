#include "symtab.h"
#include "list.h"
#include "syntax.tab.h"
#include "type.h"
#include <stdint.h>

static symbol *make_symbol(cmm_type *ctype, char *name, int ival, float fval,
                           uint32_t *dimension) {
  symbol *sym = malloc(sizeof(symbol));
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

symbol *make_isymbol(char *name, uint32_t ival) {
  return make_symbol(new_cmm_btype(new_literal(INT)), name, ival, 0, NULL);
}
symbol *make_fsymbol(char *name, float fval) {
  return make_symbol(new_cmm_btype(new_literal(FLOAT)), name, 0, fval, NULL);
}
symbol *make_asymbol(char *name, int basetype, uint32_t *dimension) {
  return make_symbol(new_cmm_ctype(TYPE_ARR, new_literal(basetype)), name, 0, 0,
                     dimension);
}
symbol *make_ssymbol(char *name, cmm_type *ctype) {
  return make_symbol(ctype, name, 0, 0, 0);
}
symbol *make_funsymbol(char *name, cmm_type *ctype) {
  return make_symbol(ctype, name, 0, 0, 0);
}

int symbol_isbtype(symbol *s) { return s->type->is_basetype; }

sentry *make_sentry(symbol *s) {
  sentry *se = malloc(sizeof(sentry));
  se->symbol = s;
  list_init(&se->link);
  return se;
}

symbol *symget(symtab *stab, char *name) {
  sentry *se = stab->head[hash(name, strlen(name))];
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
  uint32_t index = hash(name, strlen(name));
  sentry *se = stab->head[index];
  if (!list_empty(&se->link)) {
    sentry *pos;
    list_entry *head = &se->link;
    list_for_each_item(pos, head, link) {
      if (!strcmp(pos->symbol->name, name)) {
        pos->symbol = sym; //! leak =(
        return;
      }
    }
  }
  list_add(&make_sentry(sym)->link, &se->link);
}

symtab *make_symtab() {
  symtab *stab = malloc(sizeof(symtab));
  stab->head = malloc(sizeof(sentry) * __CMM_HASH_SIZE__);
  for (int i = 0; i < __CMM_HASH_SIZE__; ++i) {
    stab->head[i] = make_sentry(NULL);
  }
  return stab;
}