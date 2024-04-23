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
static S_table cenv;
static S_symbol MAIN_CLASS;
static string curClassId;
static string curMethodId;

static int globaloff = 0;
static S_table varoff;
static S_table methoff;

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
  cenv = S_empty();
  MAIN_CLASS = S_Symbol("main");
  varoff = S_empty();
  methoff = S_empty();

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
  if (!cdl) return;

  // first pass: record class names and inheritance
  transA_ClassDeclList_basic(out, cdl);

  // second pass: detect cycles in inheritance
  transA_ClassDeclList_extend(out, cdl);
}

// trans basic class decl info
void transA_ClassDeclList_basic(FILE *out, A_classDeclList cdl) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_ClassDeclList_basic...\n");
#endif
  if (!cdl) return;

  transA_ClassDecl_basic(out, cdl->head);

  if (cdl->tail) {
    transA_ClassDeclList_basic(out, cdl->tail);
  }
}

void transA_ClassDecl_basic(FILE *out, A_classDecl cd) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_ClassDecl_basic with class %s...\n", cd->id);
#endif
  if (!cd) return;

  if (S_look(cenv, S_Symbol(cd->id)) != NULL) {
    transError(out, cd->pos, String("error: class already declared"));
  }

  S_symbol fa = cd->parentID ? S_Symbol(cd->parentID) : MAIN_CLASS;

  curClassId = cd->id;

  // init class variables
  S_table vtbl = S_empty();
  transA_ClassVarDeclList_basic(out, vtbl, cd->vdl);

  // init class methods
  S_table mtbl = S_empty();
  transA_MethodDeclList_basic(out, mtbl, S_Symbol(cd->id), cd->mdl);

  S_enter(cenv, S_Symbol(cd->id),
          E_ClassEntry(cd, fa, E_transInit, vtbl, mtbl));
}

void transA_ClassVarDeclList_basic(FILE *out, S_table vtbl, A_varDeclList vdl) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_ClassVarDeclList_basic...\n");
#endif
  if (!vdl) return;

  transA_ClassVarDecl_basic(out, vtbl, vdl->head);

  if (vdl->tail) {
    transA_ClassVarDeclList_basic(out, vtbl, vdl->tail);
  }
}

void transA_ClassVarDecl_basic(FILE *out, S_table vtbl, A_varDecl vd) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_ClassVarDecl_basic...\n");
#endif
  if (!vd) return;

  // check if the variable is already declared
  if (S_look(vtbl, S_Symbol(vd->v)) != NULL) {
    transError(
        out, vd->pos,
        Stringf("error: variable already declared in class %s", curClassId));
  }

  // enter the variable into the environment
  switch (vd->t->t) {
    case A_intType: {
      S_enter(vtbl, S_Symbol(vd->v), E_VarEntry(vd, Ty_Int(), NULL));
      break;
    }
    case A_floatType: {
      S_enter(vtbl, S_Symbol(vd->v), E_VarEntry(vd, Ty_Float(), NULL));
      break;
    }
    case A_intArrType: {
      S_enter(vtbl, S_Symbol(vd->v), E_VarEntry(vd, Ty_Array(Ty_Int()), NULL));
      break;
    }
    case A_floatArrType: {
      S_enter(vtbl, S_Symbol(vd->v),
              E_VarEntry(vd, Ty_Array(Ty_Float()), NULL));
      break;
    }
    case A_idType: {
      S_enter(vtbl, S_Symbol(vd->v),
              E_VarEntry(vd, Ty_Name(S_Symbol(vd->t->id)), NULL));
      break;
    }
  }

  // record the offset of the variable
  if (offtable_look(varoff, S_Symbol(vd->v)) < 0) {
    offtable_enter(varoff, S_Symbol(vd->v), globaloff);
    globaloff++;
  }
}

void transA_MethodDeclList_basic(FILE *out, S_table mtbl, S_symbol classid,
                                 A_methodDeclList mdl) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_MethodDeclList_basic...\n");
#endif
  if (!mdl) return;

  transA_MethodDecl_basic(out, mtbl, classid, mdl->head);

  if (mdl->tail) {
    transA_MethodDeclList_basic(out, mtbl, classid, mdl->tail);
  }
}

void transA_MethodDecl_basic(FILE *out, S_table mtbl, S_symbol classid,
                             A_methodDecl md) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_MethodDecl_basic...\n");
#endif
  if (!md) return;

  // check if the method is already declared
  if (S_look(mtbl, S_Symbol(md->id))) {
    transError(
        out, md->pos,
        Stringf("error: method already declared in class %s", curClassId));
  }

  curMethodId = md->id;

  // get return type
  Ty_ty ret = atype2tyty(md->t);

  // get formal list and check if there's redefined variables
  S_beginScope(venv);
  Ty_fieldList fl = transA_FormalList_basic(out, md->fl);
  S_endScope(venv);
  // reset temp counter
  Temp_resettemp();

  // enter the method into the environment
  S_enter(mtbl, S_Symbol(md->id),
          E_MethodEntry(md, S_Symbol(curClassId), ret, fl));

  // record the offset of the method
  if (offtable_look(methoff, S_Symbol(md->id)) < 0) {
    offtable_enter(methoff, S_Symbol(md->id), globaloff);
    globaloff++;
  }
}

Ty_fieldList transA_FormalList_basic(FILE *out, A_formalList fl) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_FormalList_basic...\n");
#endif
  if (!fl) return NULL;

  return Ty_FieldList(transA_Formal_basic(out, fl->head),
                      transA_FormalList_basic(out, fl->tail));
}

Ty_field transA_Formal_basic(FILE *out, A_formal f) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_Formal_basic with (line:%d col:%d)...\n",
          f->pos->line, f->pos->pos);
#endif
  if (!f) return NULL;

  E_enventry var = S_look(venv, S_Symbol(f->id));
  if (var) {
    transError(out, f->pos,
               Stringf("error: formal redefined in method %s, class %s",
                       curMethodId, curClassId));
  }
  S_enter(venv, S_Symbol(f->id),
          E_VarEntry(f2vd(f), atype2tyty(f->t),
                     Temp_newtemp(f->t->t == A_floatType ? T_float : T_int)));

  return f2tyf(f);
}

// trans extends class decl info
void transA_ClassDeclList_extend(FILE *out, A_classDeclList cdl) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_ClassDeclList_extend...\n");
#endif
  if (!cdl) return;

  E_enventry ce = S_look(cenv, S_Symbol(cdl->head->id));
  if (ce->u.cls.status == E_transInit) {
    curClassId = cdl->head->id;
    transA_ClassDecl_extend(out, cdl->head);
  }

  if (cdl->tail) {
    transA_ClassDeclList_extend(out, cdl->tail);
  }
}

void transA_ClassDecl_extend(FILE *out, A_classDecl cd) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_ClassDecl_extend with class %s...\n", cd->id);
#endif
  if (!cd) return;

  E_enventry ce = S_look(cenv, S_Symbol(cd->id));

  // FIND: this class is being processed
  ce->u.cls.status = E_transFind;

  if (cd->parentID) {
    E_enventry fa = S_look(cenv, ce->u.cls.fa);
    if (!fa) {
      transError(out, cd->pos,
                 Stringf("error: class %s's parent class %s not declared",
                         cd->id, cd->parentID));
    }

    // there's a cycle in inheritance
    if (fa->u.cls.status == E_transFind) {
      E_enventry curce = S_look(cenv, S_Symbol(curClassId));
      transError(
          out, curce->u.cls.cd->pos,
          Stringf("error: class %s has a cycle in inheritance", curClassId));
    }

    // parent class is not processed yet, process it first
    if (fa->u.cls.status == E_transInit) {
      transA_ClassDecl_extend(out, fa->u.cls.cd);
    }

    // copy parent class's vtbl and mtbl
    curClassId = cd->id;
    transA_ClassVtbl_copy(out, fa->u.cls.vtbl, ce->u.cls.vtbl);
    transA_ClassMtbl_copy(out, fa->u.cls.mtbl, ce->u.cls.mtbl);
  }

  // FILL: this class is processed
  ce->u.cls.status = E_transFill;
}

void transA_ClassVtbl_copy(FILE *out, S_table fa, S_table cur) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_ClassVtbl_copy with class %s...\n", curClassId);
#endif
  void *top;
  binder b;

  top = fa->top;
  while (top) {
    b = TAB_getBinder(fa, top);
    E_enventry var = TAB_look(cur, b->key);
    if (var) {
      transError(out, var->u.var.vd->pos,
                 Stringf("error: class %s has duplicate variable names with "
                         "parent class",
                         curClassId));
    }
    TAB_enter(cur, b->key, b->value);

    top = b->prevtop;
  }
}

void transA_ClassMtbl_copy(FILE *out, S_table fa, S_table cur) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_ClassMtbl_copy with class %s...\n", curClassId);
#endif
  void *top;
  binder b;

  top = fa->top;
  while (top) {
    b = TAB_getBinder(fa, top);
    E_enventry meth = TAB_look(cur, b->key);
    if (meth) {
      // check if signatures are equal
      E_enventry fa_meth = b->value;
      if (!equalClassMethSignature(fa_meth, meth)) {
        transError(out, meth->u.meth.md->pos,
                   Stringf("error: class %s has method %s with different "
                           "signature with parent class",
                           curClassId, meth->u.meth.md->id));
      }
    } else {
      TAB_enter(cur, b->key, b->value);
    }

    top = b->prevtop;
  }
}

/* semantic analysis */

T_funcDeclList transSemant(FILE *out, A_classDeclList cdl, A_mainMethod m) {
#ifdef __DEBUG
  fprintf(out, "Entering transSemant...\n");
#endif
  T_funcDeclList classes = transA_ClassDeclList(out, cdl);
  T_funcDecl mainmethod = transA_MainMethod(out, m);
  return Tr_FuncDeclList(mainmethod, classes);
}

// classes
T_funcDeclList transA_ClassDeclList(FILE *out, A_classDeclList cdl) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_ClassDeclList...\n");
#endif
  if (!cdl) {
    return NULL;
  }

  T_funcDeclList cd = transA_ClassDecl(out, cdl->head);
  T_funcDeclList rest = transA_ClassDeclList(out, cdl->tail);

  return Tr_ChainFuncDeclList(cd, rest);
}

T_funcDeclList transA_ClassDecl(FILE *out, A_classDecl cd) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_ClassDecl with class %s...\n", cd->id);
#endif
  if (!cd) {
    return NULL;
  }

  curClassId = cd->id;

  E_enventry ce = S_look(cenv, S_Symbol(cd->id));

  // init class variables
  if (cd->vdl) {
    transA_ClassVarDeclList(out, ce->u.cls.vtbl, cd->vdl);
  }

  // init class methods
  T_funcDeclList methods = transA_MethodDeclList(out, ce->u.cls.mtbl, cd->mdl);

  return methods;
}

void transA_ClassVarDeclList(FILE *out, S_table vtbl, A_varDeclList vdl) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_ClassVarDeclList...\n");
#endif
  if (!vdl) return;

  transA_ClassVarDecl(out, vtbl, vdl->head);

  if (vdl->tail) {
    transA_ClassVarDeclList(out, vtbl, vdl->tail);
  }
}

void transA_ClassVarDecl(FILE *out, S_table vtbl, A_varDecl vd) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_ClassVarDecl...\n");
#endif
  if (!vd) return;

  if (vd->t->t == A_idType) {
    E_enventry x = S_look(cenv, S_Symbol(vd->t->id));
    if (!x) {
      transError(out, vd->pos, String("error: variable type not declared"));
    }
  }
}

T_funcDeclList transA_MethodDeclList(FILE *out, S_table mtbl,
                                     A_methodDeclList mdl) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_MethodDeclList...\n");
#endif
  if (!mdl) {
    return NULL;
  }

  T_funcDecl meth = transA_MethodDecl(out, mtbl, mdl->head);
  T_funcDeclList rest = transA_MethodDeclList(out, mtbl, mdl->tail);

  return Tr_FuncDeclList(meth, rest);
}

T_funcDecl transA_MethodDecl(FILE *out, S_table mtbl, A_methodDecl md) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_MethodDecl...\n");
#endif
  if (!md) {
    return NULL;
  }

  curMethodId = md->id;

  if (md->t->t == A_idType) {
    E_enventry x = S_look(cenv, S_Symbol(md->t->id));
    if (!x) {
      transError(
          out, md->pos,
          Stringf("error: method %s's return type not declared in class %s",
                  curMethodId, curClassId));
    }
  }

  E_enventry meth = S_look(mtbl, S_Symbol(md->id));

  S_beginScope(venv);

  // check if the formal list is declared, and add them to the environment
  Temp_tempList fl = transA_FormalList(out, meth->u.meth.fl, md->fl, md->pos);

  // get method body
  Tr_exp vdl = transA_VarDeclList(out, md->vdl);
  Tr_exp sl = transA_StmList(out, md->sl);

  S_endScope(venv);
  // Temp_resettemp();

  return Tr_ClassMethod(S_name(S_link(S_Symbol(curClassId), S_Symbol(md->id))),
                        fl, vdl, sl);
}

Temp_tempList transA_FormalList(FILE *out, Ty_fieldList fieldList,
                                A_formalList fl, A_pos pos) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_FormalList with method %s...\n",
          S_name(S_link(S_Symbol(curClassId), S_Symbol(curMethodId))));
#endif
  if (!fl) {
    return NULL;
  }

  Temp_temp curTemp = transA_Formal(out, fieldList->head, fl->head);
  Temp_tempList rest = transA_FormalList(out, fieldList->tail, fl->tail, pos);
  return Temp_TempList(curTemp, rest);
}

Temp_temp transA_Formal(FILE *out, Ty_field field, A_formal f) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_Formal...\n");
#endif
  if (!f) {
    return NULL;
  }

  switch (f->t->t) {
    case A_intType: {
      Temp_temp temp = Temp_newtemp(T_int);
      S_enter(venv, S_Symbol(f->id), E_VarEntry(f2vd(f), Ty_Int(), temp));
      return temp;
    }
    case A_floatType: {
      Temp_temp temp = Temp_newtemp(T_float);
      S_enter(venv, S_Symbol(f->id), E_VarEntry(f2vd(f), Ty_Float(), temp));
      return temp;
    }
    case A_intArrType: {
      Temp_temp temp = Temp_newtemp(T_int);
      S_enter(venv, S_Symbol(f->id),
              E_VarEntry(f2vd(f), Ty_Array(Ty_Int()), temp));
      return temp;
    }
    case A_floatArrType: {
      Temp_temp temp = Temp_newtemp(T_int);
      S_enter(venv, S_Symbol(f->id),
              E_VarEntry(f2vd(f), Ty_Array(Ty_Float()), temp));
      return temp;
    }
    case A_idType: {
      if (!S_look(cenv, S_Symbol(f->t->id))) {
        transError(out, f->pos, String("error: variable type not declared"));
      }
      Temp_temp temp = Temp_newtemp(T_int);
      S_enter(venv, S_Symbol(f->id),
              E_VarEntry(f2vd(f), Ty_Name(S_Symbol(f->t->id)), temp));
      return temp;
    }
    default:
      return NULL;  // unreachable
  }
}

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
    if (S_Symbol(curClassId) == MAIN_CLASS) {
      transError(out, vd->pos,
                 String("error: variable already declared in main method"));
    } else {
      transError(
          out, vd->pos,
          Stringf("error: variable already declared in method %s, class %s",
                  curClassId, curMethodId));
    }
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
    case A_intArrType: {
      temp = Temp_newtemp(T_int);
      S_enter(venv, S_Symbol(vd->v), E_VarEntry(vd, Ty_Array(Ty_Int()), temp));
      break;
    }
    case A_floatArrType: {
      temp = Temp_newtemp(T_int);
      S_enter(venv, S_Symbol(vd->v),
              E_VarEntry(vd, Ty_Array(Ty_Float()), temp));
      break;
    }
    case A_idType: {
      if (!S_look(cenv, S_Symbol(vd->t->id))) {
        transError(out, vd->pos, String("error: variable type not declared"));
      }
      temp = Temp_newtemp(T_int);
      S_enter(venv, S_Symbol(vd->v),
              E_VarEntry(vd, Ty_Name(S_Symbol(vd->t->id)), temp));
      break;
    }
    default:
      break;  // unreachable
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
      case A_intArrType:
        return Tr_ArrayInit(Tr_IdExp(temp),
                            transA_ExpList_NumConst(out, vd->elist, Ty_Int()),
                            T_int);
      case A_floatArrType:
        return Tr_ArrayInit(Tr_IdExp(temp),
                            transA_ExpList_NumConst(out, vd->elist, Ty_Float()),
                            T_float);
      default:
        return NULL;  // unreachable
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

  curClassId = S_name(MAIN_CLASS);

  Tr_exp vdl = transA_VarDeclList(out, main->vdl);
  Tr_exp sl = transA_StmList(out, main->sl);

  S_endScope(venv);
  // Temp_resettemp();

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

  expty right = transA_Exp(out, s->u.assign.value, NULL);
  if (!right) {
    return NULL;
  }
  // check if the types match
  if (!equalTyCast(left->location, right->value)) {
    transError(
        out, s->pos,
        Stringf("error: right side of assignment expected '%s', got '%s'",
                ty2str(left->location), ty2str(right->value)));
  }

  return Tr_AssignStm(left->exp, right->exp);
}

Tr_exp transA_ArrayInit(FILE *out, A_stm s) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_ArrayInit...\n");
#endif
  if (!s) {
    return NULL;
  }

  expty arr = transA_Exp(out, s->u.array_init.arr, NULL);
  if (!arr) {
    return NULL;
  }
  if (!arr->location) {
    transError(out, s->pos,
               String("error: left side of array initialization must be a "
                      "location value"));
  }
  if (arr->location->kind != Ty_array) {
    transError(out, s->pos,
               String("error: left side of array initialization must be an "
                      "array type"));
  }

  Tr_expList initExp = transA_ExpList_Num(out, s->u.array_init.init_values,
                                          arr->location->u.array);
  if (!initExp) {
    return NULL;
  }

  T_type type;
  switch (arr->location->u.array->kind) {
    case Ty_int:
      type = T_int;
      break;
    case Ty_float:
      type = T_float;
      break;
    default:
      return NULL;  // unreachable
  }
  return Tr_ArrayInit(arr->exp, initExp, type);
}

Tr_exp transA_CallStm(FILE *out, A_stm s) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_CallStm...\n");
#endif
  if (!s) {
    return NULL;
  }

  expty obj = transA_Exp(out, s->u.call_stat.obj, NULL);
  if (!obj) {
    return NULL;
  }
  if (obj->value->kind != Ty_name) {
    transError(out, s->pos,
               String("error: call statement must be called on an object"));
  }

  E_enventry ce = S_look(cenv, obj->value->u.name);
  S_table mtbl = ce->u.cls.mtbl;
  E_enventry me = S_look(mtbl, S_Symbol(s->u.call_stat.fun));
  if (!me) {
    transError(out, s->pos,
               Stringf("error: class %s has no method %s",
                       S_name(obj->value->u.name), s->u.call_stat.fun));
  }

  // get the arguments of the method
  Tr_expList expl =
      transA_ExpList_Call(out, me->u.meth.fl, s->u.call_stat.el, s->pos);

  // get the return type of the method
  T_type ret = me->u.meth.ret->kind == Ty_float ? T_float : T_int;

  // get the offset of the method
  int offset = offtable_look(methoff, S_Symbol(s->u.call_stat.fun));
  Tr_exp methAddr = Tr_ClassMethExp(obj->exp, offset);

  return Tr_CallStm(s->u.call_stat.fun, methAddr, obj->exp, expl, ret);
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

  expty ret = transA_Exp(out, s->u.e, NULL);
  if (!ret) {
    return NULL;
  }

  if (S_Symbol(curClassId) == MAIN_CLASS) {
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
  } else {
    E_enventry ce = S_look(cenv, S_Symbol(curClassId));
    S_table mtbl = ce->u.cls.mtbl;
    E_enventry me = S_look(mtbl, S_Symbol(curMethodId));
    if (!equalTyCast(me->u.meth.ret, ret->value)) {
      transError(out, s->pos,
                 Stringf("error: return type expected '%s', got '%s'",
                         ty2str(me->u.meth.ret), ty2str(ret->value)));
    }
    if (me->u.meth.ret->kind == Ty_int && ret->value->kind == Ty_float) {
      return Tr_Return(Tr_Cast(ret->exp, T_int));
    }
    if (me->u.meth.ret->kind == Ty_float && ret->value->kind == Ty_int) {
      return Tr_Return(Tr_Cast(ret->exp, T_float));
    }
    return Tr_Return(ret->exp);
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
  if (!s) {
    return NULL;
  }

  expty pos = transA_Exp(out, s->u.putarray.e1, NULL);
  if (!pos) {
    return NULL;
  }
  if (pos->value->kind != Ty_int && pos->value->kind != Ty_float) {
    transError(out, s->pos,
               String("error: first argument of putarray() must be of type int "
                      "or float"));
  }

  expty array = transA_Exp(out, s->u.putarray.e2, NULL);
  if (!array) {
    return NULL;
  }
  if (array->value->kind != Ty_array) {
    transError(
        out, s->pos,
        String("error: second argument of putarray() must be of type array"));
  }

  if (pos->value->kind == Ty_float) {
    pos->exp = Tr_Cast(pos->exp, T_int);
  }

  return Tr_Putarray(pos->exp, array->exp);
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
Tr_expList transA_ExpList_NumConst(FILE *out, A_expList el, Ty_ty type) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_ExpList_NumConst...\n");
#endif
  if (!el) {
    return NULL;
  }

  Tr_exp exp = transA_Exp_NumConst(out, el->head, type);
  Tr_expList rest = transA_ExpList_NumConst(out, el->tail, type);

  return Tr_ExpList(exp, rest);
}

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

Tr_expList transA_ExpList_Num(FILE *out, A_expList el, Ty_ty type) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_ExpList_Num...\n");
#endif
  if (!el) {
    return NULL;
  }

  Tr_exp exp = transA_Exp_Num(out, el->head, type);
  Tr_expList rest = transA_ExpList_Num(out, el->tail, type);

  return Tr_ExpList(exp, rest);
}

Tr_exp transA_Exp_Num(FILE *out, A_exp e, Ty_ty type) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_Exp_Num...\n");
#endif
  if (!e) {
    return NULL;
  }

  expty num = transA_Exp(out, e, type);
  if (!num) {
    return NULL;
  }
  if (num->value->kind != Ty_int && num->value->kind != Ty_float) {
    transError(out, e->pos,
               String("error: array initialization must be of type int or "
                      "float"));
  }

  if (num->value->kind == type->kind) {
    return num->exp;
  }
  // need to cast
  switch (type->kind) {
    case Ty_int:
      return Tr_Cast(num->exp, T_int);
    case Ty_float:
      return Tr_Cast(num->exp, T_float);
    default:
      return NULL;  // unreachable
  }
}

Tr_expList transA_ExpList_Call(FILE *out, Ty_fieldList fieldList, A_expList el,
                               A_pos pos) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_ExpList_Call...\n");
#endif
  if (!fieldList && !el) {
    return NULL;
  }
  if (!fieldList) {
    transError(out, pos, String("error: too many arguments in method call"));
  }
  if (!el) {
    transError(out, pos, String("error: too few arguments in method call"));
  }

  Tr_exp exp = transA_Exp_Call(out, fieldList->head, el->head);
  Tr_expList rest = transA_ExpList_Call(out, fieldList->tail, el->tail, pos);
  return Tr_ExpList(exp, rest);
}

Tr_exp transA_Exp_Call(FILE *out, Ty_field field, A_exp e) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_Exp_Call...\n");
#endif
  expty exp = transA_Exp(out, e, NULL);
  if (!exp) {
    transError(out, e->pos, String("error: method call argument is invalid"));
  }
  if (!equalTyCast(field->ty, exp->value)) {
    transError(out, e->pos,
               String("error: method call argument type does not match"));
  }

  // need to cast
  if (field->ty->kind == Ty_int && exp->value->kind == Ty_float) {
    exp->exp = Tr_Cast(exp->exp, T_int);
  }
  if (field->ty->kind == Ty_float && exp->value->kind == Ty_int) {
    exp->exp = Tr_Cast(exp->exp, T_float);
  }

  return exp->exp;
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
  if (!e) {
    return NULL;
  }

  expty array = transA_Exp(out, e->u.array_pos.arr, NULL);
  if (!array) {
    return NULL;
  }
  if (array->value->kind != Ty_array) {
    transError(out, e->pos,
               String("error: left side of array access must be an array"));
  }

  expty pos = transA_Exp(out, e->u.array_pos.arr_pos, Ty_Int());
  if (!pos) {
    return NULL;
  }
  if (pos->value->kind != Ty_int && pos->value->kind != Ty_float) {
    transError(
        out, e->pos,
        String(
            "error: right side of array access must be of type int or float"));
  }
  if (pos->value->kind == Ty_float) {
    pos->exp = Tr_Cast(pos->exp, T_int);
  }

  T_type type;
  switch (array->value->u.array->kind) {
    case Ty_int:
      type = T_int;
      break;
    case Ty_float:
      type = T_float;
      break;
    default:
      return NULL;  // unreachable
  }

  return ExpTy(Tr_ArrayExp(array->exp, pos->exp, type), array->value->u.array,
               array->value->u.array);
}

expty transA_CallExp(FILE *out, A_exp e) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_CallExp...\n");
#endif
  if (!e) {
    return NULL;
  }
  expty obj = transA_Exp(out, e->u.call.obj, NULL);
  if (!obj) {
    return NULL;
  }
  if (obj->value->kind != Ty_name) {
    transError(out, e->pos,
               String("error: call expression must be called on an object"));
  }

  E_enventry ce = S_look(cenv, obj->value->u.name);
  S_table mtbl = ce->u.cls.mtbl;
  E_enventry me = S_look(mtbl, S_Symbol(e->u.call.fun));
  if (!me) {
    transError(out, e->pos,
               Stringf("error: class %s has no method %s",
                       S_name(obj->value->u.name), e->u.call.fun));
  }

  // get the arguments of the method
  Tr_expList expl =
      transA_ExpList_Call(out, me->u.meth.fl, e->u.call.el, e->pos);

  // get the return type of the method
  T_type ret = me->u.meth.ret->kind == Ty_float ? T_float : T_int;

  // get the offset of the method
  int offset = offtable_look(methoff, S_Symbol(e->u.call.fun));
  Tr_exp methAddr = Tr_ClassMethExp(obj->exp, offset);

  return ExpTy(Tr_CallExp(e->u.call.fun, methAddr, obj->exp, expl, ret),
               me->u.meth.ret, NULL);
}

expty transA_ClassVarExp(FILE *out, A_exp e) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_ClassVarExp...\n");
#endif
  if (!e) {
    return NULL;
  }

  expty obj = transA_Exp(out, e->u.classvar.obj, NULL);
  if (!obj) {
    return NULL;
  }
  if (obj->value->kind != Ty_name) {
    transError(out, e->pos,
               String("error: class variable must be accessed on an object"));
  }

  E_enventry ce = S_look(cenv, obj->value->u.name);
  S_table vtbl = ce->u.cls.vtbl;
  E_enventry ve = S_look(vtbl, S_Symbol(e->u.classvar.var));
  if (!ve) {
    transError(out, e->pos,
               Stringf("Error: Class %s has no variable %s",
                       S_name(obj->value->u.name), e->u.classvar.var));
  }

  int offset = offtable_look(varoff, S_Symbol(e->u.classvar.var));
  T_type type = ve->u.var.ty->kind == Ty_float ? T_float : T_int;
  return ExpTy(Tr_ClassVarExp(obj->exp, offset, type), ve->u.var.ty,
               ve->u.var.ty);
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
  if (!e) {
    return NULL;
  }

  expty array = transA_Exp(out, e->u.e, NULL);
  if (!array) {
    return NULL;
  }
  if (array->value->kind != Ty_array) {
    transError(out, e->pos,
               String("error: argument of length() must be an array"));
  }

  return ExpTy(Tr_LengthExp(array->exp), Ty_Int(), NULL);
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
  if (!e) {
    return NULL;
  }

  if (S_Symbol(curClassId) == MAIN_CLASS) {
    transError(out, e->pos,
               String("error: 'this' cannot be used in main method"));
  }

  return ExpTy(Tr_ThisExp(this()), Ty_Name(S_Symbol(curClassId)),
               Ty_Name(S_Symbol(curClassId)));
}

expty transA_NewIntArrExp(FILE *out, A_exp e) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_NewIntArrExp...\n");
#endif
  if (!e) {
    return NULL;
  }

  expty size = transA_Exp(out, e->u.e, NULL);
  if (!size) {
    return NULL;
  }
  if (size->value->kind != Ty_int && size->value->kind != Ty_float) {
    transError(out, e->pos,
               String("error: argument of new int array must be of type int"));
  }
  if (size->value->kind == Ty_float) {
    size->exp = Tr_Cast(size->exp, T_int);
  }

  return ExpTy(Tr_NewArrExp(size->exp), Ty_Array(Ty_Int()), NULL);
}

expty transA_NewFloatArrExp(FILE *out, A_exp e) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_NewFloatArrExp...\n");
#endif
  if (!e) {
    return NULL;
  }

  expty size = transA_Exp(out, e->u.e, NULL);
  if (!size) {
    return NULL;
  }
  if (size->value->kind != Ty_int && size->value->kind != Ty_float) {
    transError(out, e->pos,
               String("error: argument of new float array must be of type "
                      "int or float"));
  }
  if (size->value->kind == Ty_float) {
    size->exp = Tr_Cast(size->exp, T_int);
  }

  return ExpTy(Tr_NewArrExp(size->exp), Ty_Array(Ty_Float()), NULL);
}

expty transA_NewObjExp(FILE *out, A_exp e) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_NewObjExp...\n");
#endif
  if (!e) {
    return NULL;
  }

  E_enventry ce = S_look(cenv, S_Symbol(e->u.v));
  if (!ce) {
    transError(out, e->pos, String("error: class not declared"));
  }

  Tr_exp tmpobj = Tr_NewObjTemp(Temp_newtemp(T_int));
  Tr_exp allocSpace = Tr_NewObjAlloc(tmpobj, globaloff);

  Tr_exp newObjStm = allocSpace;

  // initialize the class vars
  newObjStm = transA_NewObjClassVar(out, ce->u.cls.vtbl, tmpobj, newObjStm);

  // initialize the class meths
  newObjStm = transA_NewObjClassMeth(out, ce->u.cls.mtbl, tmpobj, newObjStm);

  Tr_exp newObjFinal = Tr_EscExp(newObjStm, tmpobj);

  return ExpTy(newObjFinal, Ty_Name(S_Symbol(e->u.v)), NULL);
}

Tr_exp transA_NewObjClassVar(FILE *out, S_table vtbl, Tr_exp tmpobj,
                             Tr_exp newObjStm) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_NewObjClassVar...\n");
#endif
  void *vtop = vtbl->top;
  binder vb;
  while (vtop) {
    vb = TAB_getBinder(vtbl, vtop);
    E_enventry ve = vb->value;
    A_varDecl vd = ve->u.var.vd;

    if (vd->elist) {
      // get offset of the class variable
      int offset = offtable_look(varoff, vb->key);
      // get address of the class variable
      T_type type = ve->u.var.ty->kind == Ty_float ? T_float : T_int;
      Tr_exp varAddr = Tr_ClassVarExp(tmpobj, offset, type);

      switch (vd->t->t) {
        case A_intType: {
          newObjStm = Tr_StmList(
              newObjStm,
              Tr_AssignStm(varAddr, transA_Exp_NumConst(out, vd->elist->head,
                                                        Ty_Int())));
          break;
        }
        case A_floatType: {
          newObjStm = Tr_StmList(
              newObjStm,
              Tr_AssignStm(varAddr, transA_Exp_NumConst(out, vd->elist->head,
                                                        Ty_Float())));
          break;
        }
        case A_intArrType: {
          newObjStm = Tr_StmList(
              newObjStm,
              Tr_ArrayInit(varAddr,
                           transA_ExpList_NumConst(out, vd->elist, Ty_Int()),
                           T_int));
          break;
        }
        case A_floatArrType: {
          newObjStm = Tr_StmList(
              newObjStm,
              Tr_ArrayInit(varAddr,
                           transA_ExpList_NumConst(out, vd->elist, Ty_Float()),
                           T_float));
          break;
        }
        default:
          break;  // unreachable
      }
    }

    vtop = vb->prevtop;
  }
  
  return newObjStm;
}

Tr_exp transA_NewObjClassMeth(FILE *out, S_table mtbl, Tr_exp tmpobj, Tr_exp newObjStm) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_NewObjClassMeth...\n");
#endif
  void *mtop = mtbl->top;
  binder mb;
  while (mtop) {
    mb = TAB_getBinder(mtbl, mtop);
    E_enventry meth = mb->value;

    // get label of the method
    S_symbol from = meth->u.meth.from;
    S_symbol name = mb->key;
    Tr_exp methLabel = Tr_ClassMethLabel(Temp_namedlabel(S_name(S_link(from, name))));

    // get offset of the method
    int offset = offtable_look(methoff, mb->key);
    // get address of the method
    Tr_exp methAddr = Tr_ClassMethExp(tmpobj, offset);

    newObjStm = Tr_StmList(newObjStm, Tr_AssignStm(methAddr, methLabel));

    mtop = mb->prevtop;
  }

  return newObjStm;
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
  if (!e) {
    return NULL;
  }

  expty array = transA_Exp(out, e->u.e, NULL);
  if (!array) {
    return NULL;
  }
  if (array->value->kind != Ty_array) {
    transError(out, e->pos,
               String("error: argument of getarray() must be an array"));
  }

  switch (array->value->u.array->kind) {
    case Ty_int:
      return ExpTy(Tr_Getarray(array->exp), Ty_Int(), NULL);
    case Ty_float:
      return ExpTy(Tr_Getfarray(array->exp), Ty_Int(), NULL);
    default:
      return NULL;  // unreachable
  }
}

static char buf[IR_MAXLEN];
static string Stringf(char *fmt, ...) {
  va_list argp;
  va_start(argp, fmt);
  vsnprintf(buf, IR_MAXLEN, fmt, argp);
  va_end(argp);
  return String(buf);
}

static void offtable_enter(S_table t, S_symbol key, int off) {
  int *p = checked_malloc(sizeof(int));
  *p = off;
  S_enter(t, key, p);
}

static int offtable_look(S_table t, S_symbol key) {
  int *p = S_look(t, key);
  if (!p) {
    return -1;
  }
  return *p;
}

static A_varDecl f2vd(A_formal f) {
  if (!f) return NULL;

  return A_VarDecl(f->pos, f->t, f->id, NULL);
}

static Ty_ty atype2tyty(A_type t) {
  if (!t) return NULL;

  switch (t->t) {
    case A_intType:
      return Ty_Int();
    case A_floatType:
      return Ty_Float();
    case A_intArrType:
      return Ty_Array(Ty_Int());
    case A_floatArrType:
      return Ty_Array(Ty_Float());
    case A_idType:
      return Ty_Name(S_Symbol(t->id));
    default:
      return NULL;  // unreachable
  }
}

static Ty_field f2tyf(A_formal f) {
  if (!f) return NULL;

  return Ty_Field(S_Symbol(f->id), atype2tyty(f->t));
}

static bool equalTy(Ty_ty fa, Ty_ty cur) {
  if (!fa || !cur) {
    return FALSE;
  }

  if (fa->kind != cur->kind) {
    return FALSE;
  }
  if (fa->kind == Ty_array) {
    return equalTy(fa->u.array, cur->u.array);
  }
  if (fa->kind == Ty_name) {
    return fa->u.name == cur->u.name;
  }
  return TRUE;
}

static bool equalClassMethSignature(E_enventry fa, E_enventry cur) {
  if (!fa || !cur) {
    return FALSE;
  }

  // check if return types are equal
  if (!equalTy(fa->u.meth.ret, cur->u.meth.ret)) {
    return FALSE;
  }

  // check if formal lists are equal
  Ty_fieldList fa_fl = fa->u.meth.fl;
  Ty_fieldList cur_fl = cur->u.meth.fl;
  while (fa_fl && cur_fl) {
    if (!equalTy(fa_fl->head->ty, cur_fl->head->ty)) {
      return FALSE;
    }
    fa_fl = fa_fl->tail;
    cur_fl = cur_fl->tail;
  }
  if (fa_fl || cur_fl) {
    return FALSE;
  }
  return TRUE;
}

static bool isParentClass(Ty_ty left, Ty_ty right) {
  // check if the class of the right side is a subclass of the left side
  S_symbol leftClass = left->u.name;
  S_symbol rightClass = right->u.name;
  while (rightClass != MAIN_CLASS) {
    if (leftClass == rightClass) {
      return TRUE;
    }
    E_enventry ce = S_look(cenv, rightClass);
    rightClass = ce->u.cls.fa;
  }
  return FALSE;
}

static bool equalTyCast(Ty_ty param, Ty_ty arg) {
  if (!param || !arg) {
    return FALSE;
  }

  if (param->kind == Ty_int || param->kind == Ty_float) {
    return arg->kind == Ty_int || arg->kind == Ty_float;
  }
  if (param->kind == Ty_array) {
    return arg->kind == Ty_array && equalTy(param->u.array, arg->u.array);
  }
  if (param->kind == Ty_name) {
    return arg->kind == Ty_name && isParentClass(param, arg);
  }
}

static string ty2str(Ty_ty t) {
  if (!t) return NULL;

  switch (t->kind) {
    case Ty_int:
      return String("int");
    case Ty_float:
      return String("float");
    case Ty_array:
      return Stringf("array of %s", ty2str(t->u.array));
    case Ty_name:
      return Stringf("class %s", S_name(t->u.name));
    default:
      return NULL;  // unreachable
  }
}
