#ifndef __CMM_OPT_H__
#define __CMM_OPT_H__

#include "array.h"
#include "basic_blk.h"
#include "cfg.h"
#include "ir.h"
#include "live.h"
extern array *ir_list;
extern array *cfg_list;
extern array *ir_live_ctx;
extern array *entry_live_ctx;
extern array *exit_live_ctx;

extern int funcno;
extern int varno;
extern int maxtempcnt;

void init_live();

live_info *get_ir_live(int idx);
live_info *get_entry_live(int idx);
live_info *get_exit_live(int idx);

void do_live(int idx);

int live_transfer_bb(BB *node, cfg *g);

int live_transfer_ir(intercode *ir, int end, array *succ);

#endif