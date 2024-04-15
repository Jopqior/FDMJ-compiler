#include "semant.h"

#include <stdarg.h>
#include <stdlib.h>

#include "symbol.h"
#include "table.h"

#define __DEBUG
#undef __DEBUG

/* structs */

expty Expty(bool location, Ty_ty ty) {
  expty e = checked_malloc(sizeof(*e));
  e->location = location;
  e->ty = ty;
  return e;
}

/* semant */

static S_table venv;  // for main method
static int whileDepth = 0;

static S_table cenv;
static S_symbol MAIN_CLASS;
static string curClassId;
static string curMethodId;

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
    transPreprocess(out, p->cdl);
  }

  if (!(p->m)) {
    transError(out, p->pos, String("error: there's no main class"));
  }

  transSemant(out, p->cdl, p->m);
}

void transPreprocess(FILE* out, A_classDeclList cdl) {
#ifdef __DEBUG
  fprintf(out, "Entering transPreprocess...\n");
#endif
  if (!cdl) return;

  // first pass: record class names and inheritance
  transA_ClassDeclList_basic(out, cdl);

  // second pass: detect cycles in inheritance
  transA_ClassDeclList_extend(out, cdl);
}

void transA_ClassDeclList_basic(FILE* out, A_classDeclList cdl) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_ClassDeclList_basic...\n");
#endif
  if (!cdl) return;

  transA_ClassDecl_basic(out, cdl->head);

  if (cdl->tail) {
    transA_ClassDeclList_basic(out, cdl->tail);
  }
}

void transA_ClassDecl_basic(FILE* out, A_classDecl cd) {
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
  transA_MethodDeclList_basic(out, mtbl, cd->mdl);

  S_enter(cenv, S_Symbol(cd->id),
          E_ClassEntry(cd, fa, E_transInit, vtbl, mtbl));
}

void transA_ClassVarDeclList_basic(FILE* out, S_table vtbl, A_varDeclList vdl) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_ClassVarDeclList_basic...\n");
#endif
  if (!vdl) return;

  transA_ClassVarDecl_basic(out, vtbl, vdl->head);

  if (vdl->tail) {
    transA_ClassVarDeclList_basic(out, vtbl, vdl->tail);
  }
}

void transA_ClassVarDecl_basic(FILE* out, S_table vtbl, A_varDecl vd) {
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
      S_enter(vtbl, S_Symbol(vd->v), E_VarEntry(vd, Ty_Int()));
      break;
    }
    case A_floatType: {
      S_enter(vtbl, S_Symbol(vd->v), E_VarEntry(vd, Ty_Float()));
      break;
    }
    case A_intArrType: {
      S_enter(vtbl, S_Symbol(vd->v), E_VarEntry(vd, Ty_Array(Ty_Int())));
      break;
    }
    case A_floatArrType: {
      S_enter(vtbl, S_Symbol(vd->v), E_VarEntry(vd, Ty_Array(Ty_Float())));
      break;
    }
    case A_idType: {
      S_enter(vtbl, S_Symbol(vd->v),
              E_VarEntry(vd, Ty_Name(S_Symbol(vd->t->id))));
      break;
    }
  }
}

void transA_MethodDeclList_basic(FILE* out, S_table mtbl,
                                 A_methodDeclList mdl) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_MethodDeclList_basic...\n");
#endif
  if (!mdl) return;

  transA_MethodDecl_basic(out, mtbl, mdl->head);

  if (mdl->tail) {
    transA_MethodDeclList_basic(out, mtbl, mdl->tail);
  }
}

void transA_MethodDecl_basic(FILE* out, S_table mtbl, A_methodDecl md) {
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

  // enter the method into the environment
  S_enter(mtbl, S_Symbol(md->id),
          E_MethodEntry(md, S_Symbol(curClassId), ret, fl));
}

Ty_fieldList transA_FormalList_basic(FILE* out, A_formalList fl) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_FormalList_basic...\n");
#endif
  if (!fl) return NULL;

  return Ty_FieldList(transA_Formal_basic(out, fl->head),
                      transA_FormalList_basic(out, fl->tail));
}

Ty_field transA_Formal_basic(FILE* out, A_formal f) {
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
  S_enter(venv, S_Symbol(f->id), E_VarEntry(f2vd(f), atype2tyty(f->t)));

  return f2tyf(f);
}

void transA_ClassDeclList_extend(FILE* out, A_classDeclList cdl) {
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

void transA_ClassDecl_extend(FILE* out, A_classDecl cd) {
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

void transA_ClassVtbl_copy(FILE* out, S_table fa, S_table cur) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_ClassVtbl_copy with class %s...\n", curClassId);
#endif
  void* top;
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

void transA_ClassMtbl_copy(FILE* out, S_table fa, S_table cur) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_ClassMtbl_copy with class %s...\n", curClassId);
#endif
  void* top;
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

void transSemant(FILE* out, A_classDeclList cdl, A_mainMethod m) {
#ifdef __DEBUG
  fprintf(out, "Entering transSemant...\n");
#endif
  if (cdl) {
    transA_ClassDeclList(out, cdl);
  }

  transA_MainMethod(out, m);
}

void transA_ClassDeclList(FILE* out, A_classDeclList cdl) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_ClassDeclList...\n");
#endif
  if (!cdl) return;

  transA_ClassDecl(out, cdl->head);

  if (cdl->tail) {
    transA_ClassDeclList(out, cdl->tail);
  }
}

void transA_ClassDecl(FILE* out, A_classDecl cd) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_ClassDecl with class %s...\n", cd->id);
#endif
  if (!cd) return;

  curClassId = cd->id;
  if (cd->vdl) {
    transA_ClassVarDeclList(out, cd->vdl);
  }
  if (cd->mdl) {
    transA_MethodDeclList(out, cd->mdl);
  }
}

void transA_ClassVarDeclList(FILE* out, A_varDeclList vdl) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_ClassVarDeclList...\n");
#endif
  if (!vdl) return;

  transA_ClassVarDecl(out, vdl->head);

  if (vdl->tail) {
    transA_ClassVarDeclList(out, vdl->tail);
  }
}

void transA_ClassVarDecl(FILE* out, A_varDecl vd) {
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

void transA_MethodDeclList(FILE* out, A_methodDeclList mdl) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_MethodDeclList...\n");
#endif
  if (!mdl) return;

  transA_MethodDecl(out, mdl->head);

  if (mdl->tail) {
    transA_MethodDeclList(out, mdl->tail);
  }
}

void transA_MethodDecl(FILE* out, A_methodDecl md) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_MethodDecl...\n");
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
    transA_VarDeclList(out, md->vdl);
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

  // no need to check if the variable is already declared

  switch (f->t->t) {
    case A_intType: {
      S_enter(venv, S_Symbol(f->id), E_VarEntry(f2vd(f), Ty_Int()));
      break;
    }
    case A_floatType: {
      S_enter(venv, S_Symbol(f->id), E_VarEntry(f2vd(f), Ty_Float()));
      break;
    }
    case A_intArrType: {
      S_enter(venv, S_Symbol(f->id), E_VarEntry(f2vd(f), Ty_Array(Ty_Int())));
      break;
    }
    case A_floatArrType: {
      S_enter(venv, S_Symbol(f->id), E_VarEntry(f2vd(f), Ty_Array(Ty_Float())));
      break;
    }
    case A_idType: {
      if (!S_look(cenv, S_Symbol(f->t->id))) {
        transError(out, f->pos, String("error: variable type not declared"));
      }
      S_enter(venv, S_Symbol(f->id),
              E_VarEntry(f2vd(f), Ty_Name(S_Symbol(f->t->id))));
      break;
    }
    default:
      break; // unreachable
  }
}

void transA_MainMethod(FILE* out, A_mainMethod m) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_MainMethod...\n");
#endif
  S_beginScope(venv);

  curClassId = S_name(MAIN_CLASS);

  if (m->vdl) {
    transA_VarDeclList(out, m->vdl);
  }
  if (m->sl) {
    transA_StmList(out, m->sl);
  }
  S_endScope(venv);
}

void transA_VarDeclList(FILE* out, A_varDeclList vdl) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_VarDeclList...\n");
#endif
  if (!vdl) return;

  transA_VarDecl(out, vdl->head);

  if (vdl->tail) {
    transA_VarDeclList(out, vdl->tail);
  }
}

void transA_VarDecl(FILE* out, A_varDecl vd) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_VarDecl...\n");
#endif
  if (!vd) return;

  // check if the variable is already declared
  if (S_look(venv, S_Symbol(vd->v)) != NULL) {
    if (MAIN_CLASS == S_Symbol(curClassId)) {
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
    default:
      break; // unreachable
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
    case A_starttime:
      transA_Starttime(out, s);
      break;
    case A_stoptime:
      transA_Stoptime(out, s);
      break;
    default:
      break;  // unreachable
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

  expty ty = transA_Exp(out, s->u.while_stat.e);
  if (!ty) return;
  if (ty->ty->kind != Ty_int && ty->ty->kind != Ty_float) {
    transError(
        out, s->pos,
        String(
            "error: while statement condition must be of type int or float"));
  }

  whileDepth++;
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
        String("error: left side of assignment must have a location value"));
  }

  expty right = transA_Exp(out, s->u.assign.value);
  if (!right) return;
  // check if the types match
  if (!equalTyCast(left->ty, right->ty)) {
    transError(
        out, s->pos,
        Stringf("error: right side of assignment expected '%s', got '%s'",
                ty2str(left->ty), ty2str(right->ty)));
  }
}

void transA_ArrayInit(FILE* out, A_stm s) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_ArrayInit...\n");
#endif
  if (!s) return;

  expty arr = transA_Exp(out, s->u.array_init.arr);
  if (!arr) return;
  if (!arr->location) {
    transError(out, s->pos,
               String("error: left side of array initialization must have a "
                      "location value"));
  }
  if (arr->ty->kind != Ty_array) {
    transError(
        out, s->pos,
        String("error: left side of array initialization must be an array"));
  }

  transA_ExpList_ArrayInit(out, s->u.array_init.init_values);
}

void transA_ExpList_ArrayInit(FILE* out, A_expList el) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_ExpList_ArrayInit...\n");
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
    transA_ExpList_ArrayInit(out, el->tail);
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
               Stringf("error: class %s has no method %s",
                       S_name(ty->ty->u.name), s->u.call_stat.fun));
  }

  transA_ExpList_Call(out, me->u.meth.fl, s->u.call_stat.el, s->pos);
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

void transA_Starttime(FILE* out, A_stm s) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_Starttime...\n");
#endif
  if (!s) return;
  // do nothing
}

void transA_Stoptime(FILE* out, A_stm s) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_Stoptime...\n");
#endif
  if (!s) return;
  // do nothing
}

void transA_ExpList_Call(FILE* out, Ty_fieldList fl, A_expList el, A_pos pos) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_ExpList_Call...\n");
#endif
  if (!fl && !el) return;
  if (!fl) {
    transError(out, pos, String("error: too many arguments in method call"));
  }
  if (!el) {
    transError(out, pos, String("error: too few arguments in method call"));
  }

  // check if the types match
  transA_Exp_Call(out, fl->head, el->head);

  transA_ExpList_Call(out, fl->tail, el->tail, pos);
}

void transA_Exp_Call(FILE* out, Ty_field f, A_exp e) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_ExpList_Call...\n");
#endif
  expty ty = transA_Exp(out, e);
  if (!ty) {
    transError(out, e->pos, String("error: method call argument is invalid"));
  }
  if (!equalTyCast(f->ty, ty->ty)) {
    transError(out, e->pos,
               Stringf("error: method call argument types do not match, "
                       "expected '%s', got '%s'",
                       ty2str(f->ty), ty2str(ty->ty)));
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

  transA_ExpList_Call(out, me->u.meth.fl, e->u.call.el, e->pos);

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

static A_varDecl f2vd(A_formal f) {
  if (!f) return NULL;

  return A_VarDecl(f->pos, f->t, f->id, NULL);
}

static char buf[IR_MAXLEN];
static string Stringf(char* fmt, ...) {
  va_list argp;
  va_start(argp, fmt);
  vsnprintf(buf, IR_MAXLEN, fmt, argp);
  va_end(argp);
  return String(buf);
}
