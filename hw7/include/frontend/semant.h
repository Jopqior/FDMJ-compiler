#ifndef SEMANT_H
#define SEMANT_H

#include <stdio.h>
#include <stdlib.h>
#include "env.h"
#include "fdmjast.h"
#include "translate.h"
#include "tigerirp.h"
#include "util.h"

/* structs */

typedef struct expty_* expty;

struct expty_ {
  Tr_exp exp;
  Ty_ty value, location;
};

expty ExpTy(Tr_exp exp, Ty_ty value, Ty_ty location);

/* API */

T_funcDeclList transA_Prog(FILE *out, A_prog p, int arch_size);

/* TODO: semant */
// you can change anything below. definitions below are only for reference:)

/* preprocess */

void transPreprocess(FILE* out, A_classDeclList cdl);

// trans basic class decl info
void transA_ClassDeclList_basic(FILE *out, A_classDeclList cdl);
void transA_ClassDecl_basic(FILE *out, A_classDecl cd);
void transA_ClassVarDeclList_basic(FILE *out, S_table vtbl, A_varDeclList vdl);
void transA_ClassVarDecl_basic(FILE *out, S_table vtbl, A_varDecl vd);
void transA_MethodDeclList_basic(FILE *out, S_table mtbl, S_symbol classid, A_methodDeclList mdl);
void transA_MethodDecl_basic(FILE *out, S_table mtbl, S_symbol classid, A_methodDecl md);
Ty_fieldList transA_FormalList_basic(FILE *out, A_formalList fl);
Ty_field transA_Formal_basic(FILE *out, A_formal f);

// trans extends class decl info
void transA_ClassDeclList_extend(FILE *out, A_classDeclList cdl);
void transA_ClassDecl_extend(FILE *out, A_classDecl cd);

/* semantic analysis */

T_funcDeclList transSemant(FILE *out, A_classDeclList cdl, A_mainMethod m);

// classes
T_funcDeclList transA_ClassDeclList(FILE *out, A_classDeclList cdl);
T_funcDeclList transA_ClassDecl(FILE *out, A_classDecl cd);
void transA_ClassVarDeclList(FILE *out, S_table vtbl, A_varDeclList vdl);
void transA_ClassVarDecl(FILE *out, S_table vtbl, A_varDecl vd);
T_funcDeclList transA_MethodDeclList(FILE *out, S_table mtbl, A_methodDeclList mdl);
T_funcDecl transA_MethodDecl(FILE *out, S_table mtbl, A_methodDecl md);
Temp_tempList transA_FormalList(FILE *out, Ty_fieldList fieldList, A_formalList fl, A_pos pos);
Temp_temp transA_Formal(FILE *out, Ty_field field, A_formal f);
Tr_exp transA_VarDeclList(FILE *out, A_varDeclList vdl);
Tr_exp transA_VarDecl(FILE *out, A_varDecl vd);

// main method
T_funcDecl transA_MainMethod(FILE *out, A_mainMethod main);

// stms
Tr_exp transA_StmList(FILE *out, A_stmList sl);
Tr_exp transA_Stm(FILE *out, A_stm s);
Tr_exp transA_NestedStm(FILE *out, A_stm s);
Tr_exp transA_IfStm(FILE *out, A_stm s);
Tr_exp transA_WhileStm(FILE *out, A_stm s);
Tr_exp transA_AssignStm(FILE *out, A_stm s);
Tr_exp transA_ArrayInit(FILE *out, A_stm s);
Tr_exp transA_CallStm(FILE *out, A_stm s);
Tr_exp transA_Continue(FILE *out, A_stm s);
Tr_exp transA_Break(FILE *out, A_stm s);
Tr_exp transA_Return(FILE *out, A_stm s);
Tr_exp transA_Putnum(FILE *out, A_stm s);
Tr_exp transA_Putch(FILE *out, A_stm s);
Tr_exp transA_Putarray(FILE *out, A_stm s);
Tr_exp transA_Starttime(FILE *out, A_stm s);
Tr_exp transA_Stoptime(FILE *out, A_stm s);

// exps
void transA_ExpList_NumConst_silent(FILE *out, A_expList el);
void transA_Exp_NumConst_silent(FILE *out, A_exp e);
Tr_expList transA_ExpList_NumConst(FILE *out, A_expList el, Ty_ty type);
Tr_exp transA_Exp_NumConst(FILE *out, A_exp e, Ty_ty type);
Tr_expList transA_ExpList_Num(FILE *out, A_expList el, Ty_ty type);
Tr_exp transA_Exp_Num(FILE *out, A_exp e, Ty_ty type);
Tr_expList transA_ExpList_Call(FILE *out, Ty_fieldList fieldList, A_expList el, A_pos pos);
Tr_exp transA_Exp_Call(FILE *out, Ty_field field, A_exp e);
expty transA_Exp(FILE *out, A_exp e, Ty_ty type);
expty transA_OpExp(FILE *out, A_exp e);
expty transA_ArrayExp(FILE *out, A_exp e);
expty transA_CallExp(FILE *out, A_exp e);
expty transA_ClassVarExp(FILE *out, A_exp e);
expty transA_BoolConst(FILE *out, A_exp e);
expty transA_NumConst(FILE *out, A_exp e, Ty_ty type);
expty transA_LengthExp(FILE *out, A_exp e);
expty transA_IdExp(FILE *out, A_exp e);
expty transA_ThisExp(FILE *out, A_exp e);
expty transA_NewIntArrExp(FILE *out, A_exp e);
expty transA_NewFloatArrExp(FILE *out, A_exp e);
expty transA_NewObjExp(FILE *out, A_exp e);
expty transA_NotExp(FILE *out, A_exp e);
expty transA_MinusExp(FILE *out, A_exp e);
expty transA_EscExp(FILE *out, A_exp e);
expty transA_Getnum(FILE *out, A_exp e);
expty transA_Getch(FILE *out, A_exp e);
expty transA_Getarray(FILE *out, A_exp e);

#endif