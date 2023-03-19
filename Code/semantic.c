#include "semantic.h"
#include "ast.h"
#include "frame.h"
#include "list.h"
#include "symtab.h"
#include "syntax.tab.h"
#include "type.h"
#include <assert.h>
#include <stdio.h>
frame *global;
frame *currf; // this name is bad =)
void do_semantic(ast_node *root) {
  init_semantic();
  if (root)
    program(root);
}

static void TODO() {
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
    spec = specifier(_specifier);
    extdeclist(ch2, spec);
  } else if (ch2->symbol == SEMI) {
    spec = specifier(_specifier);
  } else {
    spec = specifier(_specifier);
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

cmm_type *specifier(ast_node *root) {
  ast_node *ch1 = get_child_n(root, 0);
  if (ch1->symbol == TYPE) {
    return new_cmm_btype(
        new_literal(!strcmp("int", ch1->value.str_val) ? INT : FLOAT));
  } else {
    return structspecifier(ch1);
  }
}

void extdeclist(ast_node *root, cmm_type *spec) {
  ast_node *_vardec = get_child_n(root, 0);
  ast_node *_extdeclist = get_child_n(root, 2);
  vardec(_vardec, spec, 0);
  if (_extdeclist) {
    extdeclist(_extdeclist, spec);
  }
}

static char *vardec_ass(ast_node *root, uint32_t **shape, int *size,
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
    *shape[*size] = ival;
    ++(*size);
    return vardec_ass(get_child_n(root, 0), shape, size, capacity);
  }
  assert(0);
}

symbol *vardec(ast_node *root, cmm_type *type, int isfield) {
  ast_node *ch1 = get_child_n(root, 0);
  int *size = malloc(sizeof(int));
  int *capacity = malloc(sizeof(int));
  *capacity = 16;
  *size = 0;
  uint32_t *shape = malloc(sizeof(uint32_t) * (*capacity));
  memset(shape, 0, *capacity * sizeof(shape));
  char *name = vardec_ass(ch1, &shape, size, capacity);
  symbol *sym = symget(currf->stab, name);
  if (sym) {
    error(type->btype->dectype == STRUCT ? 15 : 3, root->lineno);
  }
  if (*size) { // array
    sym = make_asymbol(name, type, shape);
    uint32_t *dimension = malloc(sizeof(uint32_t) * ((*size) + 1));
    memcpy(dimension + 1, shape, *size * sizeof(uint32_t));
    free(shape);
    dimension[0] = *size; // use dimension[0] as len.
    sym->dimension = dimension;
    if (isfield)
      frame_adds(currf, name, sym);
    else
      frame_add(currf, name, sym);
  } else {
    sym = make_symbol(
        name, type); // here type can only be btype, int, float, or struct.
    if (isfield)
      frame_adds(currf, name, sym);
    else
      frame_add(currf, name, sym);
  }
  return sym;
}

cmm_type *fundec(ast_node *root, cmm_type *rtype) {
  ast_node *_id = get_child_n(root, 0);
  if (root->childnum == 4) {
    symbol *sym = symget(currf->stab, _id->value.str_val);
    if (sym) {
      return new_errtype(ERR_REDEFINE);
    }
    ast_node *vlist = get_child_n(root, 2);
    cmm_type *ftype = new_cmm_ctype(TYPE_FUNC, rtype->btype);
    cmm_type *_varlist = new_cmm_ctype(-1, rtype->btype);
    varlist(vlist, _varlist);
    ftype->contain_types = _varlist->contain_types;
    cmm_compute_len(ftype);
    sym = make_funsymbol(_id->value.str_val, ftype);
    symset(currf->stab, _id->value.str_val, sym);
    return ftype;
  } else {
    symbol *sym = symget(currf->stab, _id->value.str_val);
    if (sym) {
      return new_errtype(ERR_REDEFINE);
    }
    cmm_type *ftype = new_cmm_ctype(TYPE_FUNC, rtype->btype);
    sym = make_funsymbol(_id->value.str_val, ftype);
    cmm_compute_len(ftype);
    symset(currf->stab, _id->value.str_val, sym);
    return ftype;
  }
}

void varlist(ast_node *root, cmm_type *_v) {
  ast_node *_paramdec = get_child_n(root, 0);
  ast_node *_varlist = get_child_n(root, 2);
  paramdec(_paramdec, _v);
  if (_varlist) {
    varlist(_varlist, _v);
  }
}

void paramdec(ast_node *root, cmm_type *_v) {
  ast_node *_specifier = get_child_n(root, 0);
  ast_node *_vardec = get_child_n(root, 1);
  cmm_type *spec = specifier(_specifier);
  ctype_add_tail(spec, _v);
  vardec(_vardec, spec, 0);
}

void compst(ast_node *root, cmm_type *rtype) {
  ast_node *_deflist = get_child_n(root, 1);
  ast_node *_stmtlist = get_child_n(root, 2);
  deflist(_deflist, 0);
  stmtlist(_stmtlist);
}

cmm_type *deflist(ast_node *root, int isfield) {
  if (!root)
    return NULL;
  ast_node *_def = get_child_n(root, 0);
  ast_node *_deflist = get_child_n(root, 1);
  if (_def) {
    cmm_type *first = def(_def, isfield);
    cmm_type *last = deflist(_deflist, isfield);
    if (last)
      ctype_add_tail(last, first);
    return first;
  }
  return NULL;
}

cmm_type *def(ast_node *root, int isfield) {
  ast_node *_specifier = get_child_n(root, 0);
  ast_node *_declist = get_child_n(root, 1);
  cmm_type *spec = specifier(_specifier);
  declist(_declist, spec, isfield);
  return spec;
}

cmm_type *declist(ast_node *root, cmm_type *spec, int isfield) {
  ast_node *_dec = get_child_n(root, 0);
  ast_node *_declist = get_child_n(root, 2);
  dec(_dec, spec, isfield);
  if (_declist) {
    declist(_declist, spec, isfield);
  }
  return spec;
}

void dec(ast_node *root, cmm_type *spec, int isfield) {
  ast_node *_vardec = get_child_n(root, 0);
  ast_node *_exp = get_child_n(root, 2);
  symbol *sym = vardec(root, spec, isfield);
  if (_exp) {
    cmm_type *exptype = exp(_exp);
  }
}

cmm_type *exp(ast_node *root) { //屎
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
      return sym->type;
    } break;
    case INT:
    case FLOAT: {
      return new_cmm_btype(new_literal(ch->symbol));
    } break;
    default:
      assert(0);
    }
  } break;
  case 2: { // MINUS EXP | NOT EXP
    ast_node *_exp = get_child_last(root);
    cmm_type *etype = exp(_exp);
    ast_node *op = get_child_n(root, 0);
    if (op->symbol == NOT) {
      if (!etype->is_basetype || etype->btype->dectype != FLOAT) {
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
      if (!sym) {
        error(2, head->lineno);
        return new_errtype(ERR_UNDEFINE);
      }
      return sym->type;
    } break;
    case LP: { // bullshit =)
      return exp(get_child_n(root, 1));
    } break;
    case Exp: {
      ast_node *ch2 = get_child_n(root, 1);
      switch (ch2->symbol) {
      case DOT: {
        cmm_type *h = exp(head);
        if (!h->is_basetype || h->btype->dectype != STRUCT) {
          error(13, head->lineno);
          return new_errtype(ERR_TYPEDISMATCH);
        }
        ast_node *_id = get_child_last(root);
        char *n = _id->value.str_val;
        cmm_type *pos;
        list_entry *head = &h->struct_fields; // super deceiving myself
      } break;
      }
    }
    }
  } break;
  case 4: {
    assert(0);
  } break;
  default:
    assert(0);
  }
}

cmm_type *structspecifier(ast_node *root) {
  ast_node *_deflist = get_child_n(root, 3);
  if (_deflist) { // struct opttag lc deflist rc
    ast_node *_opttag = get_child_n(root, 1);
    char *_id = opttag(_opttag);
    cmm_type *_struct = new_cmm_btype(new_struct_type(_id));
    cmm_type *struct_fields =
        deflist(_deflist, 1); // feel free to def struct fields in frames =(
    _struct->struct_fields = struct_fields->link;
    symbol *some_struct = symget(currf->stab, _id);
    if (some_struct) // already defined.
      return new_errtype(ERR_REDEFINE);
    symset(currf->stab, _id, make_ssymbol(_id, _struct));
    return _struct;
  } else { // struct tag, meant to get some defined type.
    char *_id = opttag(get_child_n(root, 1)); // i am too lazy.
    assert(_id);
    symbol *some_struct = symget(currf->stab, _id);
    if (some_struct && some_struct->type->btype->dectype == STRUCT) {
      return some_struct->type;
    }
    return new_errtype(ERR_UNDEFINE);
  }
}

char *opttag(ast_node *root) {
  ast_node *_id = get_child_n(root, 0);
  if (_id)
    return _id->value.str_val;
  return NULL;
}
