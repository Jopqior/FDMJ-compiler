/* 1. declarations */

/* included code */
%{

#include <stdlib.h>
#include "fdmjast.h"
#include "parser.h"

%}

/* start conditions */

/* regexp nicknames */

%% /* 2. rules */

. {return yytext[0];} // this needs to be replaced!

%% /* 3. programs */