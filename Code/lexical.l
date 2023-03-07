%{
#include "syntax.tab.h"
#include "common.h"
#include "ast.h"
int yycolumn=1;
#define DEBUG
#ifdef DEBUG
#define Log(a, ...)                                                            \
  do {                                                                         \
    printf("\e[35mSaw\e[0m:" a "\n",##__VA_ARGS__);    \
  } while (0)
#else 
#define Log(a, ...) do{}while(0)
#endif
#define YY_USER_ACTION \
  yylloc.first_line = yylloc.last_line = yylineno; \
  yylloc.first_column = yycolumn; \
  yylloc.last_column=yycolumn+yyleng-1; \
  yycolumn+=yyleng;

extern ast_node* yylval;
ast_value empty;
int lexerr=0;
int parerr=0;

#define LEXERR() \
  do{\
    fprintf(stderr, "Error type A at Line %d: Mysterious character %s\n", yylineno, yytext);\
    lexerr=1;\
  }while(0)

#define MAKE_INODE(n) \
  ast_value av;\
  if(n==INT)\
    av.int_val = strtoul(yytext, NULL, 0);\
  else\
    av.float_val = atof(yytext);\
  yylval = make_ast_term(n, av);\
  return n;

#define MAKE_SNODE(n) \
  ast_value av;\
  av.str_val = strdup(yytext);\
  yylval = make_ast_term(n,av);\
  return n;

#define MAKE_ENODE(n)\
  yylval = make_ast_term(n, empty);\
  return n;

#define MAKE_ERR(n)\
  yylval = make_ast_term(n, empty);\
  return n;

%}
/* %define api.value.type { ast_node* } */
%option yylineno
decimal_digit ([0-9])
hexdecimal_digit ([0-9A-Fa-f])
hexdecimal_prefix (0x)|(0X)
octal_digit ([0-7])
octal_prefix (0)
letter ([A-Za-z_])
semi (;)
comma (,)
assignop (=)
relop ((<)|(>)|(>=)|(<=)|(==)|(!=))
if ("if")
else ("else")
while ("while")
return ("return")
struct ("struct")
int ("int")
float ("float")
whitespace ([ \t\n\r])
herr 0[xX][0-9a-fA-F]*[g-zG-Z]+[0-9a-zA-Z]*
oerr 0[0-7]*[8-9]+[0-9]*
%%
{herr} {LEXERR();MAKE_INODE(INT);}
{oerr} {LEXERR();MAKE_INODE(INT);}
[0]{1}|([1-9]{decimal_digit}{0,31}) {MAKE_INODE(INT);}
{octal_prefix}{octal_digit}+ {MAKE_INODE(INT);}
{hexdecimal_prefix}{hexdecimal_digit}+ {MAKE_INODE(INT);}
{decimal_digit}+\.{decimal_digit}+ {MAKE_INODE(FLOAT);}
{int} {MAKE_SNODE(TYPE);}
{float} {MAKE_SNODE(TYPE);}
{struct} {MAKE_ENODE(STRUCT);}
{return} {MAKE_ENODE(RETURN);}
{if} {MAKE_ENODE(IF);}
{else} {MAKE_ENODE(ELSE);}
{while} {MAKE_ENODE(WHILE);}
{letter}[0-9A-Za-z_]* {MAKE_SNODE(ID);}
{assignop} {MAKE_ENODE(ASSIGNOP);}
{semi} {MAKE_ENODE(SEMI);}
{comma} {MAKE_ENODE(COMMA);}
{relop} {MAKE_SNODE(RELOP);}
"+" {MAKE_ENODE(PLUS);}
"-" {MAKE_ENODE(MINUS);}
"*" {MAKE_ENODE(STAR);}
"/" {MAKE_ENODE(DIV);}
"&&" {MAKE_ENODE(AND);}
"||" {MAKE_ENODE(OR);}
"." {MAKE_ENODE(DOT);}
"!" {MAKE_ENODE(NOT);}
"(" {MAKE_ENODE(LP);}
")" {MAKE_ENODE(RP);}
"[" {MAKE_ENODE(LB);}
"]" {MAKE_ENODE(RB);}
"{" {MAKE_ENODE(LC);}
"}" {MAKE_ENODE(RC);}
"\n" {yycolumn=1;}
{whitespace}+ {;}
<<EOF>> {MAKE_ENODE(EOF);}
. {fprintf(stderr, "Error type A at Line %d: Mysterious character %s\n", yylineno, yytext);lexerr=1;}
%%