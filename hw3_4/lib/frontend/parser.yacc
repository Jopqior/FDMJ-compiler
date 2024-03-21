/* 1. declarations */

/* included code */
%{

#include <stdio.h>
#include "fdmjast.h"

extern int yylex();
extern void yyerror(char*);
extern int  yywrap();

extern A_prog root;

%}

/* yylval */
%union {
  A_pos pos;
  A_type type;
  A_prog prog;
  A_mainMethod mainMethod;
  A_classDecl classDecl;
  A_classDeclList classDeclList;
  A_methodDecl methodDecl;
  A_methodDeclList methodDeclList;
  A_formal formal;
  A_formalList formalList;
  A_varDecl varDecl;
  A_varDeclList varDeclList;
  A_stmList stmList;
  A_stm stm;
  A_exp exp;
  A_expList expList;
}

/* termianl symbols */
// reserved keywords
%token<pos> PUBLIC INT MAIN

// punctuation
%token<pos> '(' ')' '[' ']' '{' '}' '=' ',' ';' '.' '!'

// operators
%token<pos> ADD MINUS TIMES DIV

/* non-termianl symbols */
%type<type> TYPE
%type<prog> PROG
%type<mainMethod> MAIN_METHOD
%type<classDecl> CLASS_DECL
%type<classDeclList> CLASS_DECL_LIST
%type<methodDecl> METHOD_DECL
%type<methodDeclList> METHOD_DECL_LIST
%type<formal> FORMAL
%type<formalList> FORMAL_LIST
%type<varDecl> VAR_DECL
%type<varDeclList> VAR_DECL_LIST
%type<stmList> STM_LIST
%type<stm> STM
%type<exp> EXP
%type<expList> EXP_LIST

/* start symbol */
%start PROG

/* precedence */
%left ADD MINUS
%left TIMES DIV
%right UMINUS

%% /* 2. rules */

PROG: MAIN_METHOD CLASS_DECL_LIST { //to be replaced! 
  root = A_Prog(A_Pos($1->pos->line, $1->pos->pos), $1, $2);
  $$ = root;
} ;

MAIN_METHOD: PUBLIC INT MAIN '(' ')' '{' VAR_DECL_LIST STM_LIST '}' {
  $$ = A_MainMethod(A_Pos($1->line, $1->pos), $7, $8);
} ;

%% /* 3. programs */

void yyerror(char *s) {
  fprintf(stderr, "%s\n",s);
}

int yywrap() {
  return(1);
}