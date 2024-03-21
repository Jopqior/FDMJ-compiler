%{
#include <stdio.h>
#include "parser.h"
%}

%%
ca(t|r)s? { return PATTERN; }
. { return yytext[0]; }
\n {}
%%