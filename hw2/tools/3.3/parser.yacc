%{
#include <stdio.h>
extern int yylex();
extern void yyerror(char*);
extern int yywrap();
%}

// terminal symbols
%token OPENBRKT CLOSEBRKT OPENPAREN CLOSEPAREN

// non-terminal symbols
%type S P RP

// start symbol
%start S

%%
S   :   /* empty */
    |   OPENBRKT P CLOSEBRKT S
    |   OPENPAREN S CLOSEPAREN S
    ; 

P   :   RP
    |   OPENPAREN P
    ;

RP  :   S
    |   RP OPENPAREN
    ;
%%

int main() {
    if (yyparse() == 0) {
        printf("Accepted\n");
    } else {
        printf("Rejected\n");
    }
    return 0;
}

void yyerror(char *s) {
  // do nothing
}

int yywrap() {
  return 1;
}
