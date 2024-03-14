%{
#include <stdio.h>
%}

%s CAND_ACCEPT MY_REJECT

%%
<INITIAL>a((b|a*c)x)*|x*a { BEGIN CAND_ACCEPT; }
<INITIAL>. { BEGIN MY_REJECT; }
<CAND_ACCEPT>. { BEGIN MY_REJECT; }
<CAND_ACCEPT>\n { printf("Accept\n"); BEGIN INITIAL; }
<MY_REJECT>. {}
<MY_REJECT>\n { printf("Reject\n"); BEGIN INITIAL; }
%%

int main() {
    yylex();
    return 0;
}
