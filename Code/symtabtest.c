#include "symtab.h"
#include "type.h"
#include <assert.h>
#include <math.h>

int main(){
  symtab* stab = make_symtab();
  symbol* a1 = make_isymbol("a1", 1);
  symbol* a2 = make_fsymbol("a2", 1.2);
  symset(stab, a1->name, a1);
  symset(stab, a2->name, a2);
  symbol* aa1 = symget(stab, "a1");
  assert(aa1==a1);
  assert(aa1->ival == a1->ival);
  assert(aa1->ival == 1);
  symbol* aa2 = symget(stab, "a2");
  assert(aa2==a2);
  assert(fabs(aa2->fval-1.2) < 1e-6);
  assert(aa2->fval == a2->fval);
  symbol* non = symget(stab, "what");
  assert(!non);
  symbol* a1modify = make_isymbol("a1", 114514);
  symset(stab, a1modify->name, a1modify);
  symbol* am1 = symget(stab, "a1");
  assert(am1 != aa1);
  
}