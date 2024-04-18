#ifndef TRANSLATE_H
#define TRANSLATE_H

#include <stdio.h>
#include "fdmjast.h"
#include "temp.h"
#include "tigerirp.h"
#include "util.h"

/* structs */

typedef struct Tr_exp_ *Tr_exp;
typedef struct Tr_expList_ *Tr_expList;

/* translate */

// methods
T_funcDeclList Tr_FuncDeclList(T_funcDecl fd, T_funcDeclList fdl);
T_funcDeclList Tr_ChainFuncDeclList(T_funcDeclList first, T_funcDeclList second);
T_funcDecl Tr_MainMethod(Tr_exp vdl, Tr_exp sl);
T_funcDecl Tr_ClassMethod(string name, Temp_tempList paras, Tr_exp vdl, Tr_exp sl);

// stms
Tr_exp Tr_StmList(Tr_exp head, Tr_exp tail);
Tr_exp Tr_IfStm(Tr_exp test, Tr_exp then, Tr_exp elsee);
Tr_exp Tr_WhileStm(Tr_exp test, Tr_exp loop, Temp_label whiletest, Temp_label whileend);
Tr_exp Tr_AssignStm(Tr_exp location, Tr_exp value);
Tr_exp Tr_ArrayInit(Tr_exp arr, Tr_expList init, T_type type);
Tr_exp Tr_CallStm(string meth, Tr_exp clazz, Tr_exp thiz, Tr_expList el, T_type type);
Tr_exp Tr_Continue(Temp_label whiletest);
Tr_exp Tr_Break(Temp_label whileend);
Tr_exp Tr_Return(Tr_exp ret);
Tr_exp Tr_Putint(Tr_exp exp);
Tr_exp Tr_Putfloat(Tr_exp exp);
Tr_exp Tr_Putch(Tr_exp exp);
Tr_exp Tr_Putarray(Tr_exp pos, Tr_exp arr);
Tr_exp Tr_Starttime();
Tr_exp Tr_Stoptime();

// exps
Tr_expList Tr_ExpList(Tr_exp head, Tr_expList tail);
Tr_exp Tr_OpExp(A_binop op, Tr_exp left, Tr_exp right);
Tr_exp Tr_ArrayExp(Tr_exp arr, Tr_exp pos, T_type type);
Tr_exp Tr_CallExp(string meth, Tr_exp clazz, Tr_exp thiz, Tr_expList el, T_type type);
Tr_exp Tr_ClassVarExp(Tr_exp clazz, Tr_exp offset, T_type type);
Tr_exp Tr_ClassMethExp(Tr_exp clazz, Tr_exp offset);
Tr_exp Tr_ClassMethLabel(Temp_label label);
Tr_exp Tr_BoolConst(bool b);
Tr_exp Tr_NumConst(float num, T_type type);
Tr_exp Tr_LengthExp(Tr_exp arr);
Tr_exp Tr_IdExp(Temp_temp tmp);
Tr_exp Tr_ThisExp(Temp_temp tmp);
Tr_exp Tr_NewArrExp(Tr_exp size);
Tr_exp Tr_NewObjExp(Tr_exp size);
Tr_exp Tr_NotExp(Tr_exp exp);
Tr_exp Tr_MinusExp(Tr_exp exp);
Tr_exp Tr_EscExp(Tr_exp stm, Tr_exp exp);
Tr_exp Tr_Getint();
Tr_exp Tr_Getfloat();
Tr_exp Tr_Getch();
Tr_exp Tr_Getarray(Tr_exp exp);
Tr_exp Tr_Getfarray(Tr_exp exp);
Tr_exp Tr_Cast(Tr_exp exp, T_type type);

#endif
