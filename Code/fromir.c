#include "fromir.h"
#include "array.h"
#include "basic_blk.h"
#include "ir.h"
#include "mips.h"
#include "symtab.h"
#include "syntax.tab.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static char buf[4096];
static char sbuf[7][256];

static int ha(char *str, int len) { return hash(str, len, 4096); }

static operand *funpool[4096];
static operand *labelpool[4096];
static operand *varpool[4096];
static operand *tmppool[4096];

static char *mstrdup(char *s) {
  char *r = malloc(sizeof(char) * strlen(s));
  strcpy(r, s);
  return r;
}

static operand *parse_func(char *s) {
  int funid = ha(s, strlen(s));
  operand *to = funpool[funid];
  if (!to) {
    to = new_func(mstrdup(s), funcno++);
    funpool[funid] = to;
  }
  return to;
}

static operand *parse_label(char *s) {
  int labelid;
  sscanf(s, "label%d", &labelid);
  operand *to = labelpool[labelid];
  if (!to)
    labelpool[labelid] = to = new_v(OPR_LABEL, labelid, NULL, 0);
  return to;
}

static operand *parse_opr(char *s) {
  int ref = 0, deref = 0;
  if (s[0] == '*') {
    deref = 1;
    ++s;
  } else if (s[0] == '&') {
    ref = 1;
    ++s;
  } else if (s[0] == '#') {
    int32_t imm;
    sscanf(s, "#%d", &imm);
    return new_imm(imm);
  } else if (strchr("0123456789", s[0])) {
    int size;
    sscanf(s, "%d", &size);
    return new_size(size / 4);
  }
  int cnt;
  int istmp = s[0] == 't';
  if (istmp) {
    sscanf(s, "t%d", &cnt);
  } else {
    sscanf(s, "v%d", &cnt);
  }
  cnt = ha(s, strlen(s));
  operand *ret;
  if (istmp) {
    if (!(ret = tmppool[cnt])) {
      tempcnt++;
      maxtempcnt++;
    }
    ret = new_v(OPR_TMP, !ret ? (tempcnt - 1) : ret->tempno, NULL, 0);
    ret->deref = deref;
    ret->ref = ref;
    tmppool[cnt] = ret;
  } else {
    if (!(ret = varpool[cnt])) {
      varno++;
    }
    ret = new_v(OPR_VAR, -1, mstrdup(s), !ret ? (varno - 1) : ret->varno);
    ret->deref = deref;
    ret->ref = ref;
    varpool[cnt] = ret;
  }
  return ret;
}

#define FUNCTION 2510
#define LABEL 3886
#define GOTO 4007
#define _IF 3435
#define _RETURN 410
#define DEC 598
#define ARG 2228
#define CALL 2878
#define PARAM 3815
#define READ 1030
#define WRITE 3949
void fromir(FILE *f) {
  assert(f);
  //* these magic numbers are results of the following lines.
  // const int FUNCTION = hash("FUNCTION", 8, 4096);
  // const int LABEL = hash("LABEL", 5, 4096);
  // const int GOTO = hash("GOTO", 4, 4096);
  // const int IF = hash("IF", 2, 4096);
  // const int RETURN = hash("RETURN", 6, 4096);
  // const int DEC = hash("DEC", 3, 4096);
  // const int ARG = hash("ARG", 3, 4096);
  // const int CALL = hash("CALL", 4, 4096);
  // const int PARAM = hash("PARAM", 5, 4096);
  // const int READ = hash("READ", 4, 4096);
  // const int WRITE = hash("WRITE", 5, 4096);
  while (fscanf(f, "%[^\n]\n", buf) != EOF) {
    sscanf(buf, "%s", sbuf[0]);
    if (strchr("*vt", sbuf[0][0])) {
      int cnt = sscanf(buf, "%s := %s%s%s", sbuf[1], sbuf[2], sbuf[3], sbuf[4]);
      operand *lhs = parse_opr(sbuf[1]);
      int hash = ha(sbuf[2], strlen(sbuf[2]));
      if (hash == CALL) {
        operand *func = parse_func(sbuf[3]);
        intercode *call = new_call_ir(func, lhs);
        ir_pushback(call);
      } else {
        if (cnt == 2) {
          operand *rhs = parse_opr(sbuf[2]);
          intercode *assign = new_assign_ir(lhs, rhs);
          ir_pushback(assign);
        } else if (cnt == 4) {
          operand *src1 = parse_opr(sbuf[2]);
          char *op = sbuf[3];
          static int _[] = {
              ['+'] = PLUS, ['-'] = MINUS, ['*'] = STAR, ['/'] = DIV};
          int kind = _[(int)op[0]];
          operand *src2 = parse_opr(sbuf[4]);
          intercode *arith = new_arith_ir(kind, src1, src2, lhs);
          ir_pushback(arith);
        } else
          assert(0);
      }
    } else {
      int hash = ha(sbuf[0], strlen(sbuf[0]));
      switch (hash) {
      case FUNCTION:
      case LABEL: {
        sscanf(buf, "%s %s :", sbuf[0], sbuf[1]);
        if (hash == FUNCTION) {
          operand *func = parse_func(sbuf[1]);
          intercode *fir = new_func_ir(func);
          ir_pushback(fir);
        } else {
          int labelid;
          sscanf(sbuf[1], "label%d", &labelid);
          operand *label = parse_label(sbuf[1]);
          intercode *l = new_label_ir(label);
          ir_pushback(l);
        }
      } break;
      case GOTO: {
        sscanf(buf, "%s %s", sbuf[0], sbuf[1]);
        int labelid;
        sscanf(sbuf[1], "label%d", &labelid);
        operand *to = parse_label(sbuf[1]);
        intercode *l = new_goto_ir(to);
        ir_pushback(l);
      } break;
      case _RETURN: {
        sscanf(buf, "%s %s", sbuf[0], sbuf[1]);
        operand *r = parse_opr(sbuf[1]);
        intercode *rt = new_ret_ir(r);
        ir_pushback(rt);
      } break;
      case ARG: {
        sscanf(buf, "%s %s", sbuf[0], sbuf[1]);
        operand *arg = parse_opr(sbuf[1]);
        intercode *ag = new_arg_ir(arg);
        ir_pushback(ag);
      } break;
      case PARAM: {
        sscanf(buf, "%s %s", sbuf[0], sbuf[1]);
        operand *param = parse_opr(sbuf[1]);
        intercode *ag = new_param_ir(param);
        ir_pushback(ag);
      } break;
      case READ: {
        sscanf(buf, "%s %s", sbuf[0], sbuf[1]);
        operand *param = parse_opr(sbuf[1]);
        intercode *ag = new_read_ir(param);
        ir_pushback(ag);
      } break;
      case WRITE: {
        sscanf(buf, "%s %s", sbuf[0], sbuf[1]);
        operand *param = parse_opr(sbuf[1]);
        intercode *ag = new_write_ir(param);
        ir_pushback(ag);
      } break;
      case _IF: {
        sscanf(buf, "IF %s %s %s GOTO %s", sbuf[0], sbuf[1], sbuf[2], sbuf[3]);
        operand *x = parse_opr(sbuf[0]), *y = parse_opr(sbuf[2]);
        char *relop = sbuf[1];
        operand *l = parse_label(sbuf[3]);
        intercode *ifgoto = new_ifgoto_ir(relop, x, y, l);
        ir_pushback(ifgoto);
      } break;
      case DEC: {
        sscanf(buf, "DEC %s %s", sbuf[0], sbuf[1]);
        operand *arr = parse_opr(sbuf[0]);
        operand *size = parse_opr(sbuf[1]);
        intercode *dec = new_dec_ir(arr, size);
        ir_pushback(dec);
      } break;
      default:
        assert(0);
      }
    }
  }
}
#undef FUNCTION
#undef LABEL
#undef GOTO
#undef _IF
#undef _RETURN
#undef DEC
#undef ARG
#undef CALL
#undef PARAM
#undef READ
#undef WRITE
