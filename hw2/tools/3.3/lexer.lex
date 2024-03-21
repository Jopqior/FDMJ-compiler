%{
#include <stdio.h>
#include "parser.h"
%}

%%
"[" {return OPENBRKT;}
"]" {return CLOSEBRKT;}
"(" {return OPENPAREN;}
")" {return CLOSEPAREN;}
. {printf("Illegal input \"%c\"\n", yytext[0]);}
\n {}
%%