#include "semant.h"

int SEM_ARCH_SIZE; // to be set by arch_size in transA_Prog

expty ExpTy(Tr_exp exp, Ty_ty value, Ty_ty location) {
  expty et = checked_malloc(sizeof(*et));
  et->exp = exp;
  et->value = value;
  et->location = location;
  return et;
}

void transError(FILE *out, A_pos pos, string msg) {
  fprintf(out, "(line:%d col:%d) %s\n", pos->line, pos->pos, msg);
  fflush(out);
  exit(1);
}

/* TODO: semant */

T_funcDeclList transA_Prog(FILE *out, A_prog p, int arch_size) {
  transError(out, A_Pos(0,0), "TODO: translate during semant! See Tiger Book for designing details:)");
  return NULL;
}