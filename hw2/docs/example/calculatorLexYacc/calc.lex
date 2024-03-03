%{

#include <stdio.h>
#include "y.tab.h"
int c;
%}
%%
" "       ;
[a-z]     {
            c = yytext[0];
            yylval.a = c - 'a';
            return(LETTER);
          }
[0-9]     {
            c = yytext[0];
            yylval.a = c - '0';
            return(DIGIT);
          }
[^a-z0-9\b]    {
                 c = yytext[0];
                 return(c);
              }
%%
