#include "semantic.h"
#include "ast.h"
#include "frame.h"
#include "list.h"
#include "symtab.h"
#include "syntax.tab.h"
#include "type.h"
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

static void program(ast_node *root) {
  ast_node *_extdeflist = get_child_n(root, 0);
  extdeflist(_extdeflist);
}

static void extdeflist(ast_node *root) {
  ast_node *_extdef = get_child_n(root, 0);
  ast_node *_extdeflist = get_child_n(root, 1);
  extdef(_extdef); // guaranteed by grammar.
  if (_extdeflist) {
    extdeflist(_extdeflist);
  }
}

static void extdef(ast_node *root) {
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

static cmm_type *specifier(ast_node *root) {
  ast_node *ch1 = get_child_n(root, 0);
  if (ch1->symbol == TYPE) {
    return new_cmm_btype(
        new_literal(!strcmp("int", ch1->value.str_val) ? INT : FLOAT));
  } else {
    return structspecifier(ch1);
  }
}

static void extdeclist(ast_node *root, cmm_type *spec) {
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

static symbol *vardec(ast_node *root, cmm_type *type, int isfield) {
  ast_node *ch1 = get_child_n(root, 0);
  int *size = malloc(sizeof(int));
  int *capacity = malloc(sizeof(int));
  *capacity = 16;
  *size = 0;
  uint32_t *shape = malloc(sizeof(uint32_t) * (*capacity));
  memset(shape, 0, *capacity * sizeof(uint32_t));
  char *name = vardec_ass(ch1, &shape, size, capacity);
  symbol *sym =
      isfield ? symget(currf->sstab, name) : symget(currf->stab, name);
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
    if (isfield) {   // ! TODO: check redefine
      frame_adds(currf, name, sym);
    } else {
      frame_add(currf, name, sym);
    }
  }
  return sym;
}

static cmm_type *fundec(ast_node *root, cmm_type *rtype) {
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

static void varlist(ast_node *root, cmm_type *_v) {
  ast_node *_paramdec = get_child_n(root, 0);
  ast_node *_varlist = get_child_n(root, 2);
  paramdec(_paramdec, _v);
  if (_varlist) {
    varlist(_varlist, _v);
  }
}

static void paramdec(ast_node *root, cmm_type *_v) {
  ast_node *_specifier = get_child_n(root, 0);
  ast_node *_vardec = get_child_n(root, 1);
  cmm_type *spec = specifier(_specifier);
  ctype_add_tail(spec, _v);
  vardec(_vardec, spec, 0);
}

static void compst(ast_node *root, cmm_type *rtype) {
  ast_node *_deflist = get_child_n(root, 1);
  ast_node *_stmtlist = get_child_n(root, 2);
  deflist(_deflist, 0);
  stmtlist(_stmtlist, rtype);
}

static cmm_type *deflist(ast_node *root, int isfield) {
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

static cmm_type *def(ast_node *root, int isfield) {
  ast_node *_specifier = get_child_n(root, 0);
  ast_node *_declist = get_child_n(root, 1);
  cmm_type *spec = specifier(_specifier);
  if (spec->errcode == NO_ERR) {
    declist(_declist, spec, isfield);
  }
  return spec;
}

static cmm_type *declist(ast_node *root, cmm_type *spec, int isfield) {
  ast_node *_dec = get_child_n(root, 0);
  ast_node *_declist = get_child_n(root, 2);
  dec(_dec, spec, isfield);
  if (_declist) {
    declist(_declist, spec, isfield);
  }
  return spec;
}

static void dec(ast_node *root, cmm_type *spec, int isfield) {
  ast_node *_vardec = get_child_n(root, 0);
  ast_node *_exp = get_child_n(root, 2);
  symbol *sym = vardec(_vardec, spec, isfield);
  if (_exp) {
    cmm_type *exptype = expr(_exp);
    if (exptype == NO_ERR && ctypecmp(exptype, sym->type)) {
      error(5, _vardec->lineno);
    }
  }
  if (isfield) {
    frame_adds(currf, sym->name, sym);
  } else {
    frame_add(currf, sym->name, sym);
  }
}

static cmm_type *expr(ast_node *root) { //屎
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
    cmm_type *etype = expr(_exp);
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
      return expr(get_child_n(root, 1));
    } break;
    case Exp: {
      ast_node *ch2 = get_child_n(root, 1);
      switch (ch2->symbol) {
      case DOT: {
        cmm_type *h = expr(head); // struct
        if (h->errcode == NO_ERR) {
          if (!h->is_basetype || h->btype->dectype != STRUCT) {
            error(13, head->lineno);
            return new_errtype(ERR_TYPEDISMATCH);
          }
          ast_node *_id = get_child_last(root); // struct._id
          char *n = _id->value.str_val;
          symbol *sym = frame_lookup(currf, h->btype->struct_name);
          if (!sym) {
            error(1, head->lineno);
            return new_errtype(ERR_UNDEFINE);
          }
          symbol *field = frame_slookup(currf, n);
          if (!field) {
            error(14, _id->lineno);
            return new_errtype(ERR_UNDEFINE);
          }
          return field->type;
        }
        return h;
      } break;
      case AND:
      case OR: {
        ast_node *exp1 = get_child_n(root, 0);
        ast_node *exp2 = get_child_last(root);
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
        return et1;
      } break;
      case ASSIGNOP: {
        ast_node *e1 = get_child_n(root, 0);
        ast_node *e2 = get_child_n(root, 2);
        cmm_type *t1 = expr(e1);
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
          if (!et1->is_basetype || !et2->is_basetype)
            return new_errtype(ERR_TYPEDISMATCH);
          if (!et1->btype->dectype != !et2->btype->dectype ||
              et1->btype->dectype == STRUCT)
            return new_errtype(ERR_TYPEDISMATCH);
        }
        // TODO(); // should have errored.
        return et1;
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
      if (!fsym) {
        error(2, head->lineno);
        return new_errtype(ERR_UNDEFINE);
      }
      args(_args, fsym->type);
      return new_cmm_btype(fsym->type->return_type);
    } else if (head->symbol == Exp) {
      cmm_type *atype = expr(head);
      if (atype->errcode == NO_ERR &&
          (atype->is_basetype || atype->ctype != TYPE_ARR)) {
        error(10, head->lineno);
        return new_errtype(ERR_TYPEDISMATCH);
      }
      ast_node *index = get_child_n(root, 2);
      cmm_type *_index = expr(index);
      if (_index->errcode == NO_ERR &&
          (!_index->is_basetype || _index->btype->dectype != INT)) {
        error(12, index->lineno);
        return new_errtype(ERR_TYPEDISMATCH);
      }
      TODO(); // lvalue
      return new_cmm_btype(atype->btype);
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

static cmm_type *structspecifier(ast_node *root) {
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

static char *opttag(ast_node *root) {
  ast_node *_id = get_child_n(root, 0);
  if (_id)
    return _id->value.str_val;
  return NULL;
}

static void args(ast_node *root, cmm_type *functype) {}

static void stmtlist(ast_node *root, cmm_type *rtype) {
  if (!root) {
    return;
  }
  ast_node *_stmt = get_child_n(root, 0);
  ast_node *_stlist = get_child_n(root, 1);
  stmt(_stmt, rtype);
  stmtlist(_stlist, rtype);
}

static void stmt(ast_node *root, cmm_type *rtype) {
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
  default:{
    printf("what is this %s\n", head->value.str_val);
    assert(0);
  }break;
  }
}