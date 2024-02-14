#include <stdio.h>
#include "fdmjslpast.h"
#include "slpast_fdmjprinter.h"

#define __DEBUG
#undef __DEBUG

/* */
void printA_Pos(FILE* out, A_pos pos) {
#ifdef __DEBUG
  fprintf(out, "Entering printA_Pos...\n");
#endif
  return ; //don't need to print position
}

/* */
void printA_Prog(FILE *out, A_prog p) {
#ifdef __DEBUG
  fprintf(out, "Entering printA_Prog...\n");
#endif
  if (p->m) printA_MainMethod(out, p->m);
  else
    fprintf(out, "Error: There's no main class!\n");
  fflush(out);
  return;
}

/* */
void printA_MainMethod(FILE *out, A_mainMethod main) {
#ifdef __DEBUG
  fprintf(out, "Entering printA_MainMethod...\n");
#endif
  fprintf(out, "public int main () {\n");
  if (main->sl) printA_StmList(out, main->sl);
  fprintf(out, "}\n");
  return ;
}

/* */
void printA_StmList(FILE *out, A_stmList sl) {
#ifdef __DEBUG
  fprintf(out, "Entering printA_StmList...\n");
#endif
  if (!sl) return;
  printA_Stm(out, sl->head);
  if (sl->tail) printA_StmList(out, sl->tail);
  return;
}

/* */
void printA_Stm(FILE *out, A_stm s) {
#ifdef __DEBUG
  fprintf(out, "Entering printA_Stm...\n");
#endif
  if (!s) return;
  switch (s->kind) {
  case A_assignStm:
    printA_AssignStm(out, s);
    break;
  case A_putint:
    printA_Putint(out, s);
    break;
  case A_putch:
    printA_Putch(out, s);
    break;
  default:
    fprintf(out, "Unknown statement kind!");
    break;
  }
  return;
}


/* */
void printA_AssignStm(FILE *out, A_stm s) {
#ifdef __DEBUG
  fprintf(out, "Entering printA_AssignStm...\n");
#endif
  if (!s) return;
  if (s->kind != A_assignStm) fprintf(out, "Not an assign stm!\n");
  else {
    printA_Exp(out, s->u.assign.id);
    /* March 20, 2023. Remove redundant grammar rule.. hence the ast struct
        if (s->u.assign.pos) { //this is an array position
          fprintf(out, "[");
          printA_Exp(out, s->u.assign.pos);
          fprintf(out, "]");
        }
    */
    fprintf(out, "=");
    printA_Exp(out, s->u.assign.value);
    fprintf(out, ";\n");
  }
  return;
}

/* */
void printA_Putint(FILE *out, A_stm s) {
#ifdef __DEBUG
  fprintf(out, "Entering printA_Putin...\n");
#endif
  if (s->kind != A_putint) fprintf(out, "Not a putint stm!\n");
  else {
    fprintf(out, "putint(");
    if (s->u.e) printA_Exp(out, s->u.e);
    fprintf(out, ");\n");
  }
  return;
}

/* */
void printA_Putch(FILE *out, A_stm s) {
#ifdef __DEBUG
  fprintf(out, "Entering printA_Putch...\n");
#endif
  if (s->kind != A_putch) fprintf(out, "Not a putch stm!\n");
  else {
    fprintf(out, "putch(");
    if (s->u.e) printA_Exp(out, s->u.e);
    fprintf(out, ");\n");
  }
  return;
}

/* */
void printA_Exp(FILE *out, A_exp e) {
#ifdef __DEBUG
  fprintf(out, "Entering printA_Exp...\n");
#endif
  if (!e) return;
  switch (e->kind) {
  case A_opExp:
    printA_OpExp(out, e);
    break;
  case A_numConst:
    printA_NumConst(out, e);
    break;
  case A_idExp:
    printA_IdExp(out, e);
    break;
  case A_minusExp:
    printA_MinusExp(out, e);
    break;
  case A_escExp:
    printA_EscExp(out, e);
    break;
  default:
    fprintf(out, "Unknown expression kind!\n");
    break;
  }
  return ;
}

/* */
void printA_OpExp(FILE *out, A_exp e) {
#ifdef __DEBUG
  fprintf(out, "Entering printA_OpExp...\n");
#endif
  if (!e) return;
  printA_Exp(out, e->u.op.left);
  switch (e->u.op.oper) {
  case A_plus:
    fprintf(out, "+");
    break;
  case A_minus:
    fprintf(out, "-");
    break;
  case A_times:
    fprintf(out, "*");
    break;
  case A_div:
    fprintf(out, "/");
    break;
  }
  printA_Exp(out, e->u.op.right);
  return;
}

/* */
void printA_NumConst(FILE *out, A_exp e) {
#ifdef __DEBUG
  fprintf(out, "Entering printA_NumConst...\n");
#endif
  if (!e) return;
  if (e->kind != A_numConst) fprintf(out, "Not Num constant!\n");
  else {
    fprintf(out, "%d", e->u.num);
  }
  return;
}

/* */
void printA_IdExp(FILE *out, A_exp e) {
#ifdef __DEBUG
  fprintf(out, "Entering printA_IdExp...\n");
#endif
  if (!e) return;
  if (e->kind != A_idExp) fprintf(out, "Not ID exp!\n");
  else {
    fprintf(out, "%s", e->u.v);
  }
  return;
}

/* */
void printA_MinusExp(FILE *out, A_exp e) {
#ifdef __DEBUG
  fprintf(out, "Entering printA_MinusExp...\n");
#endif
  if (!e) return;
  if (e->kind != A_minusExp) fprintf(out, "Not Minus exp!\n");
  else {
    fprintf(out, "-");
    printA_Exp(out, e->u.e);
  }
  return;
}

/* */
void printA_EscExp(FILE *out, A_exp e) {
#ifdef __DEBUG
  fprintf(out, "Entering printA_EscExp...\n");
#endif
  if (!e) return;
  if (e->kind != A_escExp) fprintf(out, "Not Esc exp!\n");
  else {
    fprintf(out, "({");
    printA_StmList(out, e->u.escExp.ns);
    fprintf(out, "} ");
    printA_Exp(out, e->u.escExp.exp);
    fprintf(out, ")");
  }
  return;
}
