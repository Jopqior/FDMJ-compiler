#include "semant.h"
#include <stdio.h>
#include <stdlib.h>

int SEM_ARCH_SIZE; // to be set by arch_size in transA_Prog

void transError(FILE *out, A_pos pos, string msg) {
  fprintf(out, "(line:%d col:%d) %s\n", pos->line, pos->pos, msg);
  fflush(out);
  exit(1);
}

T_funcDeclList transA_Prog(FILE *out, A_prog p, int arch_size) {
  transError(out, A_Pos(0,0), "TODO: translate during semant! See Tiger Book for designing details:)");
  return NULL;
}