%{
#include <stdio.h>
%}

%%
a((b|a*c)x)*|x*a { printf("Accepted\n"); }
.* { printf("Rejected\n"); }
\n {}
%%

int main() {
    yylex();
    return 0;
}
