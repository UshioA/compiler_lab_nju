#ifndef __CMM_FROM_IR__
#define __CMM_FROM_IR__

#include "array.h"
#include <stdio.h>
extern array *ir_list;
extern int funcno;
extern int varno;
extern int tempcnt;
extern int maxtempcnt;
extern int labelcnt;

void fromir(FILE *f);

#endif