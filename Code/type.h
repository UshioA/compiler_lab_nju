#ifndef __CMM_TYPE_H__
#define __CMM_TYPE_H__
#include "common.h"
#include "list.h"
#include "syntax.tab.h"
typedef struct base_type {
  int dectype;
  const char *struct_name; // if is struct.
} base_type;               // `Specifier' in Grammar.

typedef struct {
  int is_basetype;
  union {
    base_type *btype;
    base_type *return_type; // return_type for functions, union is dogshit.
  };
  enum {
    TYPE_ARR,
    TYPE_FUNC,
    TYPE_ERR,
  } ctype; // indicates complex type. not sure if i should add `STRUCT' into
           // this.

  enum {
    NO_ERR,
    ERR_UNDEFINE,
    ERR_REDEFINE,
    ERR_TYPEDISMATCH,
  } errcode;
  list_entry link;
  union {
    list_entry struct_fields; // struct
    list_entry param_types;   // func
    list_entry contain_types; // summary dogshit.
  };
  int contain_len;
} cmm_type;

static base_type *__new_btype(int dectype, const char *struct_name);

base_type *new_literal(int dectype);

base_type *new_struct_type(const char *struct_name);

static cmm_type *__new_cmm_type(int is_basetype, int ctype,
                                base_type *return_type, base_type *btype);

cmm_type *new_cmm_btype(base_type *btype);

cmm_type *new_cmm_ctype(int ctype, base_type *return_type);

cmm_type *new_errtype(int errcode);

int cmm_compute_len(cmm_type *ptr);

void ctype_add(cmm_type *ptr, cmm_type *prev, cmm_type *next);

void ctype_add_head(cmm_type *ptr, cmm_type *head);

void ctype_add_tail(cmm_type *ptr, cmm_type *head);

void ctype_set_contain_type(cmm_type *contain, cmm_type *head);

int btypecmp(base_type *b1, base_type *b2);

int ctypecmp(cmm_type *c1, cmm_type *c2);

#endif