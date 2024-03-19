%{
#include <stdio.h>
%}

%%
ca(t|r)s? { printf("Accepted\n"); }
.* { printf("Rejected\n"); }
\n {}
%%

int main() {
    yylex();
    return 0;
}
