#include "translate.h"

#ifndef __DEBUG
#define __DEBUG
#undef __DEBUG
#endif

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
  for (; pl; pl = pl->tail) *(pl->head) = tl;
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
  enum { Tr_ex, Tr_nx, Tr_cx } kind;
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
      return T_Eseq(
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
    case Tr_cx:
      return exp->u.cx->stm;
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
T_funcDeclList Tr_FuncDeclList(T_funcDecl fd, T_funcDeclList fdl) {
#ifdef __DEBUG
  fprintf(stderr, "\tEntering Tr_FuncDeclList...\n");
#endif
  if (!fd) {
    return fdl;
  }
  return T_FuncDeclList(fd, fdl);
}

T_funcDecl Tr_MainMethod(Tr_exp vdl, Tr_exp sl) {
#ifdef __DEBUG
  fprintf(stderr, "\tEntering Tr_MainMethod...\n");
#endif
  return T_FuncDecl(String("main"), NULL, unNx(Tr_StmList(vdl, sl)));
}

// stms
Tr_exp Tr_StmList(Tr_exp head, Tr_exp tail) {
#ifdef __DEBUG
  fprintf(stderr, "\tEntering Tr_StmList...\n");
#endif
  if (!head && !tail) {
    return NULL;
  }
  if (!head) {
    return Tr_Nx(unNx(tail));
  }
  if (!tail) {
    return Tr_Nx(unNx(head));
  }
  return Tr_Nx(T_Seq(unNx(head), unNx(tail)));
}

Tr_exp Tr_IfStm(Tr_exp test, Tr_exp then, Tr_exp elsee) {
#ifdef __DEBUG
  fprintf(stderr, "\tEntering Tr_IfStm...\n");
#endif
  /*
  [1. then is NULL and elsee is NULL]
      Cjump(T_ne, test, 0, e, e)
    e:

  [2. then is NULL and elsee is not NULL]
      Cjump(T_ne, test, 0, e, f)
    f:
      elsee
    e:

  [3. then is not NULL and elsee is NULL]
      Cjump(T_ne, test, 0, t, e)
    t:
      then
    e:

  [4. then is not NULL and elsee is not NULL]
      Cjump(T_ne, test, 0, t, f)
    t:
      then
      Jump(e)
    f:
      elsee
    e:
  */
  if (!then && !elsee) {
    Temp_label e = Temp_newlabel();

    Cx cx = unCx(test);
    doPatch(cx->trues, e);
    doPatch(cx->falses, e);

    return Tr_Nx(T_Seq(cx->stm, T_Label(e)));
  }

  if (!then) {
    Temp_label e = Temp_newlabel();
    Temp_label f = Temp_newlabel();

    Cx cx = unCx(test);
    doPatch(cx->trues, e);
    doPatch(cx->falses, f);

    return Tr_Nx(
        T_Seq(cx->stm, T_Seq(T_Label(f), T_Seq(unNx(elsee), T_Label(e)))));
  }

  if (!elsee) {
    Temp_label t = Temp_newlabel();
    Temp_label e = Temp_newlabel();

    Cx cx = unCx(test);
    doPatch(cx->trues, t);
    doPatch(cx->falses, e);

    return Tr_Nx(
        T_Seq(cx->stm, T_Seq(T_Label(t), T_Seq(unNx(then), T_Label(e)))));
  }

  Temp_label t = Temp_newlabel();
  Temp_label f = Temp_newlabel();
  Temp_label e = Temp_newlabel();

  Cx cx = unCx(test);
  doPatch(cx->trues, t);
  doPatch(cx->falses, f);

  return Tr_Nx(T_Seq(
      cx->stm,
      T_Seq(T_Label(t),
            T_Seq(unNx(then),
                  T_Seq(T_Jump(e),
                        T_Seq(T_Label(f), T_Seq(unNx(elsee), T_Label(e))))))));
}

Tr_exp Tr_WhileStm(Tr_exp test, Tr_exp loop, Temp_label whiletest,
                   Temp_label whileend) {
#ifdef __DEBUG
  fprintf(stderr, "\tEntering Tr_WhileStm...\n");
#endif
  /*
  [1. loop is NULL]
    whiletest:
      Cjump(T_ne, test, 0, whiletest, whileend)
    whileend:

  [2. loop is not NULL]
    whiletest:
      Cjump(T_ne, test, 0, whileloop, whileend)
    whileloop:
      loop
      Jump(whiletest)
    whileend:
  */

  if (!loop) {
    Cx cx = unCx(test);
    doPatch(cx->trues, whiletest);
    doPatch(cx->falses, whileend);

    return Tr_Nx(T_Seq(T_Label(whiletest), T_Seq(cx->stm, T_Label(whileend))));
  }

  Temp_label whileloop = Temp_newlabel();

  Cx cx = unCx(test);
  doPatch(cx->trues, whileloop);
  doPatch(cx->falses, whileend);

  return Tr_Nx(T_Seq(
      T_Label(whiletest),
      T_Seq(cx->stm, T_Seq(T_Label(whileloop),
                           T_Seq(unNx(loop), T_Seq(T_Jump(whiletest),
                                                   T_Label(whileend)))))));
}

Tr_exp Tr_AssignStm(Tr_exp location, Tr_exp value) {
#ifdef __DEBUG
  fprintf(stderr, "\tEntering Tr_AssignStm...\n");
#endif
  T_exp loc = unEx(location);
  T_exp val = unEx(value);

  if (loc->type != val->type) {
    return Tr_Nx(T_Move(loc, T_Cast(val, loc->type)));
  }
  return Tr_Nx(T_Move(loc, val));
}

Tr_exp Tr_Continue(Temp_label whiletest) {
#ifdef __DEBUG
  fprintf(stderr, "\tEntering Tr_Continue...\n");
#endif
  return Tr_Nx(T_Jump(whiletest));
}

Tr_exp Tr_Break(Temp_label whileend) {
#ifdef __DEBUG
  fprintf(stderr, "\tEntering Tr_Break...\n");
#endif
  return Tr_Nx(T_Jump(whileend));
}

Tr_exp Tr_Return(Tr_exp ret) {
#ifdef __DEBUG
  fprintf(stderr, "\tEntering Tr_Return...\n");
#endif
  return Tr_Nx(T_Return(unEx(ret)));
}

Tr_exp Tr_Putint(Tr_exp exp) {
#ifdef __DEBUG
  fprintf(stderr, "\tEntering Tr_Putint...\n");
#endif
  return Tr_Nx(
      T_Exp(T_ExtCall(String("putint"), T_ExpList(unEx(exp), NULL), T_int)));
}

Tr_exp Tr_Putfloat(Tr_exp exp) {
#ifdef __DEBUG
  fprintf(stderr, "\tEntering Tr_Putfloat...\n");
#endif
  return Tr_Nx(
      T_Exp(T_ExtCall(String("putfloat"), T_ExpList(unEx(exp), NULL), T_int)));
}

Tr_exp Tr_Putch(Tr_exp exp) {
#ifdef __DEBUG
  fprintf(stderr, "\tEntering Tr_Putch...\n");
#endif
  return Tr_Nx(
      T_Exp(T_ExtCall(String("putch"), T_ExpList(unEx(exp), NULL), T_int)));
}

Tr_exp Tr_Starttime() {
#ifdef __DEBUG
  fprintf(stderr, "\tEntering Tr_Starttime...\n");
#endif
  return Tr_Nx(T_Exp(T_ExtCall(String("starttime"), NULL, T_int)));
}

Tr_exp Tr_Stoptime() {
#ifdef __DEBUG
  fprintf(stderr, "\tEntering Tr_Stoptime...\n");
#endif
  return Tr_Nx(T_Exp(T_ExtCall(String("stoptime"), NULL, T_int)));
}

// exps
Tr_expList Tr_ExpList(Tr_exp head, Tr_expList tail) {
#ifdef __DEBUG
  fprintf(stderr, "\tEntering Tr_ExpList...\n");
#endif
  if (!head) {
    return tail;
  }

  Tr_expList p = checked_malloc(sizeof(*p));
  p->head = head;
  p->tail = tail;
  return p;
}

Tr_exp Tr_OpExp(A_binop op, Tr_exp left, Tr_exp right) {
#ifdef __DEBUG
  fprintf(stderr, "\tEntering Tr_OpExp...\n");
#endif
  if (!left || !right) {
    return NULL;
  }

  switch (op) {
    case A_plus:
      return Tr_Ex(T_Binop(T_plus, unEx(left), unEx(right)));
    case A_minus:
      return Tr_Ex(T_Binop(T_minus, unEx(left), unEx(right)));
    case A_times:
      return Tr_Ex(T_Binop(T_mul, unEx(left), unEx(right)));
    case A_div:
      return Tr_Ex(T_Binop(T_div, unEx(left), unEx(right)));
    case A_less: {
      T_stm exp = T_Cjump(T_lt, unEx(left), unEx(right), NULL, NULL);
      patchList trues = PatchList(&(exp->u.CJUMP.t), NULL);
      patchList falses = PatchList(&(exp->u.CJUMP.f), NULL);
      return Tr_Cx(trues, falses, exp);
    }
    case A_le: {
      T_stm exp = T_Cjump(T_le, unEx(left), unEx(right), NULL, NULL);
      patchList trues = PatchList(&(exp->u.CJUMP.t), NULL);
      patchList falses = PatchList(&(exp->u.CJUMP.f), NULL);
      return Tr_Cx(trues, falses, exp);
    }
    case A_greater: {
      T_stm exp = T_Cjump(T_gt, unEx(left), unEx(right), NULL, NULL);
      patchList trues = PatchList(&(exp->u.CJUMP.t), NULL);
      patchList falses = PatchList(&(exp->u.CJUMP.f), NULL);
      return Tr_Cx(trues, falses, exp);
    }
    case A_ge: {
      T_stm exp = T_Cjump(T_ge, unEx(left), unEx(right), NULL, NULL);
      patchList trues = PatchList(&(exp->u.CJUMP.t), NULL);
      patchList falses = PatchList(&(exp->u.CJUMP.f), NULL);
      return Tr_Cx(trues, falses, exp);
    }
    case A_eq: {
      T_stm exp = T_Cjump(T_eq, unEx(left), unEx(right), NULL, NULL);
      patchList trues = PatchList(&(exp->u.CJUMP.t), NULL);
      patchList falses = PatchList(&(exp->u.CJUMP.f), NULL);
      return Tr_Cx(trues, falses, exp);
    }
    case A_ne: {
      T_stm exp = T_Cjump(T_ne, unEx(left), unEx(right), NULL, NULL);
      patchList trues = PatchList(&(exp->u.CJUMP.t), NULL);
      patchList falses = PatchList(&(exp->u.CJUMP.f), NULL);
      return Tr_Cx(trues, falses, exp);
    }
    case A_and: {
      /*
      left && right
      <=>
        Cjump(T_ne, unEx(left), 0, rightlabel, leftf)
      rightlabel:
        Cjump(T_ne, unEx(right), 0, rightt, rightf)
      t:
        ...
      f:
        ...
      [constraint]
        t = rightt
        f = leftf, rightf
      */
      Cx leftc = unCx(left);
      Cx rightc = unCx(right);

      Temp_label rightlabel = Temp_newlabel();
      doPatch(leftc->trues, rightlabel);

      patchList t = rightc->trues;
      patchList f = joinPatch(leftc->falses, rightc->falses);

      T_stm exp = T_Seq(leftc->stm, T_Seq(T_Label(rightlabel), rightc->stm));

      return Tr_Cx(t, f, exp);
    }
    case A_or: {
      /*
      left || right
      <=>
        Cjump(T_ne, unEx(left), 0, leftt, rightlabel)
      rightlabel:
        Cjump(T_ne, unEx(right), 0, rightt, rightf)
      t:
        ...
      f:
        ...
      [constraint]
        t = leftt, rightt
        f = rightf
      */
      Cx leftc = unCx(left);
      Cx rightc = unCx(right);

      Temp_label rightlabel = Temp_newlabel();
      doPatch(leftc->falses, rightlabel);

      patchList t = joinPatch(leftc->trues, rightc->trues);
      patchList f = rightc->falses;

      T_stm exp = T_Seq(leftc->stm, T_Seq(T_Label(rightlabel), rightc->stm));

      return Tr_Cx(t, f, exp);
    }
    default:
      assert(0);
  }
}

Tr_exp Tr_BoolConst(bool b) {
#ifdef __DEBUG
  fprintf(stderr, "\tEntering Tr_BoolConst...\n");
#endif
  return Tr_Ex(T_IntConst(b));
}

Tr_exp Tr_NumConst(float num, T_type type) {
#ifdef __DEBUG
  fprintf(stderr, "\tEntering Tr_NumConst...\n");
#endif
  switch (type)
  {
  case T_int:
    return Tr_Ex(T_IntConst((int)num));
  case T_float:
    return Tr_Ex(T_FloatConst(num));
  }
}

Tr_exp Tr_IdExp(Temp_temp tmp) {
#ifdef __DEBUG
  fprintf(stderr, "\tEntering Tr_IdExp...\n");
#endif
  return Tr_Ex(T_Temp(tmp));
}

Tr_exp Tr_NotExp(Tr_exp exp) {
#ifdef __DEBUG
  fprintf(stderr, "\tEntering Tr_NotExp...\n");
#endif
  /*
  !exp
  <=>
    Cjump(T_eq, unEx(exp), 0, expt, expf)
  t:
    ...
  f:
    ...
  [constraint]
    t = expf
    f = expt
  */
  Cx cx = unCx(exp);
  return Tr_Cx(cx->falses, cx->trues, cx->stm);
}

Tr_exp Tr_MinusExp(Tr_exp exp) {
#ifdef __DEBUG
  fprintf(stderr, "\tEntering Tr_MinusExp...\n");
#endif
  return Tr_Ex(T_Binop(T_minus, T_IntConst(0), unEx(exp)));
}

Tr_exp Tr_EscExp(Tr_exp stm, Tr_exp exp) {
#ifdef __DEBUG
  fprintf(stderr, "\tEntering Tr_EscExp...\n");
#endif
  if (!stm) {
    return Tr_Ex(unEx(exp));
  }

  return Tr_Ex(T_Eseq(unNx(stm), unEx(exp)));
}

Tr_exp Tr_Getint() {
#ifdef __DEBUG
  fprintf(stderr, "\tEntering Tr_Getint...\n");
#endif
  return Tr_Ex(T_ExtCall(String("getint"), NULL, T_int));
}

Tr_exp Tr_Getfloat() {
#ifdef __DEBUG
  fprintf(stderr, "\tEntering Tr_Getfloat...\n");
#endif
  return Tr_Ex(T_ExtCall(String("getfloat"), NULL, T_float));
}

Tr_exp Tr_Getch() {
#ifdef __DEBUG
  fprintf(stderr, "\tEntering Tr_Getch...\n");
#endif
  return Tr_Ex(T_ExtCall(String("getch"), NULL, T_int));
}

Tr_exp Tr_Cast(Tr_exp exp, T_type type) {
#ifdef __DEBUG
  fprintf(stderr, "\tEntering Tr_Cast...\n");
#endif
  return Tr_Ex(T_Cast(unEx(exp), type));
}
