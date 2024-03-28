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
%token<pos> '(' ')' '[' ']' '{' '}' '=' ',' ';' '.'

// operators
%token<pos> PLUS MINUS TIMES DIV
%token<pos> OR AND NOT
%token<pos> EQ NE LT LE GT GE

// identifiers
%token<exp> ID

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
%type<formalList> FORMAL_LIST FORMAL_REST
%type<varDecl> VAR_DECL
%type<varDeclList> VAR_DECL_LIST
%type<stmList> STM_LIST
%type<stm> STM
%type<exp> EXP CONST
%type<expList> EXP_LIST EXP_REST CONST_LIST CONST_REST

/* start symbol */
%start PROG

/* precedence */
%left OR
%left AND
%left EQ NE
%left LT LE GT GE
%left PLUS MINUS
%left TIMES DIV
%right UMINUS
%right NOT
%left '.' '[' '('
%precedence THEN
%precedence ELSE

%% /* 2. rules */

PROG: MAIN_METHOD CLASS_DECL_LIST {
  root = A_Prog(A_Pos($1->pos->line, $1->pos->pos), $1, $2);
  $$ = root;
};

MAIN_METHOD: PUBLIC INT MAIN '(' ')' '{' VAR_DECL_LIST STM_LIST '}' {
  $$ = A_MainMethod($1, $7, $8);
};

VAR_DECL_LIST: /* empty */ {
  $$ = NULL;
} | VAR_DECL VAR_DECL_LIST {
  if ($1 != NULL) {
    $$ = A_VarDeclList($1, $2);
  } else {
    $$ = $2;
  }
};

VAR_DECL: CLASS ID ID ';' {
  $$ = A_VarDecl($1, A_Type($1, A_idType, $2->u.v), $3->u.v, NULL);
} | INT ID ';' {
  $$ = A_VarDecl($1, A_Type($1, A_intType, NULL), $2->u.v, NULL);
} | INT ID '=' CONST ';' {
  $$ = A_VarDecl($1, A_Type($1, A_intType, NULL), $2->u.v, A_ExpList($4, NULL));
} | INT '[' ']' ID ';' {
  $$ = A_VarDecl($1, A_Type($1, A_intArrType, NULL), $4->u.v, NULL);
} | INT '[' ']' ID '=' '{' CONST_LIST '}' ';' {
  $$ = A_VarDecl($1, A_Type($1, A_intArrType, NULL), $4->u.v, $7);
} | FLOAT ID ';' {
  $$ = A_VarDecl($1, A_Type($1, A_floatType, NULL), $2->u.v, NULL);
} | FLOAT ID '=' CONST ';' {
  $$ = A_VarDecl($1, A_Type($1, A_floatType, NULL), $2->u.v, A_ExpList($4, NULL));
} | FLOAT '[' ']' ID ';' {
  $$ = A_VarDecl($1, A_Type($1, A_floatArrType, NULL), $4->u.v, NULL);
} | FLOAT '[' ']' ID '=' '{' CONST_LIST '}' ';' {
  $$ = A_VarDecl($1, A_Type($1, A_floatArrType, NULL), $4->u.v, $7);
} | INT error ';' {
  yyerrok;
  $$ = NULL;
} | FLOAT error ';' {
  yyerrok;
  $$ = NULL;
} | CLASS error ';' {
  yyerrok;
  $$ = NULL;
};

CONST: NUM {
  $$ = $1;
} | MINUS NUM {
  $$ = A_NumConst($1, -$2->u.num);
};

CONST_LIST: /* empty */ {
  $$ = NULL;
} | CONST CONST_REST {
  $$ = A_ExpList($1, $2);
};

CONST_REST: /* empty */ {
  $$ = NULL;
} | ',' CONST CONST_REST {
  $$ = A_ExpList($2, $3);
};

STM_LIST: /* empty */ {
  $$ = NULL;
} | STM STM_LIST {
  if ($1 != NULL) {
    $$ = A_StmList($1, $2);
  } else {
    $$ = $2;
  }
};

STM: '{' STM_LIST '}' {
  $$ = A_NestedStm($1, $2);
} | IF '(' EXP ')' STM ELSE STM {
  $$ = A_IfStm($1, $3, $5, $7);
} | IF '(' EXP ')' STM %prec THEN {
  $$ = A_IfStm($1, $3, $5, NULL);
} | WHILE '(' EXP ')' STM {
  $$ = A_WhileStm($1, $3, $5);
} | WHILE '(' EXP ')' ';' {
  $$ = A_WhileStm($1, $3, NULL);
} | EXP '=' EXP ';' {
  $$ = A_AssignStm(A_Pos($1->pos->line, $1->pos->pos), $1, $3);
} | EXP '[' ']' '=' '{' EXP_LIST '}' ';' {
  $$ = A_ArrayInit(A_Pos($1->pos->line, $1->pos->pos), $1, $6);
} | EXP '.' ID '(' EXP_LIST ')' ';' {
  $$ = A_CallStm(A_Pos($1->pos->line, $1->pos->pos), $1, $3->u.v, $5);
} | CONTINUE ';' {
  $$ = A_Continue($1);
} | BREAK ';' {
  $$ = A_Break($1);
} | RETURN EXP ';' {
  $$ = A_Return($1, $2);
} | PUTNUM '(' EXP ')' ';' {
  $$ = A_Putnum($1, $3);
} | PUTCH '(' EXP ')' ';' {
  $$ = A_Putch($1, $3);
} | PUTARRAY '(' EXP ',' EXP ')' ';' {
  $$ = A_Putarray($1, $3, $5);
} | STARTTIME '(' ')' ';' {
  $$ = A_Starttime($1);
} | STOPTIME '(' ')' ';' {
  $$ = A_Stoptime($1);
} | error ';' {
  yyerrok;
  $$ = NULL;
} | error '}' {
  yyerrok;
  $$ = NULL;
};

EXP: NUM {
  $$ = $1;
} | TRUE_V {
  $$ = A_BoolConst($1, TRUE);
} | FALSE_V {
  $$ = A_BoolConst($1, FALSE);
} | LENGTH '(' EXP ')' {
  $$ = A_LengthExp($1, $3);
} | GETNUM '(' ')' {
  $$ = A_Getnum($1);
} | GETCH '(' ')'{
  $$ = A_Getch($1);
} | GETARRAY '(' EXP ')' {
  $$ = A_Getarray($1, $3);
} | ID {
  $$ = $1;
} | THIS {
  $$ = A_ThisExp($1);
} | NEW INT '[' EXP ']' {
  $$ = A_NewIntArrExp($1, $4);
} | NEW FLOAT '[' EXP ']' {
  $$ = A_NewFloatArrExp($1, $4);
} | NEW ID '(' ')' {
  $$ = A_NewObjExp($1, $2->u.v);
} | EXP PLUS EXP {
  $$ = A_OpExp(A_Pos($1->pos->line, $1->pos->pos), $1, A_plus, $3);
} | EXP MINUS EXP {
  $$ = A_OpExp(A_Pos($1->pos->line, $1->pos->pos), $1, A_minus, $3);
} | EXP TIMES EXP {
  $$ = A_OpExp(A_Pos($1->pos->line, $1->pos->pos), $1, A_times, $3);
} | EXP DIV EXP {
  $$ = A_OpExp(A_Pos($1->pos->line, $1->pos->pos), $1, A_div, $3);
} | EXP OR EXP {
  $$ = A_OpExp(A_Pos($1->pos->line, $1->pos->pos), $1, A_or, $3);
} | EXP AND EXP {
  $$ = A_OpExp(A_Pos($1->pos->line, $1->pos->pos), $1, A_and, $3);
} | EXP LT EXP {
  $$ = A_OpExp(A_Pos($1->pos->line, $1->pos->pos), $1, A_less, $3);
} | EXP LE EXP {
  $$ = A_OpExp(A_Pos($1->pos->line, $1->pos->pos), $1, A_le, $3);
} | EXP GT EXP {
  $$ = A_OpExp(A_Pos($1->pos->line, $1->pos->pos), $1, A_greater, $3);
} | EXP GE EXP {
  $$ = A_OpExp(A_Pos($1->pos->line, $1->pos->pos), $1, A_ge, $3);
} | EXP EQ EXP {
  $$ = A_OpExp(A_Pos($1->pos->line, $1->pos->pos), $1, A_eq, $3);
} | EXP NE EXP {
  $$ = A_OpExp(A_Pos($1->pos->line, $1->pos->pos), $1, A_ne, $3);
} | NOT EXP {
  $$ = A_NotExp($1, $2);
} | MINUS EXP %prec UMINUS {
  $$ = A_MinusExp($1, $2);
} | '(' EXP ')' {
  $2->pos = $1;
  $$ = $2;
} | '(' '{' STM_LIST '}' EXP ')' {
  $$ = A_EscExp($1, $3, $5);
} | EXP '.' ID {
  $$ = A_ClassVarExp(A_Pos($1->pos->line, $1->pos->pos), $1, $3->u.v);
} | EXP '.' ID '(' EXP_LIST ')' {
  $$ = A_CallExp(A_Pos($1->pos->line, $1->pos->pos), $1, $3->u.v, $5);
} | EXP '[' EXP ']' {
  $$ = A_ArrayExp(A_Pos($1->pos->line, $1->pos->pos), $1, $3);
};

EXP_LIST: /* empty */ {
  $$ = NULL;
} | EXP EXP_REST {
  $$ = A_ExpList($1, $2);
};

EXP_REST: /* empty */ {
  $$ = NULL;
} | ',' EXP EXP_REST {
  $$ = A_ExpList($2, $3);
};

CLASS_DECL_LIST: /* empty */ {
  $$ = NULL;
} | CLASS_DECL CLASS_DECL_LIST {
  if ($1 != NULL) {
    $$ = A_ClassDeclList($1, $2);
  } else {
    $$ = $2;
  }
};

CLASS_DECL: PUBLIC CLASS ID '{' VAR_DECL_LIST METHOD_DECL_LIST '}' {
  $$ = A_ClassDecl($1, $3->u.v, NULL, $5, $6);
} | PUBLIC CLASS ID EXTENDS ID '{' VAR_DECL_LIST METHOD_DECL_LIST '}' {
  $$ = A_ClassDecl($1, $3->u.v, $5->u.v, $7, $8);
} | error '}' {
  yyerrok;
  $$ = NULL;
};

METHOD_DECL_LIST: /* empty */ {
  $$ = NULL;
} | METHOD_DECL METHOD_DECL_LIST {
  if ($1 != NULL) {
    $$ = A_MethodDeclList($1, $2);
  } else {
    $$ = $2;
  }
};

METHOD_DECL: PUBLIC TYPE ID '(' FORMAL_LIST ')' '{' VAR_DECL_LIST STM_LIST '}' {
  $$ = A_MethodDecl($1, $2, $3->u.v, $5, $8, $9);
} | error '}' {
  yyerrok;
  $$ = NULL;
}

TYPE: CLASS ID {
  $$ = A_Type($1, A_idType, $2->u.v);
} | INT {
  $$ = A_Type($1, A_intType, NULL);
} | INT '[' ']' {
  $$ = A_Type($1, A_intArrType, NULL);
} | FLOAT {
  $$ = A_Type($1, A_floatType, NULL);
} | FLOAT '[' ']' {
  $$ = A_Type($1, A_floatArrType, NULL);
};

FORMAL_LIST: /* empty */ {
  $$ = NULL;
} | TYPE ID FORMAL_REST {
  $$ = A_FormalList(A_Formal(A_Pos($1->pos->line, $1->pos->pos), $1, $2->u.v), $3);
};

FORMAL_REST: /* empty */ {
  $$ = NULL;
} | ',' TYPE ID FORMAL_REST {
  $$ = A_FormalList(A_Formal(A_Pos($2->pos->line, $2->pos->pos), $2, $3->u.v), $4);
};

%% /* 3. programs */

void yyerror(char *s) {
  extern int pos, line;
  extern char *yytext;
  extern int yyleng;
  extern char linebuf[500];
  fprintf(stderr, "line %d,%d: %s near %s:\n%s\n", line, pos - yyleng, s, yytext, linebuf);
  fprintf(stderr, "%*s\n", pos - yyleng, "^");
}

int yywrap() {
  return(1);
}