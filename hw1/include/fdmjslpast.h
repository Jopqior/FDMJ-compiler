//This is an AST definition for FDMJ2024 SLP
//Filename: fdmjslpast.h

#ifndef __SLPAST
#define __SLPAST
#include "util.h"

typedef struct A_pos_* A_pos; //position information
typedef struct A_prog_* A_prog;
typedef struct A_mainMethod* A_mainMethod;
typedef struct A_stmList_* A_stmList;
typedef struct A_stm_* A_stm;
typedef struct A_exp_* A_exp;

struct A_pos_ {
    int line, pos;
};

struct A_prog_ {
    A_pos pos;
    A_mainMethod m;
};

struct A_mainMethod {
    A_pos pos;
    A_stmList sl;
};

struct A_stmList_ {
    A_stm head;
    A_stmList tail;
};

typedef enum {
    A_assignStm,
    A_putint,
    A_putch,
} A_stmKind;

struct A_stm_ {
    A_pos pos;
    A_stmKind kind;
    union {
        struct {
            A_exp id; //left must be an A_idExp
            A_exp value; //right can be any expression
        } assign; //for array assignment
        A_exp e; //for putint and putch
    } u;
};

typedef enum {
    A_plus,
    A_minus,
    A_times,
    A_div
} A_binop;

typedef enum {
    A_opExp,
    A_escExp,
    A_numConst,
    A_idExp,
    A_minusExp,
} A_expKind;
    
struct A_exp_ {
    A_pos pos;
    A_expKind kind;
    union {
        struct {
                A_exp left;
                A_binop oper;
                A_exp right;
        } op;
        struct {
                A_stmList ns;
                A_exp exp;
        } escExp;
        int num;
        string v;
        A_exp e;
    } u;
};

A_pos A_Pos(int,int);
A_prog A_Prog(A_pos, A_mainMethod);
A_mainMethod A_MainMethod(A_pos, A_stmList); 

A_stmList A_StmList(A_stm, A_stmList);

A_stm A_AssignStm(A_pos, A_exp, A_exp); 
A_stm A_Putint(A_pos, A_exp);
A_stm A_Putch(A_pos, A_exp);

A_exp A_OpExp(A_pos, A_exp, A_binop, A_exp);
A_exp A_NumConst(A_pos, int);
A_exp A_IdExp(A_pos, string);
A_exp A_MinusExp(A_pos, A_exp);
A_exp A_EscExp(A_pos, A_stmList, A_exp);

#endif
