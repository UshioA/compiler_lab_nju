%define api.value.type { ast_node* }
%{
  #include "common.h"
  #include "lex.yy.c"
  void yyerror(ast_node* para, char* fmt, ...);
  int yylex(void);
  void cmmerror(char* fmt, ...);
#define ERR(_, ...) \
  do{\
  }while(0)
  extern ast_node* ast_root; 
  extern int lexerr;
  extern int parerr;
%}
%code requires{
  #include "ast.h"
}
/* %destructor {free($$);} */
%parse-param { ast_node* para }
%locations
%token TYPE
%token INT FLOAT
%token ID
%token ASSIGNOP
%token SEMI COMMA
%token RELOP PLUS MINUS STAR DIV AND OR DOT NOT
%token LP RP LB RB LC RC
%token STRUCT RETURN IF ELSE WHILE

%right ASSIGNOP

%left OR

%left AND

%left RELOP

%left PLUS MINUS

%left STAR DIV

%right NOT

%left UMINUS
%left LP RP LB RB LC RC DOT

%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE

%%
/* High level Definitions */
Program: ExtDefList {
  if(lexerr){parerr = 1;}
  $$ = make_ast_nonterm(Program);
  ast_root = $$;
  add_ast_child($$, $1);
} | error {ERR();ast_root=$$=NULL;}
ExtDefList: ExtDef ExtDefList {
    $$ = make_ast_nonterm(ExtDefList);
    add_ast_child($$, $1);
    add_ast_child($$, $2);
  }
  | {$$ = NULL;} | error {ERR();$$=NULL;}
ExtDef: Specifier ExtDecList SEMI {
    $$ = make_ast_nonterm(ExtDef);
    add_ast_child($$, $1);
    add_ast_child($$, $2);
    add_ast_child($$, $3);
  }
  | Specifier SEMI {
    $$ = make_ast_nonterm(ExtDef);
    add_ast_child($$, $1);
    add_ast_child($$, $2);
  } 
  | Specifier FunDec CompSt {
    $$ = make_ast_nonterm(ExtDef);
    add_ast_child($$, $1);
    add_ast_child($$, $2);
    add_ast_child($$, $3);
  }
  | error {ERR();$$=NULL;}
ExtDecList: VarDec {
    $$ = make_ast_nonterm(ExtDecList);
    add_ast_child($$, $1);
  } 
  | VarDec COMMA ExtDecList {
    $$ = make_ast_nonterm(ExtDecList);
    add_ast_child($$, $1);
    add_ast_child($$, $2);
    add_ast_child($$, $3);
  }| error {ERR();$$=NULL;}
/* Specifiers */
Specifier: TYPE {
    $$ = make_ast_nonterm(Specifier);
    add_ast_child($$, $1);
  } 
  | StrtuctSpecifier {
    $$ = make_ast_nonterm(Specifier);
    add_ast_child($$, $1);
  }| error {ERR();$$=NULL;}
StrtuctSpecifier: STRUCT OptTag LC DefList RC {
                    $$ = make_ast_nonterm(StructSpecifier);
                    add_ast_child($$, $1);
                    add_ast_child($$, $2);
                    add_ast_child($$, $3);
                    add_ast_child($$, $4);
                    add_ast_child($$, $5);
                  }
                  | STRUCT Tag{
                    $$ = make_ast_nonterm(StructSpecifier);
                    add_ast_child($$, $1);
                    add_ast_child($$, $2);
                  }| error {ERR();$$=NULL;}
OptTag: ID {
  $$ = make_ast_nonterm(OptTag);
  add_ast_child($$, $1);
} | {$$=NULL;} | error {ERR();$$=NULL;}
Tag: ID {
  $$ = make_ast_nonterm(Tag);
  add_ast_child($$, $1);
} | error {ERR();$$=NULL;}
/* Declarators */
VarDec: ID {
          $$ = make_ast_nonterm(VarDec);
          add_ast_child($$, $1);
        } 
        | VarDec LB INT RB {
          $$ = make_ast_nonterm(VarDec);
          add_ast_child($$, $1);
          add_ast_child($$, $2);
          add_ast_child($$, $3);
          add_ast_child($$, $4);
        }| error {ERR();$$=NULL;}
FunDec: ID LP VarList RP {
          $$ = make_ast_nonterm(FunDec);
          add_ast_child($$, $1);
          add_ast_child($$, $2);
          add_ast_child($$, $3);
          add_ast_child($$, $4);
        } 
        | ID LP RP{
          $$ = make_ast_nonterm(FunDec);
          add_ast_child($$, $1);
          add_ast_child($$, $2);
          add_ast_child($$, $3);
        }| error {ERR();$$=NULL;}
VarList: ParamDec COMMA VarList {
          $$ = make_ast_nonterm(VarList);
          add_ast_child($$, $1);
          add_ast_child($$, $2);
          add_ast_child($$, $3);
        }
        | ParamDec {
          $$ = make_ast_nonterm(VarList);
          add_ast_child($$, $1);
        }| error {ERR();$$=NULL;}
ParamDec: Specifier VarDec {
  $$ = make_ast_nonterm(ParamDec);
  add_ast_child($$, $1);
  add_ast_child($$, $2);
}| error {ERR();$$=NULL;}

CompSt: LC DefList StmtList RC {
  $$ = make_ast_nonterm(CompSt);
  add_ast_child($$, $1);
  add_ast_child($$, $2);
  add_ast_child($$, $3);
  add_ast_child($$, $4);
}| error {ERR();$$=NULL;}

StmtList: Stmt StmtList {
          $$ = make_ast_nonterm(StmtList);
          add_ast_child($$, $1);
          add_ast_child($$, $2);
        } | {$$=NULL;}| error {ERR();$$=NULL;}
Stmt: Exp SEMI {
      $$ = make_ast_nonterm(Stmt);
      add_ast_child($$, $1);
      add_ast_child($$, $2);
    }
    | CompSt {
      $$ = make_ast_nonterm(Stmt);
      add_ast_child($$, $1);
    }
    | RETURN Exp SEMI {
      $$ = make_ast_nonterm(Stmt);
      add_ast_child($$, $1);
      add_ast_child($$, $2);
      add_ast_child($$, $3);
    }
    | IF LP Exp RP Stmt %prec LOWER_THAN_ELSE {
      $$ = make_ast_nonterm(Stmt);
      add_ast_child($$, $1);
      add_ast_child($$, $2);
      add_ast_child($$, $3);
      add_ast_child($$, $4);
      add_ast_child($$, $5);
    }
    | IF LP Exp RP Stmt ELSE Stmt {
      $$ = make_ast_nonterm(Stmt);
      add_ast_child($$, $1);
      add_ast_child($$, $2);
      add_ast_child($$, $3);
      add_ast_child($$, $4);
      add_ast_child($$, $5);
      add_ast_child($$, $6);
      add_ast_child($$, $7);
    }
    | WHILE LP Exp RP Stmt{
      $$ = make_ast_nonterm(Stmt);
      add_ast_child($$, $1);
      add_ast_child($$, $2);
      add_ast_child($$, $3);
      add_ast_child($$, $4);
      add_ast_child($$, $5);
    }
    | error {ERR();$$=NULL;}
    /* | error SERR();EMI{
      ERR("sth error but idk fool");
    } */
/* Local Definitions */
DefList: Def DefList {
      $$ = make_ast_nonterm(DefList);
      add_ast_child($$, $1);
      add_ast_child($$, $2);
    }|
    {$$ = NULL;}| error {ERR();$$=NULL;}
Def: Specifier DecList SEMI {
      $$ = make_ast_nonterm(Def);
      add_ast_child($$, $1);
      add_ast_child($$, $2);
      add_ast_child($$, $3);
    }| error {ERR();$$=NULL;}
DecList: Dec {
      $$ = make_ast_nonterm(DecList);
      add_ast_child($$, $1);
    }| Dec COMMA DecList {
      $$ = make_ast_nonterm(DecList);
      add_ast_child($$, $1);
      add_ast_child($$, $2);
      add_ast_child($$, $3);
    }| error {ERR();$$=NULL;}
Dec: VarDec {
      $$ = make_ast_nonterm(Dec);
      add_ast_child($$, $1);
    }| VarDec ASSIGNOP Exp{
      $$ = make_ast_nonterm(Dec);
      add_ast_child($$, $1);
      add_ast_child($$, $2);
      add_ast_child($$, $3);
    }| error {ERR();$$=NULL;}
/* Expressions */
Exp:
    Exp ASSIGNOP Exp {
      $$ = make_ast_nonterm(Exp);
      add_ast_child($$, $1);
      add_ast_child($$, $2);
      add_ast_child($$, $3);
    } |
     Exp AND Exp {
      $$ = make_ast_nonterm(Exp);
      add_ast_child($$, $1);
      add_ast_child($$, $2);
      add_ast_child($$, $3);
    } |
     Exp OR Exp {
      $$ = make_ast_nonterm(Exp);
      add_ast_child($$, $1);
      add_ast_child($$, $2);
      add_ast_child($$, $3);
    } |
     Exp RELOP Exp {
      $$ = make_ast_nonterm(Exp);
      add_ast_child($$, $1);
      add_ast_child($$, $2);
      add_ast_child($$, $3);
    } |
     Exp PLUS Exp {
      $$ = make_ast_nonterm(Exp);
      add_ast_child($$, $1);
      add_ast_child($$, $2);
      add_ast_child($$, $3);
    } |
     Exp MINUS Exp {
      $$ = make_ast_nonterm(Exp);
      add_ast_child($$, $1);
      add_ast_child($$, $2);
      add_ast_child($$, $3);
    } |
     Exp STAR Exp {
      $$ = make_ast_nonterm(Exp);
      add_ast_child($$, $1);
      add_ast_child($$, $2);
      add_ast_child($$, $3);
    } |
     Exp DIV Exp {
      $$ = make_ast_nonterm(Exp);
      add_ast_child($$, $1);
      add_ast_child($$, $2);
      add_ast_child($$, $3);
    } |
     LP Exp RP {
      $$ = make_ast_nonterm(Exp);
      add_ast_child($$, $1);
      add_ast_child($$, $2);
      add_ast_child($$, $3);
    } |
     MINUS Exp %prec UMINUS {
      $$ = make_ast_nonterm(Exp);
      add_ast_child($$, $1);
      add_ast_child($$, $2);
    }|
     NOT Exp {
      $$ = make_ast_nonterm(Exp);
      add_ast_child($$, $1);
      add_ast_child($$, $2);
    }|
     ID LP Args RP {
      $$ = make_ast_nonterm(Exp);
      add_ast_child($$, $1);
      add_ast_child($$, $2);
      add_ast_child($$, $3);
      add_ast_child($$, $4);
    }|
     ID LP RP {
      $$ = make_ast_nonterm(Exp);
      add_ast_child($$, $1);
      add_ast_child($$, $2);
      add_ast_child($$, $3);
    } |
     Exp LB Exp RB {
      $$ = make_ast_nonterm(Exp);
      add_ast_child($$, $1);
      add_ast_child($$, $2);
      add_ast_child($$, $3);
      add_ast_child($$, $4);
    }|
     Exp DOT ID {
      $$ = make_ast_nonterm(Exp);
      add_ast_child($$, $1);
      add_ast_child($$, $2);
      add_ast_child($$, $3);
    } |
     ID {
      $$ = make_ast_nonterm(Exp);
      add_ast_child($$, $1);
    }|
     INT {
      $$ = make_ast_nonterm(Exp);
      add_ast_child($$, $1);
    }|
     FLOAT {
      $$ = make_ast_nonterm(Exp);
      add_ast_child($$, $1);
    }| error {ERR();$$=NULL;}
    /* | Exp error SEMI        {ERR("fo0l!");} */
    /* | Exp ASSIGNOP error    {ERR("fool expression");}
    | Exp AND error         {ERR("fool expression");}
    | Exp OR error          {ERR("fool expression");}
    | Exp RELOP error       {ERR("fool expression");}
    | Exp PLUS error        {ERR("fool expression");}
    | Exp MINUS error       {ERR("fool expression");}
    | Exp STAR error        {ERR("fool expression");}
    | Exp DIV error         {ERR("fool expression");}
    | LP error RP           {ERR("fool expression");}
    | MINUS error           {ERR("fool expression");}
    | NOT error             {ERR("fool expression");}
    | ID LP error RP        {ERR("fool expression");}
    | ID LP error SEMI      {ERR("fool you");}
    | Exp LB error RB       {ERR("fool shit");}
    | Exp LB error SEMI     {ERR("dog shit");} */
Args: Exp COMMA Args {
      $$ = make_ast_nonterm(Args);
      add_ast_child($$, $1);
      add_ast_child($$, $2);
      add_ast_child($$, $3);
    }| Exp {
      $$ = make_ast_nonterm(Args);
      add_ast_child($$, $1);
    }| error {ERR();$$=NULL;}
%%

void yyerror(ast_node* para, char* fmt, ...){
  va_list ap;
  va_start(ap, fmt);
  fprintf(stderr, "Error type B at Line %d: ", para? para->lineno : yylineno);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  va_end(ap);
  parerr = 1;
}