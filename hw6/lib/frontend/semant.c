#include "semant.h"

#include <stdlib.h>

#include "symbol.h"
#include "table.h"

#define __DEBUG
#undef __DEBUG

typedef struct expty_* expty;
struct expty_ {
  bool location;
  Ty_ty ty;
};
expty Expty(bool location, Ty_ty ty) {
  expty e = checked_malloc(sizeof(*e));
  e->location = location;
  e->ty = ty;
  return e;
}

static S_table venv;  // for main method
static int whileDepth = 0;

static S_table cenv;
static S_symbol MAIN_CLASS;
static string curClassId;
static string curMethodId;

void transA_ClassDeclList(FILE* out, A_classDeclList cdl);

void transA_ClassDeclListPreprocess(FILE* out, A_classDeclList cdl);
void transA_ClassDeclListCenvInit(FILE* out, A_classDeclList cdl);
void transA_ClassDeclCenvInit(FILE* out, A_classDecl cd);
void transA_ClassDeclListCycleDetect(FILE* out, A_classDeclList cdl);
void transA_ClassDeclCycleDetect(FILE* out, A_classDecl cd);

void transA_ClassVtblCopyFromFa(FILE* out, S_table fa, S_table cur);
void transA_ClassMtblCopyFromFa(FILE* out, S_table fa, S_table cur);

void transA_ClassDeclListTypeChecking(FILE* out, A_classDeclList cdl);
void transA_ClassDeclTypeChecking(FILE* out, A_classDecl cd);
void transA_ClassVtblCopyToVenv(FILE* out);

void transA_MethodDeclListCenvInit(FILE* out, A_methodDeclList mdl,
                                   S_table env);
void transA_MethodDeclListClassMethods(FILE* out, A_methodDeclList mdl);

void transA_MethodDeclCenvInit(FILE* out, A_methodDecl md, S_table env);
void transA_MethodDeclClassMethods(FILE* out, A_methodDecl md);

void transA_FormalList(FILE* out, A_formalList fl);
void transA_Formal(FILE* out, A_formal f);

void transA_MainMethod(FILE* out, A_mainMethod m);

void transA_VarDeclListCenvInit(FILE* out, A_varDeclList vdl, S_table env);
void transA_VarDeclListClassVars(FILE* out, A_varDeclList vdl);
void transA_VarDeclListClassMethods(FILE* out, A_varDeclList vdl);
void transA_VarDeclListMainMethod(FILE* out, A_varDeclList vdl);

void transA_VarDeclCenvInit(FILE* out, A_varDecl vd, S_table env);
void transA_VarDeclClassVars(FILE* out, A_varDecl vd);
void transA_VarDeclClassMethods(FILE* out, A_varDecl vd);
void transA_VarDeclMainMethod(FILE* out, A_varDecl vd);

void transA_StmList(FILE* out, A_stmList sl);
void transA_Stm(FILE* out, A_stm s);

void transA_NestedStm(FILE* out, A_stm s);
void transA_IfStm(FILE* out, A_stm s);
void transA_WhileStm(FILE* out, A_stm s);
void transA_AssignStm(FILE* out, A_stm s);
void transA_ArrayInit(FILE* out, A_stm s);
void transA_CallStm(FILE* out, A_stm s);
void transA_Continue(FILE* out, A_stm s);
void transA_Break(FILE* out, A_stm s);
void transA_Return(FILE* out, A_stm s);
void transA_Putnum(FILE* out, A_stm s);
void transA_Putarray(FILE* out, A_stm s);
void transA_Putch(FILE* out, A_stm s);

void transA_ArrayInitExpList(FILE* out, A_expList el);
void transA_CallExpList(FILE* out, A_expList el, Ty_fieldList fl);

expty transA_Exp(FILE* out, A_exp e);
expty transA_OpExp(FILE* out, A_exp e);
expty transA_ArrayExp(FILE* out, A_exp e);
expty transA_CallExp(FILE* out, A_exp e);
expty transA_ClassVarExp(FILE* out, A_exp e);
expty transA_BoolConst(FILE* out, A_exp e);
expty transA_NumConst(FILE* out, A_exp e);
expty transA_IdExp(FILE* out, A_exp e);
expty transA_ThisExp(FILE* out, A_exp e);
expty transA_LengthExp(FILE* out, A_exp e);
expty transA_NewIntArrExp(FILE* out, A_exp e);
expty transA_NewFloatArrExp(FILE* out, A_exp e);
expty transA_NewObjExp(FILE* out, A_exp e);
expty transA_NotExp(FILE* out, A_exp e);
expty transA_MinusExp(FILE* out, A_exp e);
expty transA_EscExp(FILE* out, A_exp e);
expty transA_Getnum(FILE* out, A_exp e);
expty transA_Getch(FILE* out, A_exp e);
expty transA_Getarray(FILE* out, A_exp e);

bool equalClassMethSignature(E_enventry fa, E_enventry cur);
bool equalTy(Ty_ty fa, Ty_ty cur);
bool equalTyCast(Ty_ty param, Ty_ty arg);
bool isParentClass(Ty_ty left, Ty_ty right);
Ty_ty atype2tyty(A_type t);
Ty_field f2tyf(A_formal f);
Ty_fieldList fl2tyfl(A_formalList fl);
string ty2str(Ty_ty t);

void transError(FILE* out, A_pos pos, string msg) {
  fprintf(out, "(line:%d col:%d) %s\n", pos->line, pos->pos, msg);
  fflush(out);
  exit(1);
}

void transA_Prog(FILE* out, A_prog p) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_Prog...\n");
#endif
  // initialize env
  venv = S_empty();
  cenv = S_empty();
  MAIN_CLASS = S_Symbol(String("0Main"));

  if (p->cdl) {
    transA_ClassDeclList(out, p->cdl);
  }

  if (p->m) {
    transA_MainMethod(out, p->m);
  } else {
    transError(out, p->pos, String("error: there's no main class"));
  }
}

void transA_ClassDeclList(FILE* out, A_classDeclList cdl) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_ClassDeclList...\n");
#endif
  if (!cdl) return;

  transA_ClassDeclListPreprocess(out, cdl);

  transA_ClassDeclListTypeChecking(out, cdl);
}

void transA_ClassDeclListPreprocess(FILE* out, A_classDeclList cdl) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_ClassDeclListPreprocess...\n");
#endif
  if (!cdl) return;

  // first pass: record class names and inheritance
  transA_ClassDeclListCenvInit(out, cdl);

  // second pass: detect cycles in inheritance
  transA_ClassDeclListCycleDetect(out, cdl);
}

void transA_ClassDeclListCenvInit(FILE* out, A_classDeclList cdl) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_ClassDeclListCenvInit...\n");
#endif
  if (!cdl) return;

  transA_ClassDeclCenvInit(out, cdl->head);

  if (cdl->tail) {
    transA_ClassDeclListCenvInit(out, cdl->tail);
  }
}

void transA_ClassDeclCenvInit(FILE* out, A_classDecl cd) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_ClassDeclCenvInit with class %s...\n", cd->id);
#endif
  if (!cd) return;

  if (S_look(cenv, S_Symbol(cd->id)) != NULL) {
    transError(out, cd->pos, String("error: class already declared"));
  }

  S_symbol fa = cd->parentID ? S_Symbol(cd->parentID) : MAIN_CLASS;

  curClassId = cd->id;

  // init class variables
  S_table vtbl = S_empty();
  transA_VarDeclListCenvInit(out, cd->vdl, vtbl);

  // init class methods
  S_table mtbl = S_empty();
  transA_MethodDeclListCenvInit(out, cd->mdl, mtbl);

  S_enter(cenv, S_Symbol(cd->id),
          E_ClassEntry(cd, fa, E_transInit, vtbl, mtbl));
}

void transA_ClassDeclListCycleDetect(FILE* out, A_classDeclList cdl) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_ClassDeclListCycleDetect...\n");
#endif
  if (!cdl) return;

  E_enventry ce = S_look(cenv, S_Symbol(cdl->head->id));
  if (ce->u.cls.status == E_transInit) {
    transA_ClassDeclCycleDetect(out, cdl->head);
  }

  if (cdl->tail) {
    transA_ClassDeclListCycleDetect(out, cdl->tail);
  }
}

void transA_ClassDeclCycleDetect(FILE* out, A_classDecl cd) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_ClassDeclCycleDetect with class %s...\n",
          cd->id);
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
      transError(out, cd->pos,
                 Stringf("error: class %s has a cycle in inheritance", cd->id));
    }

    // parent class is not processed yet, process it first
    if (fa->u.cls.status == E_transInit) {
      transA_ClassDeclCycleDetect(out, fa->u.cls.cd);
    }

    // copy parent class's vtbl and mtbl
    curClassId = cd->id;
    transA_ClassVtblCopyFromFa(out, fa->u.cls.vtbl, ce->u.cls.vtbl);
    transA_ClassMtblCopyFromFa(out, fa->u.cls.mtbl, ce->u.cls.mtbl);
  }

  // FILL: this class is processed
  ce->u.cls.status = E_transFill;
}

void transA_ClassVtblCopyFromFa(FILE* out, S_table fa, S_table cur) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_ClassVtblCopyFromFa with class %s...\n",
          curClassId);
#endif
  int i;
  binder b;
  for (i = 0; i < TABSIZE; i++) {
    for (b = fa->table[i]; b; b = b->next) {
      E_enventry var = TAB_look(cur, b->key);
      if (var != NULL) {
        transError(out, var->u.var.vd->pos,
                   Stringf("error: class %s has duplicate variable names with "
                           "parent class",
                           curClassId));
      }
      TAB_enter(cur, b->key, b->value);
    }
  }
}

void transA_ClassMtblCopyFromFa(FILE* out, S_table fa, S_table cur) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_ClassMtblCopyFromFa with class %s...\n",
          curClassId);
#endif
  int i;
  binder b;
  for (i = 0; i < TABSIZE; i++) {
    for (b = fa->table[i]; b; b = b->next) {
      E_enventry meth = TAB_look(cur, b->key);
      if (meth != NULL) {
        // check if signatures are equal
        E_enventry fa_meth = b->value;
        if (!equalClassMethSignature(fa_meth, meth)) {
          transError(out, meth->u.meth.md->pos,
                     Stringf("error: class %s has method %s with different "
                             "signature with parent class",
                             curClassId, meth->u.meth.md->id));
        }
      }
      TAB_enter(cur, b->key, b->value);
    }
  }
}

void transA_ClassDeclListTypeChecking(FILE* out, A_classDeclList cdl) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_ClassDeclListTypeChecking...\n");
#endif
  if (!cdl) return;

  transA_ClassDeclTypeChecking(out, cdl->head);

  if (cdl->tail) {
    transA_ClassDeclListTypeChecking(out, cdl->tail);
  }
}

void transA_ClassDeclTypeChecking(FILE* out, A_classDecl cd) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_ClassDeclTypeChecking with class %s...\n",
          cd->id);
#endif
  if (!cd) return;

  curClassId = cd->id;
  if (cd->vdl) {
    transA_VarDeclListClassVars(out, cd->vdl);
  }
  if (cd->mdl) {
    transA_MethodDeclListClassMethods(out, cd->mdl);
  }
}

void transA_ClassVtblCopyToVenv(FILE* out) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_ClassVtblCopyToVenv...\n");
#endif
  E_enventry ce = S_look(cenv, S_Symbol(curClassId));
  if (!ce) return;
  S_table vtbl = ce->u.cls.vtbl;

  int i;
  binder b;
  for (i = 0; i < TABSIZE; i++) {
    for (b = vtbl->table[i]; b; b = b->next) {
      TAB_enter(venv, b->key, b->value);
    }
  }
}

void transA_MethodDeclListCenvInit(FILE* out, A_methodDeclList mdl,
                                   S_table env) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_MethodDeclListCenvInit...\n");
#endif
  if (!mdl) return;

  transA_MethodDeclCenvInit(out, mdl->head, env);

  if (mdl->tail) {
    transA_MethodDeclListCenvInit(out, mdl->tail, env);
  }
}

void transA_MethodDeclListClassMethods(FILE* out, A_methodDeclList mdl) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_MethodDeclListClassMethods...\n");
#endif
  if (!mdl) return;

  transA_MethodDeclClassMethods(out, mdl->head);

  if (mdl->tail) {
    transA_MethodDeclListClassMethods(out, mdl->tail);
  }
}

void transA_MethodDeclCenvInit(FILE* out, A_methodDecl md, S_table env) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_MethodDeclCenvInit...\n");
#endif
  if (!md) return;

  // check if the method is already declared
  if (S_look(env, S_Symbol(md->id))) {
    transError(
        out, md->pos,
        Stringf("error: method already declared in class %s", curClassId));
  }

  // enter the method into the environment
  Ty_ty ret = atype2tyty(md->t);
  Ty_fieldList fl = fl2tyfl(md->fl);
  S_enter(env, S_Symbol(md->id),
          E_MethodEntry(md, S_Symbol(curClassId), ret, fl));
}

void transA_MethodDeclClassMethods(FILE* out, A_methodDecl md) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_MethodDeclClassMethods...\n");
#endif
  if (!md) return;

  S_beginScope(venv);

  curMethodId = md->id;

  // check if the return type is declared
  if (md->t->t == A_idType) {
    E_enventry x = S_look(cenv, S_Symbol(md->t->id));
    if (!x) {
      transError(
          out, md->pos,
          Stringf("error: method %s's return type not declared in class %s",
                  curMethodId, curClassId));
    }
  }

  // check if the formal list is declared, and add them to the environment
  if (md->fl) {
    transA_FormalList(out, md->fl);
  }

  // check if the declared variables are valid
  if (md->vdl) {
    transA_VarDeclListClassMethods(out, md->vdl);
  }

  // check if the statements are valid
  if (md->sl) {
    transA_StmList(out, md->sl);
  }

  S_endScope(venv);
}

void transA_FormalList(FILE* out, A_formalList fl) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_FormalList...\n");
#endif
  if (!fl) return;

  transA_Formal(out, fl->head);

  if (fl->tail) {
    transA_FormalList(out, fl->tail);
  }
}

void transA_Formal(FILE* out, A_formal f) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_Formal...\n");
#endif
  if (!f) return;

  // check if the variable is already declared
  if (S_look(venv, S_Symbol(f->id))) {
    transError(out, f->pos,
               Stringf("error: formal redefined in method %s, class %s",
                       curMethodId, curClassId));
  }

  switch (f->t->t) {
    case A_intType: {
      S_enter(venv, S_Symbol(f->id), E_VarEntry(NULL, Ty_Int()));
      break;
    }
    case A_floatType: {
      S_enter(venv, S_Symbol(f->id), E_VarEntry(NULL, Ty_Float()));
      break;
    }
    case A_intArrType: {
      S_enter(venv, S_Symbol(f->id), E_VarEntry(NULL, Ty_Array(Ty_Int())));
      break;
    }
    case A_floatArrType: {
      S_enter(venv, S_Symbol(f->id), E_VarEntry(NULL, Ty_Array(Ty_Float())));
      break;
    }
    case A_idType: {
      if (!S_look(cenv, S_Symbol(f->t->id))) {
        transError(out, f->pos, String("error: variable type not declared"));
      }
      S_enter(venv, S_Symbol(f->id),
              E_VarEntry(NULL, Ty_Name(S_Symbol(f->t->id))));
    }
  }
}

void transA_MainMethod(FILE* out, A_mainMethod m) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_MainMethod...\n");
#endif
  S_beginScope(venv);

  curClassId = S_name(MAIN_CLASS);

  if (m->vdl) {
    transA_VarDeclListMainMethod(out, m->vdl);
  }
  if (m->sl) {
    transA_StmList(out, m->sl);
  }
  S_endScope(venv);
}

void transA_VarDeclListCenvInit(FILE* out, A_varDeclList vdl, S_table env) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_VarDeclListCenvInit...\n");
#endif
  if (!vdl) return;

  transA_VarDeclCenvInit(out, vdl->head, env);

  if (vdl->tail) {
    transA_VarDeclListCenvInit(out, vdl->tail, env);
  }
}

void transA_VarDeclListClassVars(FILE* out, A_varDeclList vdl) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_VarDeclListClassVars...\n");
#endif
  if (!vdl) return;

  transA_VarDeclClassVars(out, vdl->head);

  if (vdl->tail) {
    transA_VarDeclListClassVars(out, vdl->tail);
  }
}

void transA_VarDeclListClassMethods(FILE* out, A_varDeclList vdl) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_VarDeclListClassMethods...\n");
#endif
  if (!vdl) return;

  transA_VarDeclClassMethods(out, vdl->head);

  if (vdl->tail) {
    transA_VarDeclListClassMethods(out, vdl->tail);
  }
}

void transA_VarDeclListMainMethod(FILE* out, A_varDeclList vdl) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_VarDeclListMainMethod...\n");
#endif
  if (!vdl) return;

  transA_VarDeclMainMethod(out, vdl->head);

  if (vdl->tail) {
    transA_VarDeclListMainMethod(out, vdl->tail);
  }
}

void transA_VarDeclCenvInit(FILE* out, A_varDecl vd, S_table env) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_VarDeclCenvInit...\n");
#endif
  if (!vd) return;

  // check if the variable is already declared
  if (S_look(env, S_Symbol(vd->v)) != NULL) {
    transError(
        out, vd->pos,
        Stringf("error: variable already declared in class %s", curClassId));
  }

  // enter the variable into the environment
  switch (vd->t->t) {
    case A_intType: {
      S_enter(env, S_Symbol(vd->v), E_VarEntry(vd, Ty_Int()));
      break;
    }
    case A_floatType: {
      S_enter(env, S_Symbol(vd->v), E_VarEntry(vd, Ty_Float()));
      break;
    }
    case A_intArrType: {
      S_enter(env, S_Symbol(vd->v), E_VarEntry(vd, Ty_Array(Ty_Int())));
      break;
    }
    case A_floatArrType: {
      S_enter(env, S_Symbol(vd->v), E_VarEntry(vd, Ty_Array(Ty_Float())));
      break;
    }
    case A_idType: {
      S_enter(env, S_Symbol(vd->v),
              E_VarEntry(vd, Ty_Name(S_Symbol(vd->t->id))));
      break;
    }
  }
}

void transA_VarDeclClassVars(FILE* out, A_varDecl vd) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_VarDeclClassVars...\n");
#endif
  if (!vd) return;

  if (vd->t->t == A_idType) {
    E_enventry x = S_look(cenv, S_Symbol(vd->t->id));
    if (!x) {
      transError(out, vd->pos, String("error: variable type not declared"));
    }
  }
}

void transA_VarDeclClassMethods(FILE* out, A_varDecl vd) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_VarDeclClassMethods...\n");
#endif
  if (!vd) return;

  // check if the variable is already declared
  if (S_look(venv, S_Symbol(vd->v)) != NULL) {
    transError(
        out, vd->pos,
        Stringf("error: variable already declared in method %s, class %s",
                curClassId, curMethodId));
  }

  // enter the variable into the environment
  switch (vd->t->t) {
    case A_intType: {
      S_enter(venv, S_Symbol(vd->v), E_VarEntry(vd, Ty_Int()));
      break;
    }
    case A_floatType: {
      S_enter(venv, S_Symbol(vd->v), E_VarEntry(vd, Ty_Float()));
      break;
    }
    case A_intArrType: {
      S_enter(venv, S_Symbol(vd->v), E_VarEntry(vd, Ty_Array(Ty_Int())));
      break;
    }
    case A_floatArrType: {
      S_enter(venv, S_Symbol(vd->v), E_VarEntry(vd, Ty_Array(Ty_Float())));
      break;
    }
    case A_idType: {
      if (!S_look(cenv, S_Symbol(vd->t->id))) {
        transError(out, vd->pos, String("error: variable type not declared"));
      }
      S_enter(venv, S_Symbol(vd->v),
              E_VarEntry(vd, Ty_Name(S_Symbol(vd->t->id))));
      break;
    }
  }
}

void transA_VarDeclMainMethod(FILE* out, A_varDecl vd) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_VarDeclMainMethod...\n");
#endif

  // check if the variable is already declared
  if (S_look(venv, S_Symbol(vd->v)) != NULL) {
    transError(out, vd->pos,
               String("error: variable already declared in main method"));
  }

  // enter the variable into the environment
  switch (vd->t->t) {
    case A_intType: {
      S_enter(venv, S_Symbol(vd->v), E_VarEntry(vd, Ty_Int()));
      break;
    }
    case A_floatType: {
      S_enter(venv, S_Symbol(vd->v), E_VarEntry(vd, Ty_Float()));
      break;
    }
    case A_intArrType: {
      S_enter(venv, S_Symbol(vd->v), E_VarEntry(vd, Ty_Array(Ty_Int())));
      break;
    }
    case A_floatArrType: {
      S_enter(venv, S_Symbol(vd->v), E_VarEntry(vd, Ty_Array(Ty_Float())));
      break;
    }
    case A_idType: {
      if (!S_look(cenv, S_Symbol(vd->t->id))) {
        transError(out, vd->pos, String("error: variable type not declared"));
      }
      S_enter(venv, S_Symbol(vd->v),
              E_VarEntry(vd, Ty_Name(S_Symbol(vd->t->id))));
      break;
    }
  }
}

void transA_StmList(FILE* out, A_stmList sl) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_StmList...\n");
#endif
  if (!sl) return;
  transA_Stm(out, sl->head);
  if (sl->tail) {
    transA_StmList(out, sl->tail);
  }
}

void transA_Stm(FILE* out, A_stm s) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_Stm...\n");
#endif
  if (!s) return;
  switch (s->kind) {
    case A_nestedStm:
      transA_NestedStm(out, s);
      break;
    case A_ifStm:
      transA_IfStm(out, s);
      break;
    case A_whileStm:
      transA_WhileStm(out, s);
      break;
    case A_assignStm:
      transA_AssignStm(out, s);
      break;
    case A_arrayInit:
      transA_ArrayInit(out, s);
      break;
    case A_callStm:
      transA_CallStm(out, s);
      break;
    case A_continue:
      transA_Continue(out, s);
      break;
    case A_break:
      transA_Break(out, s);
      break;
    case A_return:
      transA_Return(out, s);
      break;
    case A_putnum:
      transA_Putnum(out, s);
      break;
    case A_putch:
      transA_Putch(out, s);
      break;
    case A_putarray:
      transA_Putarray(out, s);
      break;
    default:
      return;  // unreachable
  }
}

void transA_NestedStm(FILE* out, A_stm s) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_NestedStm...\n");
#endif
  if (!s) return;
  transA_StmList(out, s->u.ns);
}

void transA_IfStm(FILE* out, A_stm s) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_IfStm...\n");
#endif
  if (!s) return;

  expty ty = transA_Exp(out, s->u.if_stat.e);
  if (!ty) return;
  if (ty->ty->kind != Ty_int && ty->ty->kind != Ty_float) {
    transError(
        out, s->pos,
        String("error: if statement condition must be of type int or float"));
  }

  transA_Stm(out, s->u.if_stat.s1);
  if (s->u.if_stat.s2) {
    transA_Stm(out, s->u.if_stat.s2);
  }
}

void transA_WhileStm(FILE* out, A_stm s) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_WhileStm...\n");
#endif
  if (!s) return;

  whileDepth++;

  expty ty = transA_Exp(out, s->u.while_stat.e);
  if (!ty) return;
  if (ty->ty->kind != Ty_int && ty->ty->kind != Ty_float) {
    transError(
        out, s->pos,
        String(
            "error: while statement condition must be of type int or float"));
  }

  if (s->u.while_stat.s) {
    transA_Stm(out, s->u.while_stat.s);
  }
  whileDepth--;
}

void transA_AssignStm(FILE* out, A_stm s) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_AssignStm...\n");
#endif
  if (!s) return;

  expty left = transA_Exp(out, s->u.assign.arr);
  if (!left) return;
  // check if the left side is a lvalue
  if (!left->location) {
    transError(
        out, s->pos,
        String("Error: Left side of assignment must have a location value"));
  }

  expty right = transA_Exp(out, s->u.assign.value);
  if (!right) return;
    // check if the types match
  if ((left->ty->kind == Ty_int || left->ty->kind == Ty_float) &&
      (right->ty->kind != Ty_int && right->ty->kind != Ty_float)) {
    if (left->ty->kind == Ty_int) {
      transError(out, s->pos,
                 String("error: right side of assignment must be of type int"));
    } else {
      transError(
          out, s->pos,
          String("error: right side of assignment must be of type float"));
    }
  }
  if (left->ty->kind == Ty_array) {
    if (right->ty->kind != Ty_array) {
      transError(
          out, s->pos,
          String("error: right side of assignment must be of type array"));
    }
    if (left->ty->u.array->kind != right->ty->u.array->kind) {
      transError(out, s->pos,
                 String("error: array types must match in assignment"));
    }
  }
  if (left->ty->kind == Ty_name) {
    if (right->ty->kind != Ty_name) {
      transError(
          out, s->pos,
          String("error: right side of assignment must be of type object"));
    }
    if (!isParentClass(left->ty, right->ty)) {
      transError(out, s->pos,
                 Stringf("error: object types do not match in assignment, "
                         "right side expected '%s', got '%s'",
                         ty2str(left->ty), ty2str(right->ty)));
    }
  }
}

void transA_ArrayInit(FILE* out, A_stm s) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_ArrayInit...\n");
#endif
  if (!s) return;

  expty arr = transA_Exp(out, s->u.array_init.arr);
  if (!arr) return;
  if (arr->ty->kind != Ty_array) {
    transError(
        out, s->pos,
        String("error: left side of array initialization must be an array"));
  }
  if (!arr->location) {
    transError(out, s->pos,
               String("error: left side of array initialization must have a "
                      "location value"));
  }

  transA_ArrayInitExpList(out, s->u.array_init.init_values);
}

void transA_ArrayInitExpList(FILE* out, A_expList el) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_ArrayInitExpList...\n");
#endif
  if (!el) return;

  expty ty = transA_Exp(out, el->head);
  if (!ty) return;
  if (ty->ty->kind != Ty_int && ty->ty->kind != Ty_float) {
    transError(
        out, el->head->pos,
        String("error: array initialization must be of type int or float"));
  }

  if (el->tail) {
    transA_ArrayInitExpList(out, el->tail);
  }
}

void transA_CallStm(FILE* out, A_stm s) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_CallStm...\n");
#endif
  if (!s) return;

  expty ty = transA_Exp(out, s->u.call_stat.obj);
  if (!ty) return;
  if (ty->ty->kind != Ty_name) {
    transError(out, s->pos,
               String("error: call statement must be called on an object"));
  }

  E_enventry ce = S_look(cenv, ty->ty->u.name);
  S_table mtbl = ce->u.cls.mtbl;
  E_enventry me = S_look(mtbl, S_Symbol(s->u.call_stat.fun));
  if (!me) {
    transError(out, s->pos,
               Stringf("error: class %s has no method %s", ty->ty->u.name,
                       s->u.call_stat.fun));
  }

  transA_CallExpList(out, s->u.call_stat.el, me->u.meth.fl);
  // if (!transA_CallExpList(out, s->u.call_stat.el, me->u.meth.fl)) {
  //   transError(
  //       out, s->pos,
  //       String("Error: Method call arguments do not match method
  //       declaration"));
  // }
}

void transA_CallExpList(FILE* out, A_expList el, Ty_fieldList fl) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_CallExpList...\n");
#endif
  A_expList curEl = el;
  Ty_fieldList curFl = fl;

  while (curEl && curFl) {
    expty ty = transA_Exp(out, curEl->head);
    if (!ty) {
      transError(out, curEl->head->pos,
                 String("error: method call argument is invalid"));
    }
    if (!equalTyCast(curFl->head->ty, ty->ty)) {
      transError(out, curEl->head->pos,
                 Stringf("error: method call argument types do not match, "
                         "expected '%s', got '%s'",
                         ty2str(curFl->head->ty), ty2str(ty->ty)));
    }
    curEl = curEl->tail;
    curFl = curFl->tail;
  }

  if (curEl) {
    transError(out, curEl->head->pos,
               String("error: too many arguments in method call"));
  }
  if (curFl) {
    transError(out, el->head->pos,
               String("error: too few arguments in method call"));
  }
}

void transA_Continue(FILE* out, A_stm s) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_Continue...\n");
#endif
  if (!s) return;

  if (whileDepth == 0) {
    transError(out, s->pos,
               String("error: continue statement outside of loop"));
  }
}

void transA_Break(FILE* out, A_stm s) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_Break...\n");
#endif
  if (!s) return;

  if (whileDepth == 0) {
    transError(out, s->pos, String("error: break statement outside of loop"));
  }
}

void transA_Return(FILE* out, A_stm s) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_Return...\n");
#endif
  if (!s) return;

  expty ty = transA_Exp(out, s->u.e);
  if (!ty) return;

  // main method
  if (S_Symbol(curClassId) == MAIN_CLASS) {
    if (ty->ty->kind != Ty_int && ty->ty->kind != Ty_float) {
      transError(out, s->pos,
                 String("error: return value of main method must be of type "
                        "int or float"));
    }
  } else {
    E_enventry ce = S_look(cenv, S_Symbol(curClassId));
    S_table mtbl = ce->u.cls.mtbl;
    E_enventry me = S_look(mtbl, S_Symbol(curMethodId));
    if (!equalTyCast(me->u.meth.ret, ty->ty)) {
      transError(out, s->pos,
                 Stringf("error: return type expected '%s', got '%s'",
                         ty2str(me->u.meth.ret), ty2str(ty->ty)));
    }
  }
}

void transA_Putnum(FILE* out, A_stm s) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_Putnum...\n");
#endif
  if (!s) return;

  expty ty = transA_Exp(out, s->u.e);
  if (!ty) return;
  if (ty->ty->kind != Ty_int && ty->ty->kind != Ty_float) {
    transError(
        out, s->pos,
        String("error: argument of putnum() must be of type int or float"));
  }
}

void transA_Putch(FILE* out, A_stm s) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_Putch...\n");
#endif
  if (!s) return;

  expty ty = transA_Exp(out, s->u.e);
  if (!ty) return;
  if (ty->ty->kind != Ty_int && ty->ty->kind != Ty_float) {
    transError(
        out, s->pos,
        String("error: argument of putch() must be of type int or float"));
  }
}

void transA_Putarray(FILE* out, A_stm s) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_Putarray...\n");
#endif
  if (!s) return;

  expty ty1 = transA_Exp(out, s->u.putarray.e1);
  if (!ty1) return;
  if (ty1->ty->kind != Ty_int && ty1->ty->kind != Ty_float) {
    transError(out, s->pos,
               String("error: first argument of putarray() must be of type int "
                      "or float"));
  }

  expty ty2 = transA_Exp(out, s->u.putarray.e2);
  if (!ty2) return;
  if (ty2->ty->kind != Ty_array) {
    transError(
        out, s->pos,
        String("error: second argument of putarray() must be of type array"));
  }
}

expty transA_Exp(FILE* out, A_exp e) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_Exp...\n");
#endif
  if (!e) return NULL;

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
      return transA_NumConst(out, e);
    case A_idExp:
      return transA_IdExp(out, e);
    case A_thisExp:
      return transA_ThisExp(out, e);
    case A_lengthExp:
      return transA_LengthExp(out, e);
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
      return NULL;  // unreachable
  }
}

expty transA_OpExp(FILE* out, A_exp e) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_OpExp...\n");
#endif
  if (!e) return NULL;

  expty left = transA_Exp(out, e->u.op.left);
  if (!left) return NULL;
  if (left->ty->kind != Ty_int && left->ty->kind != Ty_float) {
    transError(
        out, e->pos,
        String("error: left side of operator must be of type int or float"));
  }

  expty right = transA_Exp(out, e->u.op.right);
  if (!right) return NULL;
  if (right->ty->kind != Ty_int && right->ty->kind != Ty_float) {
    transError(
        out, e->pos,
        String("error: right side of operator must be of type int or float"));
  }

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
      return Expty(FALSE, Ty_Int());
    case A_plus:
    case A_minus:
    case A_times:
    case A_div: {
      if (left->ty->kind == Ty_int && right->ty->kind == Ty_int) {
        return Expty(FALSE, Ty_Int());
      } else {
        return Expty(FALSE, Ty_Float());
      }
    }
    default:
      return NULL;
  }
}

expty transA_ArrayExp(FILE* out, A_exp e) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_ArrayExp...\n");
#endif
  if (!e) return NULL;

  expty arr = transA_Exp(out, e->u.array_pos.arr);
  if (!arr) return NULL;
  if (arr->ty->kind != Ty_array) {
    transError(out, e->pos,
               String("error: left side of array access must be an array"));
  }

  expty pos = transA_Exp(out, e->u.array_pos.arr_pos);
  if (!pos) return NULL;
  if (pos->ty->kind != Ty_int && pos->ty->kind != Ty_float) {
    transError(
        out, e->pos,
        String(
            "error: right side of array access must be of type int or float"));
  }

  return Expty(TRUE, arr->ty->u.array);
}

expty transA_CallExp(FILE* out, A_exp e) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_CallExp...\n");
#endif
  if (!e) return NULL;

  expty ty = transA_Exp(out, e->u.call.obj);
  if (!ty) return NULL;
  if (ty->ty->kind != Ty_name) {
    transError(out, e->pos,
               String("error: call expression must be called on an object"));
  }

  E_enventry ce = S_look(cenv, ty->ty->u.name);
  S_table mtbl = ce->u.cls.mtbl;
  E_enventry me = S_look(mtbl, S_Symbol(e->u.call.fun));
  if (!me) {
    transError(out, e->pos,
               Stringf("error: class %s has no method %s",
                       S_name(ty->ty->u.name), e->u.call.fun));
  }

  transA_CallExpList(out, e->u.call.el, me->u.meth.fl);

  // if (!transA_CallExpList(out, e->u.call.el, me->u.meth.fl)) {
  //   transError(
  //       out, e->pos,
  //       String("Error: Method call arguments do not match method
  //       declaration"));
  // }

  return Expty(FALSE, me->u.meth.ret);
}

expty transA_ClassVarExp(FILE* out, A_exp e) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_ClassVarExp...\n");
#endif
  if (!e) return NULL;

  expty ty = transA_Exp(out, e->u.classvar.obj);
  if (!ty) return NULL;
  if (ty->ty->kind != Ty_name) {
    transError(out, e->pos,
               String("error: class variable must be accessed on an object"));
  }

  E_enventry ce = S_look(cenv, ty->ty->u.name);
  S_table vtbl = ce->u.cls.vtbl;
  E_enventry ve = S_look(vtbl, S_Symbol(e->u.classvar.var));
  if (!ve) {
    transError(out, e->pos,
               Stringf("Error: Class %s has no variable %s",
                       S_name(ty->ty->u.name), e->u.classvar.var));
  }

  return Expty(TRUE, ve->u.var.ty);
}

expty transA_BoolConst(FILE* out, A_exp e) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_BoolConst...\n");
#endif
  if (!e) return NULL;

  return Expty(FALSE, Ty_Int());
}

expty transA_NumConst(FILE* out, A_exp e) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_NumConst...\n");
#endif
  if (!e) return NULL;

  return Expty(FALSE, Ty_Float());
}

expty transA_IdExp(FILE* out, A_exp e) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_IdExp...\n");
#endif
  if (!e) return NULL;

  E_enventry x = S_look(venv, S_Symbol(e->u.v));
  if (!x) {
    transError(out, e->pos, String("error: variable not declared"));
  }

  return Expty(TRUE, x->u.var.ty);
}

expty transA_ThisExp(FILE* out, A_exp e) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_ThisExp...\n");
#endif
  if (!e) return NULL;

  if (S_Symbol(curClassId) == MAIN_CLASS) {
    transError(out, e->pos,
               String("error: 'this' cannot be used in main method"));
  }

  return Expty(TRUE, Ty_Name(S_Symbol(curClassId)));
}

expty transA_LengthExp(FILE* out, A_exp e) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_LengthExp...\n");
#endif
  if (!e) return NULL;

  expty arr = transA_Exp(out, e->u.e);
  if (!arr) return NULL;
  if (arr->ty->kind != Ty_array) {
    transError(out, e->pos,
               String("error: argument of length() must be an array"));
  }

  return Expty(FALSE, Ty_Int());
}

expty transA_NewIntArrExp(FILE* out, A_exp e) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_NewIntArrExp...\n");
#endif
  if (!e) return NULL;

  expty size = transA_Exp(out, e->u.e);
  if (!size) return NULL;
  if (size->ty->kind != Ty_int && size->ty->kind != Ty_float) {
    transError(
        out, e->pos,
        String("error: new int array size must be of type int or float"));
  }

  return Expty(FALSE, Ty_Array(Ty_Int()));
}

expty transA_NewFloatArrExp(FILE* out, A_exp e) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_NewFloatArrExp...\n");
#endif
  if (!e) return NULL;

  expty size = transA_Exp(out, e->u.e);
  if (!size) return NULL;
  if (size->ty->kind != Ty_int && size->ty->kind != Ty_float) {
    transError(
        out, e->pos,
        String("error: new float array size must be of type int or float"));
  }

  return Expty(FALSE, Ty_Array(Ty_Float()));
}

expty transA_NewObjExp(FILE* out, A_exp e) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_NewObjExp...\n");
#endif
  if (!e) return NULL;

  E_enventry ce = S_look(cenv, S_Symbol(e->u.v));
  if (!ce) {
    transError(out, e->pos, String("error: class not declared"));
  }

  return Expty(FALSE, Ty_Name(S_Symbol(e->u.v)));
}

expty transA_NotExp(FILE* out, A_exp e) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_NotExp...\n");
#endif
  if (!e) return NULL;

  expty ty = transA_Exp(out, e->u.e);
  if (!ty) return NULL;
  if (ty->ty->kind != Ty_int && ty->ty->kind != Ty_float) {
    transError(out, e->pos, String("error: ! must operate on int or float"));
  }

  return Expty(FALSE, Ty_Int());
}

expty transA_MinusExp(FILE* out, A_exp e) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_MinusExp...\n");
#endif
  if (!e) return NULL;

  expty ty = transA_Exp(out, e->u.e);
  if (!ty) return NULL;
  if (ty->ty->kind != Ty_int && ty->ty->kind != Ty_float) {
    transError(out, e->pos, String("error: - must operate on int or float"));
  }

  return Expty(FALSE, ty->ty);
}

expty transA_EscExp(FILE* out, A_exp e) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_EscExp...\n");
#endif
  if (!e) return NULL;

  transA_StmList(out, e->u.escExp.ns);

  return transA_Exp(out, e->u.escExp.exp);
}

expty transA_Getnum(FILE* out, A_exp e) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_Getnum...\n");
#endif
  if (!e) return NULL;

  return Expty(FALSE, Ty_Float());
}

expty transA_Getch(FILE* out, A_exp e) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_Getch...\n");
#endif
  if (!e) return NULL;

  return Expty(FALSE, Ty_Int());
}

expty transA_Getarray(FILE* out, A_exp e) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_Getarray...\n");
#endif
  if (!e) return NULL;

  expty arr = transA_Exp(out, e->u.e);
  if (!arr) return NULL;
  if (arr->ty->kind != Ty_array) {
    transError(out, e->pos,
               String("error: argument of getarray() must be an array"));
  }

  return Expty(FALSE, Ty_Int());
}

bool equalClassMethSignature(E_enventry fa, E_enventry cur) {
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

bool equalTy(Ty_ty fa, Ty_ty cur) {
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

bool equalTyCast(Ty_ty param, Ty_ty arg) {
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

bool isParentClass(Ty_ty left, Ty_ty right) {
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

Ty_ty atype2tyty(A_type t) {
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

Ty_field f2tyf(A_formal f) {
  if (!f) return NULL;

  return Ty_Field(S_Symbol(f->id), atype2tyty(f->t));
}

Ty_fieldList fl2tyfl(A_formalList fl) {
  if (!fl) return NULL;

  return Ty_FieldList(f2tyf(fl->head), fl2tyfl(fl->tail));
}

string ty2str(Ty_ty t) {
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
