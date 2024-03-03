%{
#include <stdio.h>

extern int yylex();
extern void yyerror(char*);
extern int  yywrap();
extern int YYLEX;

%}

%start list
%token ONE TWO EOL

%%                   /* beginning of rules section */

list: 
|
ONE ONE '\n'
         {
           printf("Got it!\n"); 
         }

;

%%
int main()
{
 return(yyparse());
}

void yyerror(s)
char *s;
{
  fprintf(stderr, "%s\n",s);
}

int yywrap()
{
  return(1);
}
