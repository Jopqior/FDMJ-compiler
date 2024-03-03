%{
#include <stdio.h>
extern int yylex();
extern void yyerror(char*);
extern int  yywrap();

%}
%start stmlist
%%

stmlist: | stmlist stm 

brlist: '[' ']' | brlist '[' ']' 

stm: 'x' brlist 'y' ';' {printf("Decl\n");}
| 'x' brlist '=' 'y' ';' {  printf("Stm\n"); }
%%

int main() { return(yyparse()); }
void yyerror(char*s) {;}
int yywrap() {return 1;}
