#include "type.h"
#include "common.h"
#include "list.h"
#include "syntax.tab.h"

static base_type *__new_btype(int dectype, const char *struct_name) {
  base_type *btype = malloc(sizeof(base_type));
  btype->dectype = dectype;
  btype->struct_name = struct_name;
  return btype;
}

base_type *new_literal(int dectype) { return __new_btype(dectype, NULL); }

base_type *new_struct_type(const char *struct_name) {
  return __new_btype(STRUCT, struct_name);
}

static cmm_type *__new_cmm_type(int is_basetype, int ctype,
                                base_type *return_type, base_type *btype) {
  cmm_type *c = malloc(sizeof(cmm_type));
  c->is_basetype = is_basetype;
  c->ctype = ctype;
  if (is_basetype) {
    assert(btype);
    c->btype = btype;
  } else {
    assert(return_type);
    c->return_type = return_type;
  }
  list_init(&c->link);
  list_init(&c->contain_types);
  c->contain_len = -1;
  c->errcode = NO_ERR;
  return c;
}

cmm_type *new_errtype(int errcode) {
  cmm_type *err = malloc(sizeof(cmm_type));
  err->errcode = errcode;
  return err;
}

int cmm_compute_len(cmm_type *ptr) {
  if (ptr->contain_len != -1)
    return ptr->contain_len;
  int cnt = 0;
  list_entry *p;
  list_for_each(p, &ptr->contain_types) { ++cnt; }
  ptr->contain_len = cnt;
  return cnt;
}

cmm_type *new_cmm_btype(base_type *btype) {
  return __new_cmm_type(1, -1, NULL, btype);
}

cmm_type *new_cmm_ctype(int ctype, base_type *return_type) {
  return __new_cmm_type(0, ctype, return_type, NULL);
}

void ctype_add(cmm_type *ptr, cmm_type *prev, cmm_type *next) {
  __list_add(&prev->link, &next->link, &ptr->link);
}

void ctype_add_head(cmm_type *ptr, cmm_type *head) {
  list_add(&ptr->link, &head->link);
}

void ctype_add_tail(cmm_type *ptr, cmm_type *head) {
  list_add_tail(&ptr->link, &head->link);
}

void ctype_set_contain_type(cmm_type *contain, cmm_type *head) {
  contain->contain_types = head->contain_types;
  contain->contain_len = -1;
}

/**
 * @brief like strcmp, but compare base_type*
 */
int btypecmp(base_type *b1, base_type *b2) {
  if (b1->dectype == b2->dectype) {
    if (b1->dectype == STRUCT) {
      return !strcmp(b1->struct_name, b2->struct_name);
    }
    return 0;
  }
  return 1;
}
/**
 * @brief like strcmp, but compare cmm_type*
 */
int ctypecmp(cmm_type *c1, cmm_type *c2) {
  if (c1->errcode != NO_ERR || c2->errcode != NO_ERR)
    return 0;
  if (c1->is_basetype && c2->is_basetype) {
    return btypecmp(c1->btype, c2->btype);
  }
  if (!c1->is_basetype && !c2->is_basetype) {
    if (c1->ctype != c2->ctype)
      return 1;
    int bcmp = btypecmp(
        c1->btype, c2->btype); // i am too lazy to specify ctypes. union anyway.
    if (bcmp)
      return bcmp;
    if (c1->ctype == TYPE_ARR) { // array dimensions.
      return c1->contain_len - c2->contain_len;
    } else {
      cmm_type *pos;
      list_entry *head = &c1->contain_types;
      list_entry *p2head = &c2->contain_types;
      cmm_type *p2 = le2(cmm_type, p2head->next, link);
      list_for_each_item(pos, head, link) {
        if (ctypecmp(pos, p2))
          return 1;
        p2 = le2(cmm_type, p2->link.next, link);
      }
      return 0;
    }
  }
  return 1;
}