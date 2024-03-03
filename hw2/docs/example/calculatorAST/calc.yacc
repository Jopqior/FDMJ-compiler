%{
#include<stdio.h>
#include "calc.h"

int base;

extern int yylex();
extern void yyerror(char*);
extern int  yywrap();

%}

%token <a> DIGIT
%token <a> LETTER

%left '|'
%left '&'
%left '+' '-'
%left '*' '/' '%'
%left UMINUS  /*supplies precedence for unary minus */

%union {A_exp e; A_stm s; int a;}

%type <a> number
%type <s> stat
%type <e> expr

%start list

%%                   /* beginning of rules section */

list:                       /*empty */
         |
        list stat '\n'
        { printf("\n");}
         |
        list error '\n'
         {
           yyerrok;
         }
         ;
stat:    expr
         {
           printExp($1);
         }
         |
         LETTER '=' expr
         {
           $$=A_AssignStm($1, $3);
         }

         ;

expr:    '(' expr ')'
         {
           $$ = $2;
         }
         |
         expr '*' expr
         {

           $$ = A_OpExp($1, A_times, $3);
         }
         |
         expr '/' expr
         {
           $$ = A_OpExp($1, A_div, $3);
         }
         |
         expr '+' expr
         {
           $$ = A_OpExp($1, A_plus, $3);
         }
         |
         expr '-' expr
         {
           $$ = A_OpExp($1, A_minus, $3);
         }
         |
         LETTER
         {
           $$ = A_IdExp($1);
         }

         |
         number
         {
           $$ = A_NumExp($1);
         }
         ;

number:  DIGIT
         {
           $$ = $1;
           base = ($1==0) ? 8 : 10;
         }       |
         number DIGIT
         {
           $$ = base * $1 + $2;
         }
         ;

%%

void yyerror(s)
char *s;
{
  fprintf(stderr, "%s\n",s);
}

int yywrap()
{
  return(1);
}
