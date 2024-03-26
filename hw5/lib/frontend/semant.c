#include "semant.h"
#include "symbol.h"
#include "table.h"
#include <stdlib.h>

void transError(FILE *out, A_pos pos, string msg) {
  fprintf(out, "(line:%d col:%d) %s\n", pos->line, pos->pos, msg);
  fflush(out);
  exit(1);
}

void transA_Prog(FILE* out, A_prog p) {
  //print all symantic check errors to out
  transError(out, A_Pos(0, 0), "Ehhh.. I haven't done anything... but hey! ALL CORRECT!!! COOL.");
}
