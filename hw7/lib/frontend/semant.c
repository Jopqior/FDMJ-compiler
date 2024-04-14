#include "semant.h"

#define __DEBUG
#undef __DEBUG

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
  loopstack tmp = loopstack_head;
}

static inline bool loopstack_empty() { return loopstack_head == NULL; }

/* envs */

static S_table venv;
static S_table cenv;
static S_symbol MAIN_CLASS;
static string curClassId;
static string curMethodId;

void transError(FILE *out, A_pos pos, string msg) {
  fprintf(out, "(line:%d col:%d) %s\n", pos->line, pos->pos, msg);
  fflush(out);
  exit(1);
}

/* TODO: semant */

T_funcDeclList transA_Prog(FILE *out, A_prog p, int arch_size) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_Prog...\n");
#endif
  // init
  SEM_ARCH_SIZE = arch_size;
  MAIN_CLASS = S_Symbol(String("0Main"));

  if (p->cdl) {
    transPreprocess(out, p->cdl);
  }

  if (!(p->m)) {
    transError(out, p->pos, String("error: there's no main class"));
  }

  return transSemant(out, p->cdl, p->m);
}

/* preprocess */

void transPreprocess(FILE *out, A_classDeclList cdl) {
#ifdef __DEBUG
  fprintf(out, "Entering transPreprocess...\n");
#endif
  // TODO: preprocess
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

  // TODO: variable declaration
}

// main method
T_funcDecl transA_MainMethod(FILE *out, A_mainMethod main) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_MainMethod...\n");
#endif
  S_beginScope(venv);

  curClassId = S_name(MAIN_CLASS);

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

  if (MAIN_CLASS == S_Symbol(curClassId)) {
    expty ret = transA_Exp(out, s->u.e, Ty_Int());
    if (!ret) {
      return NULL;
    }

    switch (ret->value->kind) {
      case Ty_int:
      case Ty_float:
        return Tr_Return(ret->exp);
      default:
        transError(out, s->pos,
                   String("error: return value of main method must be of type "
                          "int or float"));
    }
  }

  // TODO: return statement in class method
  return NULL;
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
    case Ty_float:
      return Tr_Putch(ch->exp);
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

expty transA_Exp(FILE *out, A_exp e, Ty_ty type) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_Exp...\n");
#endif
  return NULL;
}