#include "translate.h"

extern int SEM_ARCH_SIZE;

/* patchList */

typedef struct patchList_ *patchList;
struct patchList_ {
  Temp_label *head;
  patchList tail;
};

static patchList PatchList(Temp_label *head, patchList tail) {
  patchList p = checked_malloc(sizeof(*p));
  p->head = head;
  p->tail = tail;
  return p;
}

void doPatch(patchList pl, Temp_label tl) {
  for (; pl; pl = pl->tail)
    *(pl->head) = tl;
}

patchList joinPatch(patchList first, patchList second) {
  if (!first) return second;
  if (!second) return first;
  patchList tmp = first;
  while (tmp->tail) tmp = tmp->tail;
  tmp->tail = second;
  return first;
}

/* Tr_exp */

typedef struct Cx_ *Cx;

struct Cx_ {
  patchList trues;
  patchList falses;
  T_stm stm;
};

struct Tr_exp_ {
  enum  {Tr_ex, Tr_nx, Tr_cx} kind;
  union {
    T_exp ex;
    T_stm nx;
    Cx cx;
  } u;
};

static Tr_exp Tr_Ex(T_exp ex) {
  Tr_exp exp = checked_malloc(sizeof(*exp));
  exp->kind = Tr_ex;
  exp->u.ex = ex;
  return exp;
}

static Tr_exp Tr_Nx(T_stm nx) {
  Tr_exp exp = checked_malloc(sizeof(*exp));
  exp->kind = Tr_nx;
  exp->u.nx = nx;
  return exp;
}

static Tr_exp Tr_Cx(patchList trues, patchList falses, T_stm stm) {
  Tr_exp exp = checked_malloc(sizeof(*exp));
  exp->kind = Tr_cx;
  exp->u.cx = checked_malloc(sizeof(*(exp->u.cx)));
  exp->u.cx->trues = trues;
  exp->u.cx->falses = falses;
  exp->u.cx->stm = stm;
  return exp;
}

static T_exp unEx(Tr_exp exp) {
  if (!exp) return NULL;
  switch (exp->kind) {
  case Tr_ex:
    return exp->u.ex;
  case Tr_cx: {
    Temp_temp r = Temp_newtemp(T_int);
    Temp_label t = Temp_newlabel();
    Temp_label f = Temp_newlabel();
    Temp_label e = Temp_newlabel();
    doPatch(exp->u.cx->trues, t);
    doPatch(exp->u.cx->falses, f);
    return
    T_Eseq(
    T_Seq(exp->u.cx->stm,
    T_Seq(T_Label(t),
    T_Seq(T_Move(T_Temp(r), T_IntConst(1)),
    T_Seq(T_Jump(e),
    T_Seq(T_Label(f),
    T_Seq(T_Move(T_Temp(r), T_IntConst(0)), 
    T_Label(e))))))),
    T_Temp(r));
  }
  case Tr_nx:
    return T_Eseq(exp->u.nx, T_IntConst(0));
  default:
    assert(0);
  }
}

static T_stm unNx(Tr_exp exp) {
  if (!exp) return NULL;
  switch (exp->kind) {
  case Tr_ex:
    return T_Exp(exp->u.ex);
  case Tr_cx: {
    Temp_temp r = Temp_newtemp(T_int);
    Temp_label t = Temp_newlabel();
    Temp_label f = Temp_newlabel();
    Temp_label e = Temp_newlabel();
    doPatch(exp->u.cx->trues, t);
    doPatch(exp->u.cx->falses, f);
    return
    T_Seq(exp->u.cx->stm,
    T_Seq(T_Label(t), 
    T_Seq(T_Move(T_Temp(r), T_IntConst(1)),
    T_Seq(T_Jump(e),
    T_Seq(T_Label(f),
    T_Seq(T_Move(T_Temp(r), T_IntConst(0)),
    T_Seq(T_Label(e),
    T_Exp(T_Temp(r)))))))));
  }
  case Tr_nx:
    return exp->u.nx;
  default:
    assert(0);
  }
}

static Cx unCx(Tr_exp exp) {
  if (!exp) return NULL;
  switch (exp->kind) {
  case Tr_ex: {
    T_stm stm = T_Cjump(T_ne, unEx(exp), T_IntConst(0), NULL, NULL);
    patchList trues = PatchList(&stm->u.CJUMP.t, NULL);
    patchList falses = PatchList(&stm->u.CJUMP.f, NULL);
    Tr_exp cx = Tr_Cx(trues, falses, stm);
    return cx->u.cx;
  }
  case Tr_cx:
    return exp->u.cx;
  default:
    assert(0);
  }
}

/* Tr_expList */

struct Tr_expList_ {
  Tr_exp head;
  Tr_expList tail;
};

/* TODO: translate */

// methods

// stms

// exps
