#include <stdio.h>
#include "util.h"
#include "fdmjslpast.h"

A_pos A_Pos(int l, int c) {
  A_pos pos = checked_malloc(sizeof(*pos));
  pos->line = l;
  pos->pos = c;
  return pos;
}

A_prog A_Prog(A_pos p, A_mainMethod m) {
  A_prog prg = checked_malloc(sizeof(*prg));
  prg->pos = p;
  prg->m = m;
  return prg;
}

A_mainMethod A_MainMethod(A_pos pos, A_stmList stml) {
  A_mainMethod mc = checked_malloc(sizeof(*mc));
  mc->pos = pos;
  mc->sl = stml;
  return mc;
}

A_stmList A_StmList(A_stm h, A_stmList t) {
  A_stmList stm = checked_malloc(sizeof(*stm));
  stm->head = h;
  stm->tail = t;
  return (stm);
}

// March 20, 2023. updated
A_stm A_AssignStm(A_pos pos, A_exp id, A_exp value) {
  A_stm s = checked_malloc(sizeof(*s));
  s->pos = pos;
  s->kind = A_assignStm;
  s->u.assign.id = id;
  s->u.assign.value = value;
  return s;
}

A_stm A_Putint(A_pos pos, A_exp e) {
  A_stm s = checked_malloc(sizeof(*s));
  s->pos = pos;
  s->kind = A_putint;
  s->u.e = e;
  return s;
}

A_stm A_Putch(A_pos pos, A_exp e) {
  A_stm s = checked_malloc(sizeof(*s));
  s->pos = pos;
  s->kind = A_putch;
  s->u.e = e;
  return s;
}

A_exp A_OpExp(A_pos pos, A_exp e1, A_binop op, A_exp e2) {
  A_exp e = checked_malloc(sizeof(*e));
  e->pos = pos;
  e->kind = A_opExp;
  e->u.op.left = e1;
  e->u.op.oper = op;
  e->u.op.right = e2;
  return e;
}

A_exp A_NumConst(A_pos pos, int i) {
  A_exp e = checked_malloc(sizeof(*e));
  e->pos = pos;
  e->kind = A_numConst;
  e->u.num = i;
  return e;
}

A_exp A_IdExp(A_pos pos, string id) {
  A_exp e = checked_malloc(sizeof(*e));
  e->pos = pos;
  e->kind = A_idExp;
  e->u.v = id;
  return e;
}

A_exp A_MinusExp(A_pos pos, A_exp val) {
  A_exp e = checked_malloc(sizeof(*e));
  e->pos = pos;
  e->kind = A_minusExp;
  e->u.e = val;
  return e;
}

A_exp A_EscExp(A_pos pos, A_stmList sl, A_exp val) {
  A_exp e = checked_malloc(sizeof(*e));
  e->pos = pos;
  e->kind = A_escExp;
  e->u.escExp.ns = sl;
  e->u.escExp.exp = val;
  return e;
}
