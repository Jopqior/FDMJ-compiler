/* 1. declarations */

/* included code */
%{

#include <stdio.h>

extern int yylex();
extern void yyerror(char*);
extern int  yywrap();

int regs[26];

%}

/* yylval */
%union {
  int num;
}

/* termianl symbols */
%token<num> LETTER
%token<num> DIGIT

/* non-termianl symbols */
%type<num> STMLIST STM EXPR

/* start symbol */
%start STMLIST

/* precedence */
%left ADD MINUS
%left TIMES DIV
%right UMINUS
%left '(' ')'

%% /* 2. rules */

STMLIST: /* empty */ {
} | STM STMLIST {
};
STM: '\n' {
} | EXPR '\n' {
  printf("= %d\n", $1);
} | LETTER '=' EXPR '\n' {
  regs[$1] = $3;
} ;
EXPR: '(' EXPR ')' {
  $$ = $2;
} | LETTER {
  $$ = regs[$1];
} | DIGIT {
  $$ = $1;
} | EXPR ADD EXPR {
  $$ = $1 + $3;
} | EXPR MINUS EXPR {
  $$ = $1 - $3;
} | EXPR TIMES EXPR {
  $$ = $1 * $3;
} | EXPR DIV EXPR {
  $$ = $1 / $3;
} | MINUS EXPR %prec UMINUS {
  $$ = -$2;
} ;

%% /* 3. programs */

void yyerror(char *s) {
  fprintf(stderr, "%s\n",s);
}

int yywrap() {
  return(1);
}
