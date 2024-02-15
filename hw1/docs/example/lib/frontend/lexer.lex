/* 1. declarations */

/* included code */
%{

#include <stdio.h>
#include <stdlib.h>

#include "parser.h"

int c;

%}

/* start conditions */
%s COMMENT

/* regexp nicknames */
punctuation [()=]
digit [1-9][0-9]*|0
letter [a-z]

%% /* 2. rules */

<INITIAL>"//" {
  printf("Omit Comments...\n");
  BEGIN COMMENT;
}
<INITIAL>" "|\t|\r {}
<INITIAL>\n {
  return('\n');
}
<INITIAL>"+" {
  return ADD;
}
<INITIAL>"-" {
  return MINUS;
}
<INITIAL>"*" {
  return TIMES;
}
<INITIAL>"/" {
  return DIV;
}
<INITIAL>{punctuation} {
  return yytext[0];
}
<INITIAL>{digit} {
  yylval.num = atoi(yytext);
  return(DIGIT);
}
<INITIAL>{letter} {
  c = yytext[0];
  yylval.num = c - 'a';
  return(LETTER);
}
<INITIAL>. {
  printf("Illegal input \"%c\"\n", yytext[0]);
}

<COMMENT>\n {
  BEGIN INITIAL;
  return('\n');
}
<COMMENT>. {}

%% /* 3. programs */
