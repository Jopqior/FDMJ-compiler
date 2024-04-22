#include "semant.h"

#ifndef __DEBUG
#define __DEBUG
#undef __DEBUG
#endif

int SEM_ARCH_SIZE;  // to be set by arch_size in transA_Prog

/* structs */

expty ExpTy(Tr_exp exp, Ty_ty value, Ty_ty location) {
  expty et = checked_malloc(sizeof(*et));
  et->exp = exp;
  et->value = value;
  et->location = location;
  return et;
}

// stack for while loop
typedef struct loopstack_ *loopstack;
struct loopstack_ {
  Temp_label whiletest;
  Temp_label whileend;
  loopstack next;
};

static loopstack loopstack_head = NULL;

static void loopstack_push(Temp_label whiletest, Temp_label whileend) {
  loopstack new = checked_malloc(sizeof(*new));
  new->whiletest = whiletest;
  new->whileend = whileend;
  new->next = loopstack_head;
  loopstack_head = new;
}

static void loopstack_pop() {
  if (!loopstack_head) {
    return;
  }
  loopstack_head = loopstack_head->next;
}

static inline bool loopstack_empty() { return loopstack_head == NULL; }

/* envs */

static S_table venv;

void transError(FILE *out, A_pos pos, string msg) {
  fprintf(out, "(line:%d col:%d) %s\n", pos->line, pos->pos, msg);
  fflush(out);
  exit(1);
}

T_funcDeclList transA_Prog(FILE *out, A_prog p, int arch_size) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_Prog...\n");
#endif
  // init
  SEM_ARCH_SIZE = arch_size;
  venv = S_empty();

  if (!(p->m)) {
    transError(out, p->pos, String("error: there's no main class"));
  }

  return transSemant(out, p->cdl, p->m);
}

/* semantic analysis */
T_funcDeclList transSemant(FILE *out, A_classDeclList cdl, A_mainMethod m) {
#ifdef __DEBUG
  fprintf(out, "Entering transSemant...\n");
#endif
  // TODO: class
  T_funcDecl mainmethod = transA_MainMethod(out, m);
  return Tr_FuncDeclList(mainmethod, NULL);
}

// classes
Tr_exp transA_VarDeclList(FILE *out, A_varDeclList vdl) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_VarDeclList...\n");
#endif
  if (!vdl) {
    return NULL;
  }

  Tr_exp vd = transA_VarDecl(out, vdl->head);
  Tr_exp rest = transA_VarDeclList(out, vdl->tail);

  return Tr_StmList(vd, rest);
}

Tr_exp transA_VarDecl(FILE *out, A_varDecl vd) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_VarDecl...\n");
#endif
  if (!vd) {
    return NULL;
  }

  if (S_look(venv, S_Symbol(vd->v)) != NULL) {
    transError(out, vd->pos,
               String("error: variable already declared in main method"));
  }

  // enter the variable into the environment
  Temp_temp temp;
  switch (vd->t->t) {
    case A_intType: {
      temp = Temp_newtemp(T_int);
      S_enter(venv, S_Symbol(vd->v), E_VarEntry(vd, Ty_Int(), temp));
      break;
    }
    case A_floatType: {
      temp = Temp_newtemp(T_float);
      S_enter(venv, S_Symbol(vd->v), E_VarEntry(vd, Ty_Float(), temp));
      break;
    }
    default:
      break;  // Not Implemented
  }

  if (vd->elist) {
    switch (vd->t->t) {
      case A_intType:
        return Tr_AssignStm(
            Tr_IdExp(temp),
            transA_Exp_NumConst(out, vd->elist->head, Ty_Int()));
      case A_floatType:
        return Tr_AssignStm(
            Tr_IdExp(temp),
            transA_Exp_NumConst(out, vd->elist->head, Ty_Float()));
      default:
        return NULL;  // Not Implemented
    }
  }

  return NULL;
}

// main method
T_funcDecl transA_MainMethod(FILE *out, A_mainMethod main) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_MainMethod...\n");
#endif
  S_beginScope(venv);

  Tr_exp vdl = transA_VarDeclList(out, main->vdl);
  Tr_exp sl = transA_StmList(out, main->sl);

  S_endScope(venv);

  return Tr_MainMethod(vdl, sl);
}

// stms
Tr_exp transA_StmList(FILE *out, A_stmList sl) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_StmList...\n");
#endif
  if (!sl) {
    return NULL;
  }
  Tr_exp s = transA_Stm(out, sl->head);
  Tr_exp rest = transA_StmList(out, sl->tail);
  return Tr_StmList(s, rest);
}

Tr_exp transA_Stm(FILE *out, A_stm s) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_Stm...\n");
#endif
  if (!s) {
    return NULL;
  }

  switch (s->kind) {
    case A_nestedStm:
      return transA_NestedStm(out, s);
    case A_ifStm:
      return transA_IfStm(out, s);
    case A_whileStm:
      return transA_WhileStm(out, s);
    case A_assignStm:
      return transA_AssignStm(out, s);
    case A_arrayInit:
      return transA_ArrayInit(out, s);
    case A_callStm:
      return transA_CallStm(out, s);
    case A_continue:
      return transA_Continue(out, s);
    case A_break:
      return transA_Break(out, s);
    case A_return:
      return transA_Return(out, s);
    case A_putnum:
      return transA_Putnum(out, s);
    case A_putch:
      return transA_Putch(out, s);
    case A_putarray:
      return transA_Putarray(out, s);
    case A_starttime:
      return transA_Starttime(out, s);
    case A_stoptime:
      return transA_Stoptime(out, s);
    default:
      transError(out, s->pos, String("error: unknown statement"));
  }
}

Tr_exp transA_NestedStm(FILE *out, A_stm s) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_NestedStm...\n");
#endif
  if (!s) {
    return NULL;
  }

  return transA_StmList(out, s->u.ns);
}

Tr_exp transA_IfStm(FILE *out, A_stm s) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_IfStm...\n");
#endif
  if (!s) {
    return NULL;
  }

  expty condition = transA_Exp(out, s->u.if_stat.e, NULL);
  if (!condition) {
    return NULL;
  }
  if (condition->value->kind != Ty_int && condition->value->kind != Ty_float) {
    transError(
        out, s->pos,
        String("error: if statement condition must be of type int or float"));
  }

  Tr_exp then = transA_Stm(out, s->u.if_stat.s1);
  Tr_exp elsee = transA_Stm(out, s->u.if_stat.s2);

  return Tr_IfStm(condition->exp, then, elsee);
}

Tr_exp transA_WhileStm(FILE *out, A_stm s) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_WhileStm...\n");
#endif
  if (!s) {
    return NULL;
  }

  expty test = transA_Exp(out, s->u.while_stat.e, NULL);
  if (!test) {
    return NULL;
  }
  if (test->value->kind != Ty_int && test->value->kind != Ty_float) {
    transError(
        out, s->pos,
        String(
            "error: while statement condition must be of type int or float"));
  }

  Temp_label whiletest = Temp_newlabel();
  Temp_label whileend = Temp_newlabel();
  loopstack_push(whiletest, whileend);

  Tr_exp loop = transA_Stm(out, s->u.while_stat.s);

  Tr_exp whilestm = Tr_WhileStm(test->exp, loop, whiletest, whileend);

  loopstack_pop();

  return whilestm;
}

Tr_exp transA_AssignStm(FILE *out, A_stm s) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_AssignStm...\n");
#endif
  if (!s) {
    return NULL;
  }

  expty left = transA_Exp(out, s->u.assign.arr, NULL);
  if (!left) {
    return NULL;
  }
  if (!left->location) {
    transError(
        out, s->pos,
        String("error: left side of assignment must have a location value"));
  }

  expty right = transA_Exp(out, s->u.assign.value, left->location);
  if (!right) {
    return NULL;
  }
  if ((left->location->kind == Ty_int || left->location->kind == Ty_float) &&
      (right->value->kind != Ty_int && right->value->kind != Ty_float)) {
    if (left->location->kind == Ty_int) {
      transError(out, s->pos,
                 String("error: right side of assignment must be of type int"));
    } else {
      transError(
          out, s->pos,
          String("error: right side of assignment must be of type float"));
    }
  }

  return Tr_AssignStm(left->exp, right->exp);
}

Tr_exp transA_ArrayInit(FILE *out, A_stm s) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_ArrayInit...\n");
#endif
  // Not Implemented
  return NULL;
}

Tr_exp transA_CallStm(FILE *out, A_stm s) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_CallStm...\n");
#endif
  // Not Implemented
  return NULL;
}

Tr_exp transA_Continue(FILE *out, A_stm s) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_Continue...\n");
#endif
  if (!s) {
    return NULL;
  }

  if (loopstack_empty()) {
    transError(out, s->pos, String("error: continue statement outside loop"));
  }

  return Tr_Continue(loopstack_head->whiletest);
}

Tr_exp transA_Break(FILE *out, A_stm s) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_Break...\n");
#endif
  if (!s) {
    return NULL;
  }

  if (loopstack_empty()) {
    transError(out, s->pos, String("error: break statement outside loop"));
  }

  return Tr_Break(loopstack_head->whileend);
}

Tr_exp transA_Return(FILE *out, A_stm s) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_Return...\n");
#endif
  if (!s) {
    return NULL;
  }

  expty ret = transA_Exp(out, s->u.e, Ty_Int());
  if (!ret) {
    return NULL;
  }

  switch (ret->value->kind) {
    case Ty_int:
      return Tr_Return(ret->exp);
    case Ty_float:
      return Tr_Return(Tr_Cast(ret->exp, T_int));
    default:
      transError(out, s->pos,
                 String("error: return value of main method must be of type "
                        "int or float"));
  }
}

Tr_exp transA_Putnum(FILE *out, A_stm s) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_Putnum...\n");
#endif
  if (!s) {
    return NULL;
  }

  expty num = transA_Exp(out, s->u.e, NULL);
  if (!num) {
    return NULL;
  }

  switch (num->value->kind) {
    case Ty_int:
      return Tr_Putint(num->exp);
    case Ty_float:
      return Tr_Putfloat(num->exp);
    default:
      transError(out, s->pos,
                 String("error: argument of putnum() must be of "
                        "type int or float"));
  }
}

Tr_exp transA_Putch(FILE *out, A_stm s) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_Putch...\n");
#endif
  if (!s) {
    return NULL;
  }

  expty ch = transA_Exp(out, s->u.e, NULL);
  if (!ch) {
    return NULL;
  }

  switch (ch->value->kind) {
    case Ty_int:
      return Tr_Putch(ch->exp);
    case Ty_float:
      return Tr_Putch(Tr_Cast(ch->exp, T_int));
    default:
      transError(
          out, s->pos,
          String("error: argument of putch() must be of type int or float"));
  }
}

Tr_exp transA_Putarray(FILE *out, A_stm s) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_Putarray...\n");
#endif
  // Not Implemented
  return NULL;
}

Tr_exp transA_Starttime(FILE *out, A_stm s) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_Starttime...\n");
#endif
  if (!s) {
    return NULL;
  }

  return Tr_Starttime();
}

Tr_exp transA_Stoptime(FILE *out, A_stm s) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_Stoptime...\n");
#endif
  if (!s) {
    return NULL;
  }

  return Tr_Stoptime();
}

// exps
Tr_exp transA_Exp_NumConst(FILE *out, A_exp e, Ty_ty type) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_Exp_NumConst...\n");
#endif
  if (!e) {
    return NULL;
  }

  switch (type->kind) {
    case Ty_int:
      return Tr_NumConst((int)(e->u.num), T_int);
    case Ty_float:
      return Tr_NumConst(e->u.num, T_float);
    default:
      return NULL;  // unreachable
  }
}

expty transA_Exp(FILE *out, A_exp e, Ty_ty type) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_Exp...\n");
#endif
  if (!e) {
    return NULL;
  }

  switch (e->kind) {
    case A_opExp:
      return transA_OpExp(out, e);
    case A_arrayExp:
      return transA_ArrayExp(out, e);
    case A_callExp:
      return transA_CallExp(out, e);
    case A_classVarExp:
      return transA_ClassVarExp(out, e);
    case A_boolConst:
      return transA_BoolConst(out, e);
    case A_numConst:
      return transA_NumConst(out, e, type);
    case A_lengthExp:
      return transA_LengthExp(out, e);
    case A_idExp:
      return transA_IdExp(out, e);
    case A_thisExp:
      return transA_ThisExp(out, e);
    case A_newIntArrExp:
      return transA_NewIntArrExp(out, e);
    case A_newFloatArrExp:
      return transA_NewFloatArrExp(out, e);
    case A_newObjExp:
      return transA_NewObjExp(out, e);
    case A_notExp:
      return transA_NotExp(out, e);
    case A_minusExp:
      return transA_MinusExp(out, e);
    case A_escExp:
      return transA_EscExp(out, e);
    case A_getnum:
      return transA_Getnum(out, e);
    case A_getch:
      return transA_Getch(out, e);
    case A_getarray:
      return transA_Getarray(out, e);
    default:
      transError(out, e->pos, String("error: unknown expression"));
  }
}

expty transA_OpExp(FILE *out, A_exp e) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_OpExp...\n");
#endif
  if (!e) {
    return NULL;
  }

  expty left = transA_Exp(out, e->u.op.left, NULL);
  if (!left) {
    return NULL;
  }
  if (left->value->kind != Ty_int && left->value->kind != Ty_float) {
    transError(out, e->pos,
               String("error: left side of operator must be of type int or "
                      "float"));
  }

  expty right = transA_Exp(out, e->u.op.right, NULL);
  if (!right) {
    return NULL;
  }
  if (right->value->kind != Ty_int && right->value->kind != Ty_float) {
    transError(out, e->pos,
               String("error: right side of operator must be of type int or "
                      "float"));
  }

  Tr_exp opexp = Tr_OpExp(e->u.op.oper, left->exp, right->exp);

  // return type of the operator
  switch (e->u.op.oper) {
    case A_and:
    case A_or:
    case A_less:
    case A_le:
    case A_greater:
    case A_ge:
    case A_eq:
    case A_ne:
      return ExpTy(opexp, Ty_Int(), NULL);
    case A_plus:
    case A_minus:
    case A_times:
    case A_div: {
      if (left->value->kind == Ty_int && right->value->kind == Ty_int) {
        return ExpTy(opexp, Ty_Int(), NULL);
      } else {
        return ExpTy(opexp, Ty_Float(), NULL);
      }
    }
    default:
      return NULL;  // unreachable
  }
}

expty transA_ArrayExp(FILE *out, A_exp e) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_ArrayExp...\n");
#endif
  // Not Implemented
  return NULL;
}

expty transA_CallExp(FILE *out, A_exp e) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_CallExp...\n");
#endif
  // Not Implemented
  return NULL;
}

expty transA_ClassVarExp(FILE *out, A_exp e) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_ClassVarExp...\n");
#endif
  // Not Implemented
  return NULL;
}

expty transA_BoolConst(FILE *out, A_exp e) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_BoolConst...\n");
#endif
  if (!e) {
    return NULL;
  }

  return ExpTy(Tr_BoolConst(e->u.b), Ty_Int(), NULL);
}

expty transA_NumConst(FILE *out, A_exp e, Ty_ty type) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_NumConst...\n");
#endif
  if (!e) {
    return NULL;
  }

  float num = e->u.num;
  T_type t;
  if (type) {
    t = type->kind == Ty_int ? T_int : T_float;
  } else {
    t = num == (int)num ? T_int : T_float;
  }

  Tr_exp exp = Tr_NumConst(num, t);
  switch (t) {
    case T_int:
      return ExpTy(exp, Ty_Int(), NULL);
    case T_float:
      return ExpTy(exp, Ty_Float(), NULL);
    default:
      return NULL;  // unreachable
  }
}

expty transA_LengthExp(FILE *out, A_exp e) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_LengthExp...\n");
#endif
  // Not Implemented
  return NULL;
}
expty transA_IdExp(FILE *out, A_exp e) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_IdExp...\n");
#endif
  if (!e) {
    return NULL;
  }

  E_enventry x = S_look(venv, S_Symbol(e->u.v));
  if (!x) {
    transError(out, e->pos, String("error: variable not declared"));
  }

  return ExpTy(Tr_IdExp(x->u.var.tmp), x->u.var.ty, x->u.var.ty);
}

expty transA_ThisExp(FILE *out, A_exp e) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_ThisExp...\n");
#endif
  // Not Implemented
  return NULL;
}

expty transA_NewIntArrExp(FILE *out, A_exp e) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_NewIntArrExp...\n");
#endif
  // Not Implemented
  return NULL;
}

expty transA_NewFloatArrExp(FILE *out, A_exp e) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_NewFloatArrExp...\n");
#endif
  // Not Implemented
  return NULL;
}

expty transA_NewObjExp(FILE *out, A_exp e) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_NewObjExp...\n");
#endif
  // Not Implemented
  return NULL;
}

expty transA_NotExp(FILE *out, A_exp e) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_NotExp...\n");
#endif
  if (!e) {
    return NULL;
  }

  expty exp = transA_Exp(out, e->u.e, NULL);
  if (!exp) {
    return NULL;
  }
  if (exp->value->kind != Ty_int && exp->value->kind != Ty_float) {
    transError(out, e->pos, String("error: ! must operate on int or float"));
  }

  return ExpTy(Tr_NotExp(exp->exp), Ty_Int(), NULL);
}

expty transA_MinusExp(FILE *out, A_exp e) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_MinusExp...\n");
#endif
  if (!e) {
    return NULL;
  }

  expty exp = transA_Exp(out, e->u.e, NULL);
  if (!exp) {
    return NULL;
  }
  if (exp->value->kind != Ty_int && exp->value->kind != Ty_float) {
    transError(out, e->pos, String("error: - must operate on int or float"));
  }

  return ExpTy(Tr_MinusExp(exp->exp), exp->value, NULL);
}

expty transA_EscExp(FILE *out, A_exp e) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_EscExp...\n");
#endif
  if (!e) {
    return NULL;
  }

  Tr_exp stm = transA_StmList(out, e->u.escExp.ns);

  expty exp = transA_Exp(out, e->u.escExp.exp, NULL);
  if (!exp) {
    return NULL;
  }

  return ExpTy(Tr_EscExp(stm, exp->exp), exp->value, NULL);
}

expty transA_Getnum(FILE *out, A_exp e) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_Getnum...\n");
#endif
  if (!e) {
    return NULL;
  }

  return ExpTy(Tr_Getfloat(), Ty_Float(), NULL);
}

expty transA_Getch(FILE *out, A_exp e) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_Getch...\n");
#endif
  if (!e) {
    return NULL;
  }

  return ExpTy(Tr_Getch(), Ty_Int(), NULL);
}

expty transA_Getarray(FILE *out, A_exp e) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_Getarray...\n");
#endif
  // Not Implemented
  return NULL;
}
