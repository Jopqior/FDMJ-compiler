%{

int flag;

%}

/* lex definitions */

%%

a {printf("1"); }

^ba {printf("22"); }

(^"--"([a-z]*))|(" "|"\t")+   {;}

"\n" {printf("\n");}

. {printf("X");}


%%

int yywrap() {return 1;}

int main(void)
           {
               yyin = stdin;
               yylex();
	       return 0;
           }

