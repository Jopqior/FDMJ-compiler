/* 1. declarations */

/* included code */
%{

#include <stdio.h>
#include "fdmjast.h"
#include "util.h"

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

  // add for id
  string id;
}

/* termianl symbols */
// reserved keywords
%token<pos> PUBLIC MAIN
%token<pos> CLASS EXTENDS
%token<pos> INT FLOAT
%token<pos> IF ELSE
%token<pos> WHILE CONTINUE BREAK
%token<pos> RETURN
%token<pos> THIS NEW

// functions
%token<pos> PUTNUM PUTCH PUTARRAY
%token<pos> STARTTIME STOPTIME
%token<pos> GETNUM GETCH GETARRAY
%token<pos> LENGTH

// boolean values
%token<pos> TRUE_V FALSE_V

// punctuation
%token<pos> '(' ')' '[' ']' '{' '}' '=' ',' ';' '.' '!'

// operators
%token<pos> ADD MINUS TIMES DIV
%token<pos> OR AND NOT
%token<pos> EQ NE LT LE GT GE

// identifiers
%token<id> ID

// num
%token<exp> NUM

/* non-termianl symbols */
%type<type> TYPE
%type<prog> PROG
%type<mainMethod> MAIN_METHOD
%type<classDecl> CLASS_DECL
%type<classDeclList> CLASS_DECL_LIST
%type<methodDecl> METHOD_DECL
%type<methodDeclList> METHOD_DECL_LIST
%type<formal> FORMAL
%type<formalList> FORMAL_LIST FORMAL_REST
%type<varDecl> VAR_DECL
%type<varDeclList> VAR_DECL_LIST
%type<stmList> STM_LIST
%type<stm> STM
%type<exp> EXP CONST
%type<expList> EXP_LIST CONST_LIST CONST_REST

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
  $$ = A_MainMethod($1, $7, $8);
} ;

VAR_DECL_LIST: /* empty */ {
  $$ = NULL;
} | VAR_DECL VAR_DECL_LIST {
  $$ = A_VarDeclList($1, $2);
}

VAR_DECL: CLASS ID ID ';' {
  // TODO
  $$ = A_VarDecl($1, A_Type($1, A_idType, $2), NULL);
} | INT ID ';' {
  
}
%% /* 3. programs */

void yyerror(char *s) {
  fprintf(stderr, "%s\n",s);
}

int yywrap() {
  return(1);
}