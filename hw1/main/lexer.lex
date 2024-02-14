%{

#include <stdlib.h>
#include "fdmjslpast.h"
#include "util.h"
#include "y.tab.h"

int c;
int line = 1;
int pos = 1;

%}

%s COMMENT1 COMMENT2

punctuation [()\[\]{}=,;.!]
integer   [1-9][0-9]*|0
id      [a-z_A-Z][a-z_A-Z0-9]*

%%

<INITIAL>"//" {
  pos += 2;
  BEGIN COMMENT1;
}
<INITIAL>"/*" {
  pos += 2;
  BEGIN COMMENT2;
}
<INITIAL>" "|\t {
  pos += 1;
}
<INITIAL>\n {
  line += 1;
  pos = 1;
}
<INITIAL>\r {}
<INITIAL>"public" {
  yylval.pos = A_Pos(line, pos);
  pos += yyleng;
  return PUBLIC;
}
<INITIAL>"int" {
  yylval.pos = A_Pos(line, pos);
  pos += yyleng;
  return INT;
}
<INITIAL>"main" {
  yylval.pos = A_Pos(line, pos);
  pos += yyleng;
  return MAIN;
}
<INITIAL>"putint" {
  yylval.pos = A_Pos(line, pos);
  pos += yyleng;
  return PUTINT;
}
<INITIAL>"putch" {
  yylval.pos = A_Pos(line, pos);
  pos += yyleng;
  return PUTCH;
}
<INITIAL>"+" {
  yylval.pos = A_Pos(line, pos);
  pos += yyleng;
  return ADD;
}
<INITIAL>"-" {
  yylval.pos = A_Pos(line, pos);
  pos += yyleng;
  return MINUS;
}
<INITIAL>"*" {
  yylval.pos = A_Pos(line, pos);
  pos += yyleng;
  return TIMES;
}
<INITIAL>"/" {
  yylval.pos = A_Pos(line, pos);
  pos += yyleng;
  return DIV;
}
<INITIAL>{punctuation} {
  yylval.pos = A_Pos(line, pos);
  pos += 1;
  c = yytext[0];
  return c;
}
<INITIAL>{integer} {
  yylval.exp = A_NumConst(A_Pos(line, pos), atoi(yytext));
  pos += yyleng;
  return NUM;
}
<INITIAL>{id} {
  yylval.exp = A_IdExp(A_Pos(line, pos), String(yytext));
  pos += yyleng;
  return ID;
}
<INITIAL>. {
  printf("Illegal input \"%c\"\n", yytext[0]);
}

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

%%
