#ifndef SEMANT_H
#define SEMANT_H

#include <stdio.h>
#include "env.h"
#include "fdmjast.h"
#include "types.h"
#include "util.h"

/* structs */

typedef struct expty_* expty;
struct expty_ {
  bool location;
  Ty_ty ty;
};
expty Expty(bool location, Ty_ty ty);

/* API */

void transA_Prog(FILE *out, A_prog p);

/* preprocess */

void transPreprocess(FILE* out, A_classDeclList cdl);

// trans basic class decl info
void transA_ClassDeclList_basic(FILE* out, A_classDeclList cdl);
void transA_ClassDecl_basic(FILE* out, A_classDecl cd);
void transA_ClassVarDeclList_basic(FILE* out, S_table vtbl, A_varDeclList vdl);
void transA_ClassVarDecl_basic(FILE* out, S_table vtbl, A_varDecl vd);
void transA_MethodDeclList_basic(FILE* out, S_table mtbl, A_methodDeclList mdl);
void transA_MethodDecl_basic(FILE* out, S_table mtbl, A_methodDecl md);
Ty_fieldList transA_FormalList_basic(FILE* out, A_formalList fl);
Ty_field transA_Formal_basic(FILE* out, A_formal f);

// trans extends class decl info
void transA_ClassDeclList_extend(FILE* out, A_classDeclList cdl);
void transA_ClassDecl_extend(FILE* out, A_classDecl cd);
void transA_ClassVtbl_copy(FILE* out, S_table fa, S_table cur);
void transA_ClassMtbl_copy(FILE* out, S_table fa, S_table cur);

/* semantic analysis */

void transSemant(FILE* out, A_classDeclList cdl, A_mainMethod m);

// classes
void transA_ClassDeclList(FILE* out, A_classDeclList cdl);
void transA_ClassDecl(FILE* out, A_classDecl cd);
void transA_ClassVarDeclList(FILE* out, A_varDeclList vdl);
void transA_ClassVarDecl(FILE* out, A_varDecl vd);
void transA_MethodDeclList(FILE* out, A_methodDeclList mdl);
void transA_MethodDecl(FILE* out, A_methodDecl md);
void transA_FormalList(FILE* out, A_formalList fl);
void transA_Formal(FILE* out, A_formal f);

void transA_VarDeclList(FILE* out, A_varDeclList vdl);
void transA_VarDecl(FILE* out, A_varDecl vd);

// main method
void transA_MainMethod(FILE* out, A_mainMethod m);

// stms
void transA_StmList(FILE* out, A_stmList sl);
void transA_Stm(FILE* out, A_stm s);
void transA_NestedStm(FILE* out, A_stm s);
void transA_IfStm(FILE* out, A_stm s);
void transA_WhileStm(FILE* out, A_stm s);
void transA_AssignStm(FILE* out, A_stm s);
void transA_ArrayInit(FILE* out, A_stm s);
void transA_CallStm(FILE* out, A_stm s);
void transA_Continue(FILE* out, A_stm s);
void transA_Break(FILE* out, A_stm s);
void transA_Return(FILE* out, A_stm s);
void transA_Putnum(FILE* out, A_stm s);
void transA_Putch(FILE* out, A_stm s);
void transA_Putarray(FILE* out, A_stm s);
void transA_Starttime(FILE* out, A_stm s);
void transA_Stoptime(FILE* out, A_stm s);

// exps
void transA_ArrayInitExpList(FILE* out, A_expList el);
void transA_ExpList_Call(FILE* out, Ty_fieldList fl, A_expList el, A_pos pos);
void transA_Exp_Call(FILE* out, Ty_field f, A_exp e);
expty transA_Exp(FILE* out, A_exp e);
expty transA_OpExp(FILE* out, A_exp e);
expty transA_ArrayExp(FILE* out, A_exp e);
expty transA_CallExp(FILE* out, A_exp e);
expty transA_ClassVarExp(FILE* out, A_exp e);
expty transA_BoolConst(FILE* out, A_exp e);
expty transA_NumConst(FILE* out, A_exp e);
expty transA_IdExp(FILE* out, A_exp e);
expty transA_ThisExp(FILE* out, A_exp e);
expty transA_LengthExp(FILE* out, A_exp e);
expty transA_NewIntArrExp(FILE* out, A_exp e);
expty transA_NewFloatArrExp(FILE* out, A_exp e);
expty transA_NewObjExp(FILE* out, A_exp e);
expty transA_NotExp(FILE* out, A_exp e);
expty transA_MinusExp(FILE* out, A_exp e);
expty transA_EscExp(FILE* out, A_exp e);
expty transA_Getnum(FILE* out, A_exp e);
expty transA_Getch(FILE* out, A_exp e);
expty transA_Getarray(FILE* out, A_exp e);

/* helper functions */

bool equalClassMethSignature(E_enventry fa, E_enventry cur);
bool equalTy(Ty_ty fa, Ty_ty cur);
bool equalTyCast(Ty_ty param, Ty_ty arg);
bool isParentClass(Ty_ty left, Ty_ty right);
Ty_ty atype2tyty(A_type t);
Ty_field f2tyf(A_formal f);
string ty2str(Ty_ty t);
A_varDecl f2vd(A_formal f);
string Stringf(char *, ...);

#endif