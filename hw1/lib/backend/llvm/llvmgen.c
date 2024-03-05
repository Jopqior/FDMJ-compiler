#include "llvmgen.h"
#include "strtable.h"
#include <stdlib.h>

typedef enum {
  AS_num,
  AS_id,
} AS_expResultKind;

struct AS_expResult_ {
  AS_expResultKind kind;
  union {
    int num;
    int idNum;
  } u;
};

typedef struct AS_expResult_* AS_expResult;

// AS_expResult constructors

AS_expResult AS_Num(int num) {
  AS_expResult r = checked_malloc(sizeof(*r));
  r->kind = AS_num;
  r->u.num = num;
  return r;
}

AS_expResult AS_IdNum(int idNum) {
  AS_expResult r = checked_malloc(sizeof(*r));
  r->kind = AS_id;
  r->u.idNum = idNum;
  return r;
}

// AS_expResult helper functions

string AS_expResultToString(AS_expResult r) {
  switch (r->kind) {
    string res;
    case AS_num:
      res = checked_malloc(10);
      sprintf(res, "%d", r->u.num);
      return res;
    case AS_id:
      res = checked_malloc(11);
      sprintf(res, "%%%d", r->u.idNum);
      return res;
  }
}

void AS_handleMainMethod(FILE *out, A_mainMethod m);
void AS_handleStmList(FILE *out, A_stmList sl, STRTAB_table table);
void AS_handleStm(FILE *out, A_stm s, STRTAB_table table);
void AS_handleAssignStm(FILE *out, A_stm s, STRTAB_table table);
void AS_handlePutint(FILE *out, A_stm s, STRTAB_table table);
void AS_handlePutch(FILE *out, A_stm s, STRTAB_table table);
AS_expResult AS_handleExp(FILE *out, A_exp e, STRTAB_table table, string id);
AS_expResult AS_handleOpExp(FILE *out, A_exp e, STRTAB_table table, string id);
AS_expResult AS_handleEscExp(FILE *out, A_exp e, STRTAB_table table, string id);
AS_expResult AS_handleNumConst(FILE *out, A_exp e, STRTAB_table table);
AS_expResult AS_handleIdExp(FILE *out, A_exp e, STRTAB_table table);
AS_expResult AS_handleMinusExp(FILE *out, A_exp e, STRTAB_table table, string id);

// helper functions

int AS_acquireNewVar(STRTAB_table table);
int AS_createTempVar(STRTAB_table table);

void AS_generateLLVMcode(FILE *out, A_prog p) {
  // ASSUME THE AST IS ALWAYS CORRECT.
  if (p->m) {
    AS_handleMainMethod(out, p->m);
  } else {
    fprintf(out, "Error: There's no main class!\n");
  } 
}

void AS_handleMainMethod(FILE *out, A_mainMethod m) {
  if (m->sl) {
    A_stmList slHead = m->sl;

    STRTAB_table table = STRTAB_empty();
    STRTAB_enter(table, "LLVMIR_num", 1);

    AS_handleStmList(out, slHead, table);
  }
}

void AS_handleStmList(FILE *out, A_stmList sl, STRTAB_table table) {
  if (!sl) return;
  AS_handleStm(out, sl->head, table);
  AS_handleStmList(out, sl->tail, table);
}

void AS_handleStm(FILE *out, A_stm s, STRTAB_table table) {
  if (!s) return;
  switch (s->kind) {
    case A_assignStm:
      AS_handleAssignStm(out, s, table);
      break;
    case A_putint:
      AS_handlePutint(out, s, table);
      break;
    case A_putch:
      AS_handlePutch(out, s, table);
      break;
  }
  return;
}

void AS_handleAssignStm(FILE *out, A_stm s, STRTAB_table table) {
  string id = s->u.assign.id->u.v;
  AS_expResult rightRes = AS_handleExp(out, s->u.assign.value, table, id);
  if (rightRes->kind == AS_num) {
    int idNum = AS_acquireNewVar(table);
    STRTAB_enter(table, id, idNum);
    fprintf(out, "%%%d = add i64 0, %d\n", idNum, rightRes->u.num);
  } else if (rightRes->kind == AS_id) { 
    // don't need to acquire a new idNum
    // because we do it in AS_handleExp
    STRTAB_enter(table, id, rightRes->u.idNum);
  }
}

void AS_handlePutint(FILE *out, A_stm s, STRTAB_table table) {
  AS_expResult res = AS_handleExp(out, s->u.e, table, NULL);
  fprintf(out, "call void @putint(i64 %s)\ncall void @putch(i64 10)\n", AS_expResultToString(res));
}

void AS_handlePutch(FILE *out, A_stm s, STRTAB_table table) {
  AS_expResult res = AS_handleExp(out, s->u.e, table, NULL);
  fprintf(out, "call void @putch(i64 %s)\n", AS_expResultToString(res));
}

AS_expResult AS_handleExp(FILE *out, A_exp e, STRTAB_table table, string id) {
  if (!e) return NULL;
  switch (e->kind) {
    case A_opExp:
      return AS_handleOpExp(out, e, table, id);
    case A_escExp:
      return AS_handleEscExp(out, e, table, id);
    case A_numConst:
      return AS_handleNumConst(out, e, table);
    case A_idExp:
      return AS_handleIdExp(out, e, table);
    case A_minusExp:
      return AS_handleMinusExp(out, e, table, id);
  }
  return NULL;
}

AS_expResult AS_handleOpExp(FILE *out, A_exp e, STRTAB_table table, string id) {
  AS_expResult leftRes = AS_handleExp(out, e->u.op.left, table, NULL);
  AS_expResult rightRes = AS_handleExp(out, e->u.op.right, table, NULL);
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
  int idNum = id? AS_acquireNewVar(table): AS_createTempVar(table);  
  fprintf(out, "%%%d = %s i64 %s, %s\n", idNum, op, AS_expResultToString(leftRes), AS_expResultToString(rightRes));
  return AS_IdNum(idNum);
}

AS_expResult AS_handleEscExp(FILE *out, A_exp e, STRTAB_table table, string id) {
  AS_handleStmList(out, e->u.escExp.ns, table);
  return AS_handleExp(out, e->u.escExp.exp, table, id);
}

AS_expResult AS_handleNumConst(FILE *out, A_exp e, STRTAB_table table) {
  return AS_Num(e->u.num);
}

AS_expResult AS_handleIdExp(FILE *out, A_exp e, STRTAB_table table) {
  int idNum = STRTAB_look(table, e->u.v);
  if (idNum == -1) {
    fprintf(stdout, "Error: id %s not found in table\n", e->u.v);
    exit(1);
  }
  return AS_IdNum(idNum);
}

AS_expResult AS_handleMinusExp(FILE *out, A_exp e, STRTAB_table table, string id) {
  AS_expResult res = AS_handleExp(out, e->u.e, table, NULL);
  int idNum = id? AS_acquireNewVar(table): AS_createTempVar(table);
  fprintf(out, "%%%d = sub i64 0, %s\n", idNum, AS_expResultToString(res));
  return AS_IdNum(idNum);
}

int AS_acquireNewVar(STRTAB_table table) {
  int oldnum = STRTAB_look(table, "LLVMIR_num");
  STRTAB_enter(table, "LLVMIR_num", oldnum + 1);
  return oldnum;
}

int AS_createTempVar(STRTAB_table table) {
  int idNum = AS_acquireNewVar(table);
  string id = checked_malloc(15);
  sprintf(id, "temp_%d", idNum);
  STRTAB_enter(table, id, idNum);
  return idNum;
}