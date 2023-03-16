#ifndef __CMM_FRAME_H__
#define __CMM_FRAME_H__

struct symtab;
struct symbol;

typedef struct frame {
  struct frame *parent;
  struct symtab *stab;
} frame;

frame* make_frame(frame* parent, int type);

void frame_setfa(frame* f, frame* parent);

void frame_setstab(frame* f, struct symtab* stab);

struct symbol* frame_lookup(frame* f, char* name);

void frame_add(frame* f, char* name, struct symbol* sym);

#endif