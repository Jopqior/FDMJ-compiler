%{

#include <stdio.h>
#include "fdmjast.h"
#include "util.h"

extern int yylex();
extern void yyerror(char*);
extern int yywrap();

// extern int yydebug = 1;

extern A_prog root;

%}

// start symbol
%start PROG

%%                   /* beginning of rules section */

PROG:  //to be replaced!
{
  root = NULL;
} ;

;

%%

int yywrap() {return 1;}

void yyerror(char *s) {
  fprintf(stderr, "%s\n",s);
}
