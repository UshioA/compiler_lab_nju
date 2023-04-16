#ifndef __CMM_TRANSLATE_H__
#define __CMM_TRANSLATE_H__
#include "ast.h"
#include "frame.h"
#include "ir.h"
#include "list.h"
#include "symtab.h"
#include <stdio.h>
extern intercode *ircode;
extern ast_node *ast_root;
extern frame *global;
extern FILE *f;

void init_file(FILE *_f);

void emit_code(intercode *ir);

void init_translate(FILE* _f);

void dump_code();

void translate_program(ast_node *root);
void translate_extdeflist(ast_node *root);
void translate_extdef(ast_node *root);
void translate_fundec(ast_node *root);
void translate_compst(ast_node *root);
void translate_varlist(ast_node *root);
void translate_paramdec(ast_node *root);
symbol *translate_vardec(ast_node *root, int isfunc);
void translate_deflist(ast_node *root);
void translate_def(ast_node *root);
void translate_declist(ast_node *root);
void translate_dec(ast_node *root);
void translate_expr(ast_node *root, operand *);
void translate_cond(ast_node *exp, operand *label_true, operand *label_false,
                    int cond);
void translate_args(ast_node *root, operand **arg_list, int index);
void translate_stmtlist(ast_node *root);
void translate_stmt(ast_node *root);
#endif