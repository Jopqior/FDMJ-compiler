%{

#include <stdlib.h>
#include "fdmjast.h"
#include "util.h"
#include "y.tab.h"

%}

%%

. {return yytext[0];} //this needs to be replaced!

%%
