%{

#include <stdio.h>
#include "fdmjslpast.h"
#include "util.h"

extern int yylex();
extern void yyerror(char*);
extern int  yywrap();

// extern int yydebug = 1;

extern A_prog root;

%}

// yylval
%union {
  A_pos pos;
  A_prog prog;
  A_mainMethod mainMethod;
  A_stmList stmList;
  A_stm stm;
  A_exp exp;
}

// termianl symbols
%token<pos> PUBLIC INT MAIN PUTINT PUTCH
%token<pos> ADD MINUS TIMES DIV
%token<pos> '(' ')' '[' ']' '{' '}' '=' ',' ';' '.' '!'
%token<exp> ID NUM

// non-termianl symbols
%type<prog> PROG
%type<mainMethod> MAINMETHOD
%type<stmList> STMLIST
%type<stm> STM
%type<exp> EXP INTCONST

// start symbol
%start PROG

// precedence
%left ADD MINUS
%left TIMES DIV
%right UMINUS

%%
PROG: MAINMETHOD {
  root = A_Prog(A_Pos($1->pos->line, $1->pos->pos), $1);
  $$ = root;
} ;

MAINMETHOD: PUBLIC INT MAIN '(' ')' '{' STMLIST '}' {
  $$ = A_MainMethod($1, $7);
} ;
  
INTCONST: NUM {
  $$ = $1;
} ;

STMLIST: /* empty */ {
  $$ = NULL;
} | STM STMLIST {
  $$ = A_StmList($1, $2);
} ;

STM: EXP '=' EXP ';' {
  $$ = A_AssignStm(A_Pos($1->pos->line, $1->pos->pos), $1, $3);
} | PUTINT '(' EXP ')' ';' {
  $$ = A_Putint($1, $3);
} | PUTCH '(' EXP ')' ';' {
  $$ = A_Putch($1, $3);
} ;

EXP: EXP ADD EXP {
  $$ = A_OpExp(A_Pos($1->pos->line, $1->pos->pos), $1, A_plus, $3);
} | EXP MINUS EXP {
  $$ = A_OpExp(A_Pos($1->pos->line, $1->pos->pos), $1, A_minus, $3);
} | EXP TIMES EXP {
  $$ = A_OpExp(A_Pos($1->pos->line, $1->pos->pos), $1, A_times, $3);
} | EXP DIV EXP {
  $$ = A_OpExp(A_Pos($1->pos->line, $1->pos->pos), $1, A_div, $3);
} | MINUS EXP %prec UMINUS {
  $$ = A_MinusExp($1, $2);
} | '(' EXP ')' {
  $2->pos = $1;
  $$ = $2;
} | '(' '{' STMLIST '}' EXP ')' {
  $$ = A_EscExp($1, $3, $5);
} |  INTCONST {
  $$ = $1;
} | ID {
  $$=$1;
} ;

%%

void yyerror(char *s) {
  fprintf(stderr, "%s\n",s);
}

int yywrap() {
  return(1);
}
