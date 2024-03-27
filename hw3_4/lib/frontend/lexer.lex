/* 1. declarations */

/* included code */
%{

#include <stdlib.h>
#include <string.h>
#include "fdmjast.h"
#include "parser.h"

int c;
int line = 1;
int pos = 1;

// for error reporting
char linebuf[500];
%}

/* start conditions */
%s COMMENT1 COMMENT2

/* regexp nicknames */
punctuation   [()\[\]{}=,;.]
id            [a-z_A-Z][a-z_A-Z0-9]*
integer       [1-9][0-9]*|0
float         [1-9][0-9]*\.[0-9]*|0\.[0-9]*|[1-9][0-9]*\.|0\.|\.[0-9]*

%% /* 2. rules */

<INITIAL>"//" {
  pos += 2;
  BEGIN COMMENT1;
}
<INITIAL>"/*" {
  pos += 2;
  BEGIN COMMENT2;
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


<INITIAL>" "|\t {
  pos += 1;
}
<INITIAL>\n.* {
  line += 1;
  pos = 1;
  strcpy(linebuf, yytext + 1);
  yyless(1);
}
<INITIAL>\r {}


<INITIAL>{punctuation} {
  yylval.pos = A_Pos(line, pos);
  pos += 1;
  c = yytext[0];
  return c;
}


<INITIAL>{integer} {
  yylval.exp = A_NumConst(A_Pos(line, pos), (float)atoi(yytext));
  pos += yyleng;
  return NUM;
}
<INITIAL>{float} {
  yylval.exp = A_NumConst(A_Pos(line, pos), (float)atof(yytext));
  pos += yyleng;
  return NUM;
}


<INITIAL>"true" {
  yylval.pos = A_Pos(line, pos);
  pos += yyleng;
  return TRUE_V;
}
<INITIAL>"false" {
  yylval.pos = A_Pos(line, pos);
  pos += yyleng;
  return FALSE_V;
}


<INITIAL>"public" {
  yylval.pos = A_Pos(line, pos);
  pos += yyleng;
  return PUBLIC;
}
<INITIAL>"main" {
  yylval.pos = A_Pos(line, pos);
  pos += yyleng;
  return MAIN;
}
<INITIAL>"class" {
  yylval.pos = A_Pos(line, pos);
  pos += yyleng;
  return CLASS;
}
<INITIAL>"extends" {
  yylval.pos = A_Pos(line, pos);
  pos += yyleng;
  return EXTENDS;
}
<INITIAL>"int" {
  yylval.pos = A_Pos(line, pos);
  pos += yyleng;
  return INT;
}
<INITIAL>"float" {
  yylval.pos = A_Pos(line, pos);
  pos += yyleng;
  return FLOAT;
}
<INITIAL>"if" {
  yylval.pos = A_Pos(line, pos);
  pos += yyleng;
  return IF;
}
<INITIAL>"else" {
  yylval.pos = A_Pos(line, pos);
  pos += yyleng;
  return ELSE;
}
<INITIAL>"while" {
  yylval.pos = A_Pos(line, pos);
  pos += yyleng;
  return WHILE;
}
<INITIAL>"continue" {
  yylval.pos = A_Pos(line, pos);
  pos += yyleng;
  return CONTINUE;
}
<INITIAL>"break" {
  yylval.pos = A_Pos(line, pos);
  pos += yyleng;
  return BREAK;
}
<INITIAL>"return" {
  yylval.pos = A_Pos(line, pos);
  pos += yyleng;
  return RETURN;
}
<INITIAL>"this" {
  yylval.pos = A_Pos(line, pos);
  pos += yyleng;
  return THIS;
}
<INITIAL>"new" {
  yylval.pos = A_Pos(line, pos);
  pos += yyleng;
  return NEW;
}
<INITIAL>"putnum" {
  yylval.pos = A_Pos(line, pos);
  pos += yyleng;
  return PUTNUM;
}
<INITIAL>"putch" {
  yylval.pos = A_Pos(line, pos);
  pos += yyleng;
  return PUTCH;
}
<INITIAL>"putarray" {
  yylval.pos = A_Pos(line, pos);
  pos += yyleng;
  return PUTARRAY;
}
<INITIAL>"starttime" {
  yylval.pos = A_Pos(line, pos);
  pos += yyleng;
  return STARTTIME;
}
<INITIAL>"stoptime" {
  yylval.pos = A_Pos(line, pos);
  pos += yyleng;
  return STOPTIME;
}
<INITIAL>"getnum" {
  yylval.pos = A_Pos(line, pos);
  pos += yyleng;
  return GETNUM;
}
<INITIAL>"getch" {
  yylval.pos = A_Pos(line, pos);
  pos += yyleng;
  return GETCH;
}
<INITIAL>"getarray" {
  yylval.pos = A_Pos(line, pos);
  pos += yyleng;
  return GETARRAY;
}
<INITIAL>"length" {
  yylval.pos = A_Pos(line, pos);
  pos += yyleng;
  return LENGTH;
}


<INITIAL>"+" {
  yylval.pos = A_Pos(line, pos);
  pos += yyleng;
  return PLUS;
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
<INITIAL>"||" {
  yylval.pos = A_Pos(line, pos);
  pos += yyleng;
  return OR;
}
<INITIAL>"&&" {
  yylval.pos = A_Pos(line, pos);
  pos += yyleng;
  return AND;
}
<INITIAL>"!" {
  yylval.pos = A_Pos(line, pos);
  pos += yyleng;
  return NOT;
}
<INITIAL>"==" {
  yylval.pos = A_Pos(line, pos);
  pos += yyleng;
  return EQ;
}
<INITIAL>"!=" {
  yylval.pos = A_Pos(line, pos);
  pos += yyleng;
  return NE;
}
<INITIAL>"<" {
  yylval.pos = A_Pos(line, pos);
  pos += yyleng;
  return LT;
}
<INITIAL>"<=" {
  yylval.pos = A_Pos(line, pos);
  pos += yyleng;
  return LE;
}
<INITIAL>">" {
  yylval.pos = A_Pos(line, pos);
  pos += yyleng;
  return GT;
}
<INITIAL>">=" {
  yylval.pos = A_Pos(line, pos);
  pos += yyleng;
  return GE;
}

<INITIAL>{id} {
  yylval.exp = A_IdExp(A_Pos(line, pos), String(yytext));
  pos += yyleng;
  return ID;
}

<INITIAL>. {
  printf("Illegal input \"%c\" at line %d, position %d\n", yytext[0], line, pos);
  pos++;
}

%% /* 3. programs */