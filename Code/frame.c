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
  f->sstab = stab_mkr();
  return f;
}

void frame_setfa(frame *f, frame *parent) { f->parent = parent; }

void frame_setstab(frame *f, struct symtab *stab) { f->stab = stab; }

void frame_setsstab(frame *f, struct symtab *sstab) { f->sstab = sstab; }

struct symbol *frame_lookup(frame *f, char *name) {
  symbol *sym = frame_local_lookup(f, name);
  if (sym)
    return sym;
  if (f->parent)
    return frame_lookup(f, name);
  return NULL;
}

struct symbol *frame_local_lookup(frame *f, char *name) {
  return symget(f->stab, name);
}

struct symbol *frame_slookup(frame *f, char *name) {
  symbol *sym = frame_slocal_lookup(f, name);
  if (sym)
    return sym;
  if (f->parent)
    return frame_slookup(f, name);
  return NULL;
}

struct symbol *frame_slocal_lookup(frame *f, char *name) {
  return symget(f->sstab, name);
}

void frame_add(frame *f, char *name, struct symbol *sym) {
  symset(f->stab, name, sym);
}

void frame_adds(frame *f, char *name, struct symbol *sym) {
  symset(f->sstab, name, sym);
}