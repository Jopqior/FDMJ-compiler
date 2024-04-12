#include "semant.h"

#define __DEBUG
#undef __DEBUG

int SEM_ARCH_SIZE; // to be set by arch_size in transA_Prog

expty ExpTy(Tr_exp exp, Ty_ty value, Ty_ty location) {
  expty et = checked_malloc(sizeof(*et));
  et->exp = exp;
  et->value = value;
  et->location = location;
  return et;
}

/* envs */

static S_table cenv;
static S_symbol MAIN_CLASS;
static string curClassId;
static string curMethodId;

// stack for while loop

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

void transPreprocess(FILE* out, A_classDeclList cdl) {
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
  return T_FuncDeclList(mainmethod, NULL);
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
  
  if (!vd) {
    return rest;
  }
  if (!rest) {
    return vd;
  }
  return Tr_StmList(vd, rest);
}

Tr_exp transA_VarDecl(FILE *out, A_varDecl vd) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_VarDecl...\n");
#endif
}

// main method
T_funcDecl transA_MainMethod(FILE *out, A_mainMethod main) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_MainMethod...\n");
#endif
  curClassId = S_name(MAIN_CLASS);

  Tr_exp vdl = transA_VarDeclList(out, main->vdl);
  Tr_exp sl = transA_StmList(out, main->sl);
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