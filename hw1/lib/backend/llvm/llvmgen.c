#include "llvmgen.h"
#include "strtable.h"
#include <stdlib.h>

typedef enum {
  A_num,
  A_id,
} A_expResultKind;

struct A_expResult_ {
  A_expResultKind kind;
  union {
    int num;
    int idNum;
  } u;
};

typedef struct A_expResult_* A_expResult;

void handleStmList(FILE *out, A_stmList sl, STRTAB_table table);
void handleStm(FILE *out, A_stm s, STRTAB_table table);
A_expResult handleExp(FILE *out, A_exp e, STRTAB_table table, string id);

void AS_generateLLVMcode(FILE *out, A_prog p) {
  // TODO
  // ASSUME THE AST IS ALWAYS CORRECT.
  A_mainMethod m = p->m;
  A_stmList slHead = m->sl;

  STRTAB_table table = STRTAB_empty();
  STRTAB_enter(table, "LLVMIR_num", 1);

  handleStmList(out, slHead, table);
}

A_expResult A_Num(int num) {
  A_expResult r = checked_malloc(sizeof(*r));
  r->kind = A_num;
  r->u.num = num;
  return r;
}

A_expResult A_IdNum(int idNum) {
  A_expResult r = checked_malloc(sizeof(*r));
  r->kind = A_id;
  r->u.idNum = idNum;
  return r;
}

string A_expResultToString(A_expResult r) {
  switch (r->kind) {
    string res;
    case A_num:
      res = checked_malloc(10);
      sprintf(res, "%d", r->u.num);
      return res;
    case A_id:
      res = checked_malloc(11);
      sprintf(res, "%%%d", r->u.idNum);
      return res;
  }
}

int acquireNewVar(STRTAB_table table) {
  int oldnum = STRTAB_look(table, "LLVMIR_num");
  STRTAB_enter(table, "LLVMIR_num", oldnum + 1);
  return oldnum;
}

int createTempVar(STRTAB_table table) {
  int idNum = acquireNewVar(table);
  string id = checked_malloc(15);
  sprintf(id, "temp_%d", idNum);
  STRTAB_enter(table, id, idNum);
  return idNum;
}

void handleStmList(FILE *out, A_stmList sl, STRTAB_table table) {
  if (sl == NULL) {
    return;
  }
  handleStm(out, sl->head, table);
  handleStmList(out, sl->tail, table);
}

void handleStm(FILE *out, A_stm s, STRTAB_table table) {
  if (s->kind == A_assignStm) {
    string id = s->u.assign.id->u.v;
    A_expResult rightRes = handleExp(out, s->u.assign.value, table, id);
    if (rightRes->kind == A_num) {
      int idNum = acquireNewVar(table);
      STRTAB_enter(table, id, idNum);
      fprintf(out, "%%%d = add i64 0, %d\n", idNum, rightRes->u.num);
    } else if (rightRes->kind == A_id) {
      STRTAB_enter(table, id, rightRes->u.idNum);
    }
  } else if (s->kind == A_putint) {
    A_expResult res = handleExp(out, s->u.e, table, NULL);
    fprintf(out, "call void @putint(i64 %s)\ncall void @putch(i64 10)\n", A_expResultToString(res));
  } else if (s->kind == A_putch) {
    A_expResult res = handleExp(out, s->u.e, table, NULL);
    fprintf(out, "call void @putch(i64 %s)\n", A_expResultToString(res));
  }
}

A_expResult handleExp(FILE *out, A_exp e, STRTAB_table table, string id) {
  if (e->kind == A_opExp) {
    A_expResult leftRes = handleExp(out, e->u.op.left, table, NULL);
    A_expResult rightRes = handleExp(out, e->u.op.right, table, NULL);
    string op;
    switch (e->u.op.oper) {
      case A_plus:
        op = "add";
        break;
      case A_minus:
        op = "sub";
        break;
      case A_times:
        op = "mul";
        break;
      case A_div:
        op = "sdiv";
        break;
    }
    int idNum = id == NULL? createTempVar(table) : acquireNewVar(table);  
    fprintf(out, "%%%d = %s i64 %s, %s\n", idNum, op, A_expResultToString(leftRes), A_expResultToString(rightRes));
    return A_IdNum(idNum);
  } else if (e->kind == A_escExp) {
    handleStmList(out, e->u.escExp.ns, table);
    return handleExp(out, e->u.escExp.exp, table, id);
  } else if (e->kind == A_numConst) {
    return A_Num(e->u.num);
  } else if (e->kind == A_idExp) {
    int idNum = STRTAB_look(table, e->u.v);
    if (idNum == -1) {
      fprintf(stdout, "Error: id %s not found in table\n", e->u.v);
      exit(1);
    }
    return A_IdNum(idNum);
  } else if (e->kind == A_minusExp) {
    A_expResult res = handleExp(out, e->u.e, table, NULL);
    int idNum = createTempVar(table);
    fprintf(out, "%%%d = sub i64 0, %s\n", idNum, A_expResultToString(res));
    return A_IdNum(idNum);
  }
}