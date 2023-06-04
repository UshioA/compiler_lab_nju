#include "semantic.h"
#include "ast.h"
#include "common.h"
#include "frame.h"
#include "list.h"
#include "symtab.h"
#include "syntax.tab.h"
#include "type.h"
#include <stdint.h>
#include <stdio.h>
#include <time.h>
frame *global;
frame *currf; // this name is bad =)
cmm_type *temphead;
int semanerr;

static char *randstr() {
  char alphabet[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_";
  char *target = calloc(9, sizeof(char));
  for (int i = 0; i < 8; ++i) {
    target[i] = alphabet[rand() % strlen(alphabet)];
  }
  return target;
}

void do_semantic(ast_node *root) {
  init_semantic();
  if (root)
    program(root);
}

void TODO() {
  fprintf(stderr, "not implemented\n");
  assert(0);
}

void program(ast_node *root) {
  ast_node *_extdeflist = get_child_n(root, 0);
  extdeflist(_extdeflist);
}

void extdeflist(ast_node *root) {
  ast_node *_extdef = get_child_n(root, 0);
  ast_node *_extdeflist = get_child_n(root, 1);
  extdef(_extdef); // guaranteed by grammar.
  if (_extdeflist) {
    extdeflist(_extdeflist);
  }
}

void extdef(ast_node *root) {
  ast_node *_specifier = get_child_n(root, 0);
  ast_node *ch2 = get_child_n(root, 1);
  ast_node *ch3 = get_child_n(root, 2);
  cmm_type *spec;
  if (ch2->symbol == ExtDecList) {
    spec = specifier(_specifier, NULL);
    if (spec->errcode == ERR_REDEFINE) {
      error(16, _specifier->lineno);
    } else if (spec->errcode == ERR_UNDEFINE) {
      error(17, _specifier->lineno);
    }
    extdeclist(ch2, spec);
  } else if (ch2->symbol == SEMI) {
    spec = specifier(_specifier, NULL);
    if (spec->errcode == ERR_REDEFINE) {
      error(16, _specifier->lineno);
    }
  } else {
    spec = specifier(_specifier, NULL);
    if (spec->errcode == ERR_REDEFINE) {
      error(16, _specifier->lineno);
    } else if (spec->errcode == ERR_UNDEFINE) {
      error(17, _specifier->lineno);
    }
    enter_scope();
    cmm_type *paramlist = fundec(ch2, spec);
    if (paramlist->errcode == ERR_REDEFINE) {
      error(4, root->lineno);
    }
    compst(ch3, spec);
    exit_scope();
  }
}

cmm_type *specifier(ast_node *root, cmm_type *_struct) {
  ast_node *ch1 = get_child_n(root, 0);
  if (ch1->symbol == TYPE) {
    return new_cmm_btype(
        new_literal(!strcmp("int", ch1->value.str_val) ? INT : FLOAT));
  } else {
    return structspecifier(ch1, _struct);
  }
}

cmm_type *structspecifier(ast_node *root, cmm_type *_struct) {
  ast_node *_opttag = get_child_n(root, 1);
  ast_node *_deflist;
  char *_id;
  if (root->childnum > 2) {
    if (_opttag->symbol == OptTag) { // struct opttag lc deflist rc
      _deflist = get_child_n(root, 3);
      _id = opttag(_opttag);
    } else if (_opttag->symbol == LC) {
      _deflist = get_child_n(root, 2);
      _id = randstr();
    } else assert(0);
    symbol *some_struct = frame_lookup(global, _id);
    if (some_struct) // already defined.
      return new_errtype(ERR_REDEFINE);
    if (_struct) {
      some_struct = frame_local_lookup(_struct->btype->struct_field, _id);
      if (some_struct) // already defined.
        return new_errtype(ERR_REDEFINE);
      some_struct = frame_lookup(global, _id); // struct is global;
      if (some_struct)
        return new_errtype(ERR_REDEFINE);
    }
    cmm_type *_struct = new_cmm_btype(new_struct_type(_id));
    symset(global->stab, _id, make_ssymbol(_id, _struct)); // for type checking.
    init_tempnode();
    cmm_type *struct_fields = deflist(_deflist, 1, _struct);
    ctype_set_contain_type(_struct, struct_fields);
    symset(global->stab, _id, make_ssymbol(_id, _struct));
    return _struct;
  } else { // struct tag, meant to get some defined type.
    char *_id = opttag(get_child_n(root, 1)); // i am too lazy.
    assert(_id);
    symbol *some_struct = frame_lookup(global, _id);
    if (some_struct && some_struct->type->btype->dectype == STRUCT) {
      return ctypecpy(some_struct->type);
    }
    return new_errtype(ERR_UNDEFINE);
  }
}

void extdeclist(ast_node *root, cmm_type *spec) {
  ast_node *_vardec = get_child_n(root, 0);
  ast_node *_extdeclist = get_child_n(root, 2);
  vardec(_vardec, ctypecpy(spec), 0, NULL);
  if (_extdeclist) {
    extdeclist(_extdeclist, spec);
  }
}

char *vardec_ass(ast_node *root, uint32_t **shape, int *size,
                        int *capacity) {
  if (root->symbol == ID)
    return root->value.str_val;
  else if (root->symbol == VarDec) {
    uint32_t ival = root->lpeer->lpeer->value.int_val; // VarDec LB {INT} RB
    if (*size >= *capacity) {
      *capacity <<= 1;
      uint32_t *_new = malloc(*capacity * sizeof(uint32_t));
      memset(_new, 0, sizeof(uint32_t) * (*capacity));
      memcpy(_new, *shape, *size * sizeof(uint32_t));
      free(*shape); // not leak =(
      *shape = _new;
    }
    (*shape)[*size] = ival;
    ++(*size);
    return vardec_ass(get_child_n(root, 0), shape, size, capacity);
  }
  assert(0);
}

symbol *vardec(ast_node *root, cmm_type *type, int isfield,
                      cmm_type *_struct) {
  ast_node *ch1 = get_child_n(root, 0);
  int *size = malloc(sizeof(int));
  int *capacity = malloc(sizeof(int));
  *capacity = 16;
  *size = 0;
  uint32_t *shape = calloc(*capacity, sizeof(uint32_t));
  char *name = vardec_ass(ch1, &shape, size, capacity);
  symbol *sym = isfield ? frame_local_lookup(_struct->btype->struct_field, name)
                        : symget(currf->stab, name);
  if (sym) {
    error(isfield ? 15 : 3, root->lineno);
  } else {
    sym = isfield ? frame_lookup(_struct->btype->struct_field, name)
                  : frame_lookup(currf, name);
    if (sym &&
        (sym->type->is_basetype && sym->type->btype->dectype == STRUCT)) {
      error(isfield ? 15 : 3, root->lineno);
    }
  }
  if (*size) { // array
    type->is_basetype = 0;
    type->ctype = TYPE_ARR;
    type->contain_len = *size;
    type->dimensions = calloc(*size + 1, sizeof(uint32_t));
    memcpy(type->dimensions + 1, shape, *size * sizeof(uint32_t));
    type->dimensions[0] = *size;
    sym = make_asymbol(name, type, shape);
    uint32_t *dimension = malloc(sizeof(uint32_t) * ((*size) + 1));
    memcpy(dimension + 1, shape, *size * sizeof(uint32_t));
    free(shape);
    dimension[0] = *size; // use dimension[0] as len.
    sym->dimension = dimension;
  } else {
    sym = make_symbol(
        name, type); // here type can only be btype, int, float, or struct.
  }
  if (isfield) {
    frame_add(_struct->btype->struct_field, sym->name, sym);
  } else {
    frame_add(currf, sym->name, sym);
  }
  return sym;
}

cmm_type *fundec(ast_node *root, cmm_type *rtype) {
  ast_node *_id = get_child_n(root, 0);
  if (root->childnum == 4) {
    symbol *sym =
        frame_lookup(currf,
                     _id->value.str_val); // global is guarenteed. but i like to
                                          // write currf->parent =)
    symbol *original = sym;
    ast_node *vlist = get_child_n(root, 2);
    cmm_type *ftype = new_cmm_ctype(TYPE_FUNC, rtype->btype);
    cmm_type *_varlist =
        new_cmm_ctype(-1, rtype->btype); //捏着鼻子看一下参数列表有没有错误
    varlist(vlist, _varlist);
    ctype_set_contain_type(ftype, _varlist);
    sym = make_funsymbol(_id->value.str_val, ftype);
    frame_add(currf, sym->name, sym);
    if (original) {
      return new_errtype(ERR_REDEFINE);
    }
    return ftype;
  } else {
    symbol *sym = frame_lookup(currf, _id->value.str_val);
    if (sym) {
      return new_errtype(ERR_REDEFINE);
    }
    cmm_type *ftype = new_cmm_ctype(TYPE_FUNC, rtype->btype);
    sym = make_funsymbol(_id->value.str_val, ftype);
    cmm_compute_len(ftype);
    frame_add(currf, sym->name, sym);
    return ftype;
  }
}

void varlist(ast_node *root, cmm_type *_v) {
  ast_node *_paramdec = get_child_n(root, 0);
  ast_node *_varlist = get_child_n(root, 2);
  if (_paramdec)
    paramdec(_paramdec, _v);
  if (_varlist) {
    varlist(_varlist, _v);
  }
}

void paramdec(ast_node *root, cmm_type *_v) {
  ast_node *_specifier = get_child_n(root, 0);
  ast_node *_vardec = get_child_n(root, 1);
  if (_specifier) {
    cmm_type *spec = specifier(_specifier, NULL);
    vardec(_vardec, spec, 0, NULL)->is_arg = 1;
    ctype_add_tail(ctypecpy(spec), _v);
  }
}

void compst(ast_node *root, cmm_type *rtype) {
  ast_node *_deflist = get_child_n(root, 1);
  if (_deflist->symbol == DefList) {
    deflist(_deflist, 0, NULL);
    ast_node *_stmtlist = get_child_n(root, 2);
    if (_stmtlist->symbol == StmtList) {
      stmtlist(_stmtlist, rtype);
    }
  } else if (_deflist->symbol == StmtList)
    stmtlist(_deflist, rtype);
}

cmm_type *deflist(ast_node *root, int isfield, cmm_type *_struct) {
  if (!root)
    return temphead;
  ast_node *_def = get_child_n(root, 0);
  if (_def) {
    ast_node *_deflist = get_child_n(root, 1);
    cmm_type *first = def(_def, isfield, _struct);
    deflist(_deflist, isfield, _struct);
    ctype_add_tail(first, temphead);
    return temphead;
  }
  return temphead;
}

cmm_type *def(ast_node *root, int isfield, cmm_type *_struct) {
  ast_node *_specifier = get_child_n(root, 0);
  ast_node *_declist = get_child_n(root, 1);
  cmm_type *spec = specifier(_specifier, _struct);
  if (spec->errcode == NO_ERR) {
    declist(_declist, spec, isfield, _struct);
  } else if (spec->errcode == ERR_UNDEFINE) {
    error(17, _specifier->lineno);
  } else if (spec->errcode == ERR_REDEFINE) {
    error(16, _specifier->lineno);
  }
  return spec;
}

cmm_type *declist(ast_node *root, cmm_type *spec, int isfield,
                         cmm_type *_struct) {
  ast_node *_dec = get_child_n(root, 0);
  ast_node *_declist = get_child_n(root, 2);
  dec(_dec, ctypecpy(spec), isfield, _struct);
  if (_declist) {
    declist(_declist, spec, isfield, _struct);
  }
  return spec;
}

void dec(ast_node *root, cmm_type *spec, int isfield,
                cmm_type *_struct) {
  ast_node *_vardec = get_child_n(root, 0);
  ast_node *_exp = get_child_n(root, 2);
  symbol *sym = vardec(_vardec, spec, isfield, _struct);
  if (_exp) {
    if (isfield) {
      error(15, _exp->lineno);
    }
    cmm_type *exptype = expr(_exp);
    if (exptype == NO_ERR && ctypecmp(exptype, sym->type)) {
      error(5, _vardec->lineno);
    }
  }
}

cmm_type *expr(ast_node *root) { //屎
  switch (root->childnum) {
  case 1: { // ID INT FLOAT
    ast_node *ch = get_child_last(root);
    switch (ch->symbol) {
    case ID: {
      symbol *sym = frame_lookup(currf, ch->value.str_val);
      if (!sym) {
        error(1, ch->lineno);
        return new_errtype(ERR_UNDEFINE);
      }
      return ctypecpy(sym->type);
    } break;
    case INT:
    case FLOAT: {
      cmm_type *t = new_cmm_btype(new_literal(ch->symbol));
      cmm_type *s = ctypecpy(t);
      s->is_left = 0;
      return s;
    } break;
    default:
      assert(0);
    }
  } break;
  case 2: { // MINUS EXP | NOT EXP
    ast_node *_exp = get_child_last(root);
    cmm_type *etype = expr(_exp);
    ast_node *op = get_child_n(root, 0);
    if (op->symbol == NOT) {
      if (!etype->is_basetype || etype->btype->dectype == FLOAT) {
        error(7, _exp->lineno);
        return new_errtype(ERR_TYPEDISMATCH);
      }
    } else if (op->symbol == MINUS) {
      if (!etype->is_basetype || etype->btype->dectype == STRUCT) {
        error(7, _exp->lineno);
        return new_errtype(ERR_TYPEDISMATCH);
      }
    } else
      assert(0);
    return etype;
  }
  case 3: {
    ast_node *head = get_child_n(root, 0);
    switch (head->symbol) {
    case ID: { // function call
      symbol *sym = frame_lookup(currf, head->value.str_val);
      if (!sym || sym->type->ctype != TYPE_FUNC) {
        error(!sym ? 2 : 11, head->lineno);
        return new_errtype(ERR_UNDEFINE);
      }
      if (sym->type->contain_len) {
        error(9, head->lineno);
        return new_errtype(ERR_TYPEDISMATCH);
      }
      cmm_type *n = new_cmm_btype(sym->type->return_type);
      if (n->btype->dectype == INT || n->btype->dectype == FLOAT) {
        n->is_left = 0;
      }
      return n;
    } break;
    case LP: { // bullshit =)
      return expr(get_child_n(root, 1));
    } break;
    case Exp: {
      ast_node *ch2 = get_child_n(root, 1);
      ast_node *_id;
      char *n = randstr();
      switch (ch2->symbol) {
      case DOT: {
        symbol *sym = NULL;
        cmm_type *h = expr(head); // struct
        if (h->errcode == NO_ERR) {
          if (!h->is_basetype || h->btype->dectype != STRUCT) {
            error(13, head->lineno);
            return new_errtype(ERR_TYPEDISMATCH);
          }
        }
        _id = get_child_last(root); // struct._id
        n = _id->value.str_val;
        sym = frame_lookup(currf, h->btype->struct_name);
        if (!sym) {
          if (h->errcode == NO_ERR)
            error(1, head->lineno);
          return new_errtype(ERR_UNDEFINE);
        }
        symbol *field = frame_lookup(sym->type->btype->struct_field, n);
        if (!field) {
          error(14, _id->lineno);
          return new_errtype(ERR_UNDEFINE);
        }
        return ctypecpy(field->type);
      } break;
      case AND:
      case OR: {
        ast_node *exp1 = get_child_n(root, 0);
        ast_node *exp2 = get_child_n(root, 2);
        cmm_type *et1 = expr(exp1);
        cmm_type *et2 = expr(exp2);
        if (et1->errcode == NO_ERR && et2->errcode == NO_ERR) {
          if (!et1->is_basetype || !et2->is_basetype)
            return new_errtype(ERR_TYPEDISMATCH);
          if (!et1->btype->dectype != !et2->btype->dectype ||
              et1->btype->dectype != INT)
            return new_errtype(ERR_TYPEDISMATCH);
        }
        // TODO(); // should have errored.
        et1->is_left = 0;
        return et1;
      } break;
      case ASSIGNOP: {
        ast_node *e1 = get_child_n(root, 0);
        ast_node *e2 = get_child_n(root, 2);
        cmm_type *t1 = expr(e1);
        if (t1->errcode != NO_ERR)
          return t1;
        if (!t1->is_left) {
          error(6, e1->lineno);
          return new_errtype(ERR_TYPEDISMATCH);
        }
        cmm_type *t2 = expr(e2);
        if (ctypecmp(t1, t2)) {
          error(5, ch2->lineno);
          return new_errtype(ERR_TYPEDISMATCH);
        }
        // TODO(); do lrvalue check
        return t1;
      } break;
      case RELOP:
      case PLUS:
      case MINUS:
      case STAR:
      case DIV: {
        ast_node *exp1 = get_child_n(root, 0);
        ast_node *exp2 = get_child_last(root);
        cmm_type *et1 = expr(exp1);
        cmm_type *et2 = expr(exp2);
        if (et1->errcode == NO_ERR && et2->errcode == NO_ERR) {
          if (!et1->is_basetype || !et2->is_basetype) {
            error(7, exp1->lpeer->lineno);
            return new_errtype(ERR_TYPEDISMATCH);
          }
          if (et1->btype->dectype != et2->btype->dectype ||
              et1->btype->dectype == STRUCT) {
            error(7, exp1->lpeer->lineno);
            return new_errtype(ERR_TYPEDISMATCH);
          }
          if (ch2->symbol == RELOP) {
            return new_cmm_btype(new_literal(INT));
          }
          return ctypecpy(et1);
        }
        if (et1->errcode != NO_ERR)
          return ctypecpy(et1);
        return ctypecpy(et2);
      } break;
      default:
        assert(0);
      }
    }
    }
  } break;
  case 4: {
    ast_node *head = get_child_n(root, 0);
    if (head->symbol == ID) {
      ast_node *_args = get_child_n(root, 2);
      symbol *fsym = frame_lookup(currf, head->value.str_val);
      if (!fsym || fsym->type->ctype != TYPE_FUNC) {
        error(!fsym ? 2 : 11, head->lineno);
        return new_errtype(ERR_UNDEFINE);
      }
      if (fsym->type->is_basetype || fsym->type->ctype != TYPE_FUNC) {
        error(11, head->lineno);
        return new_errtype(ERR_TYPEDISMATCH);
      }
      cmm_type *iferr = args(_args, fsym->type);
      if (iferr->errcode != NO_ERR) {
        error(9, _args->lineno);
      }
      cmm_type *n = new_cmm_btype(fsym->type->return_type);
      if (n->btype->dectype == INT || n->btype->dectype == FLOAT) {
        n->is_left = 0;
      }
      n->errcode = iferr->errcode;
      return n;
    } else if (head->symbol == Exp) {
      cmm_type *atype = expr(head);
      if (atype->errcode == NO_ERR &&
          (atype->is_basetype || atype->ctype != TYPE_ARR)) {
        error(10, head->lineno);
        return new_errtype(ERR_TYPEDISMATCH);
      }
      ast_node *index = get_child_n(root, 2);
      index->arrtype = atype;
      head->arrtype = atype;
      cmm_type *_index = expr(index);
      if (_index->errcode == NO_ERR &&
          (!_index->is_basetype || _index->btype->dectype != INT)) {
        error(12, index->lineno);
        return new_errtype(ERR_TYPEDISMATCH);
      }
      cmm_type *c;
      if (atype->contain_len > 1) {
        c = new_cmm_ctype(TYPE_ARR, atype->btype);
        c->contain_len = atype->contain_len - 1;
        c->dimensions = calloc(atype->contain_len + 1, sizeof(uint32_t));
        c->dimensions[0] = c->contain_len;
        memcpy(c->dimensions + 1, atype->dimensions + 1,
               atype->contain_len * sizeof(uint32_t));
        c->errcode =
            atype->errcode != NO_ERR ? atype->errcode : _index->errcode;
        return c;
      }
      c = new_cmm_btype(atype->btype);
      c->errcode = atype->errcode != NO_ERR ? atype->errcode : _index->errcode;
      return c;
    } else
      assert(0);
  } break;
  default:
    assert(0);
  }
  assert(0);
  // should never come here.
  return NULL;
}

char *opttag(ast_node *root) {
  ast_node *_id = get_child_n(root, 0);
  if (_id)
    return _id->value.str_val;
  return NULL;
}

cmm_type *args_ass(ast_node *root, cmm_type *paramtypes) {
  ast_node *e1 = get_child_n(root, 0);
  e1->isarg = 1;
  ast_node *args = get_child_n(root, 2);
  cmm_type *t1 = expr(e1);
  cmm_type *v = ctypecpy(t1);
  ctype_add_tail(v, paramtypes);
  if (args) {
    args->isarg = 1;
    return args_ass(args, paramtypes);
  }
  return paramtypes;
}
cmm_type *args(ast_node *root, cmm_type *functype) {
  cmm_type *atype = new_cmm_ctype(-1, new_literal(INT));
  atype = args_ass(root, atype);
  cmm_type *wrapper = new_cmm_ctype(TYPE_FUNC, functype->return_type);
  ctype_set_contain_type(wrapper, atype);
  if (ctypecmp(wrapper, functype)) {
    return new_errtype(ERR_TYPEDISMATCH);
    // error(9, root->lineno);
  }
  return functype;
}

void stmtlist(ast_node *root, cmm_type *rtype) {
  if (!root) {
    return;
  }
  ast_node *_stmt = get_child_n(root, 0);
  ast_node *_stlist = get_child_n(root, 1);
  stmt(_stmt, rtype);
  stmtlist(_stlist, rtype);
}

void stmt(ast_node *root, cmm_type *rtype) {
  if (!root)
    return;
  ast_node *head = get_child_n(root, 0);
  switch (head->symbol) {
  case Exp: {
    expr(head);
  } break;
  case CompSt: {
    enter_scope();
    compst(head, rtype); // if you return in a new scope, it counts, so
                         // propagating rtype is needed.
    exit_scope();
  } break;
  case RETURN: {
    ast_node *rexpr = get_child_n(root, 1);
    cmm_type *rexpr_type = expr(rexpr);
    if (ctypecmp(rexpr_type, rtype)) {
      error(8, rexpr->lineno);
    }
  } break;
  case WHILE: {
    ast_node *_exp = get_child_n(root, 2);
    ast_node *_stmt = get_child_last(root);
    cmm_type *etype = expr(_exp);
    if (etype->errcode == NO_ERR &&
        (!etype->is_basetype || etype->btype->dectype != INT)) {
      error(7, _exp->lineno);
    }
    stmt(_stmt, rtype);
  } break;
  case IF: {
    ast_node *_exp = get_child_n(root, 2);
    ast_node *istmt = get_child_n(root, 4);
    ast_node *estmt = get_child_last(root);
    cmm_type *etype = expr(_exp);
    if (etype->errcode == NO_ERR &&
        (!etype->is_basetype || etype->btype->dectype != INT)) {
      error(7, _exp->lineno);
    }
    stmt(istmt, rtype);
    if (estmt != istmt) { // indicates the rule is `if(exp) stmt else stmt'
      stmt(estmt, rtype);
    }
  } break;
  default: {
    printf("what is this %s\n", head->value.str_val);
    assert(0);
  } break;
  }
}