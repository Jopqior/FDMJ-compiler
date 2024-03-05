/* 1. declarations */

/* included code */
%{

#include <stdio.h>
#include "fdmjast.h"

extern int yylex();
extern void yyerror(char*);
extern int  yywrap();

extern A_prog root;

%}

/* yylval */

/* termianl symbols */

/* non-termianl symbols */

/* start symbol */
%start PROG

/* precedence */

%% /* 2. rules */

PROG: /* empty */ { //to be replaced! 
  root = NULL;
} ;

%% /* 3. programs */

void yyerror(char *s) {
  fprintf(stderr, "%s\n",s);
}

int yywrap() {
  return(1);
}