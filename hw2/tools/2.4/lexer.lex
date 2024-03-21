%{
#include <stdio.h>
#include "parser.h"
%}

%%
a((b|a*c)x)*|x*a { return PATTERN; }
. { return yytext[0]; }
\n {}
%%