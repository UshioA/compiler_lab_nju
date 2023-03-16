#include "frame.h"
#include "common.h"
#include "symtab.h"
#include "type.h"

frame *make_frame(frame *parent, int type) {
  if (type > 2 || type < 0)
    return NULL;
  symtab *(*stab_mkr)() = type == 0   ? make_symtabl
                          : type == 1 ? make_symtabn
                                      : make_symtabs; // shit code.
  frame *f = malloc(sizeof(frame));
  f->parent = parent;
  f->stab = stab_mkr();
  return f;
}

void frame_setfa(frame *f, frame *parent) { f->parent = parent; }

void frame_setstab(frame *f, struct symtab *stab) { f->stab = stab; }

struct symbol *frame_lookup(frame *f, char *name) {
  symbol *sym = symget(f->stab, name);
  if (sym)
    return sym;
  if (f->parent)
    return frame_lookup(f, name);
  return NULL;
}

void frame_add(frame *f, char *name, struct symbol *sym) {
  symset(f->stab, name, sym);
}