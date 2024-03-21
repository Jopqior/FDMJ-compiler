/* 1. declarations */

/* included code */
%{

#include <stdlib.h>
#include "fdmjast.h"
#include "parser.h"

int line = 1;
int pos = 1;
%}

/* start conditions */
%s COMMENT1 COMMENT2

/* regexp nicknames */
punctuation [()\[\]{}=,;.!]
id      [a-z_A-Z][a-z_A-Z0-9]*

%% /* 2. rules */

<INITIAL>"//" {
  pos += 2;
  BEGIN COMMENT1;
}
<INITIAL>"/*" {
  pos += 2;
  BEGIN COMMENT2;
}

<INITIAL>. {return yytext[0];} // this needs to be replaced!

<COMMENT1>\n {
  line += 1;
  pos = 1;
  BEGIN INITIAL;
}
<COMMENT1>. {
  pos += 1;
}

<COMMENT2>"*/" {
  pos += 2;
  BEGIN INITIAL;
}
<COMMENT2>\n {
  line += 1;
  pos = 1;
}
<COMMENT2>. {
  pos += 1;
}

%% /* 3. programs */