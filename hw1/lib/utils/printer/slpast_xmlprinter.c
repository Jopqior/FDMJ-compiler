#include "slpast_xmlprinter.h"

#define __DEBUG
#undef __DEBUG

void printX_MainMethod(FILE *out, A_mainMethod main);
void printX_StmList(FILE *out, A_stmList sl);
void printX_Stm(FILE *out, A_stm s);
void printX_AssignStm(FILE *out, A_stm s);
void printX_Putint(FILE *out, A_stm s);
void printX_Putch(FILE *out, A_stm s);
void printX_Exp(FILE *out, A_exp e);
void printX_OpExp(FILE *out, A_exp e);
void printX_NumConst(FILE *out, A_exp e);
void printX_IdExp(FILE *out, A_exp e);
void printX_MinusExp(FILE *out, A_exp e);
void printX_EscExp(FILE *out, A_exp e);

/* */
void printX_Prog(FILE *out, A_prog p) {
#ifdef __DEBUG
  fprintf(out, "Entering printX_Prog...\n");
#endif
  if (p->m) printX_MainMethod(out, p->m);
  else
    fprintf(out, "Error: There's no main class!\n");
  fflush(out);
  return;
}

/* */
void printX_Pos(FILE* out, A_pos pos) {
#ifdef __DEBUG
  fprintf(out, "Entering printX_Pos...\n");
#endif
  return ; //don't need to print position
}

/* */
void printX_MainMethod(FILE *out, A_mainMethod main) {
#ifdef __DEBUG
  fprintf(out, "Entering printX_MainMethod...\n");
#endif
  fprintf(out, "<main>\n");
  if (main->sl) printX_StmList(out, main->sl);
  fprintf(out, "</main>\n");
  return ;
}

/* */
void printX_StmList(FILE *out, A_stmList sl) {
#ifdef __DEBUG
  fprintf(out, "Entering printX_StmList...\n");
#endif
  if (!sl) return;
  printX_Stm(out, sl->head);
  if (sl->tail) printX_StmList(out, sl->tail);
  return;
}

/* */
void printX_Stm(FILE *out, A_stm s) {
#ifdef __DEBUG
  fprintf(out, "Entering printX_Stm...\n");
#endif
  if (!s) return;
  switch (s->kind) {
  case A_assignStm:
    printX_AssignStm(out, s);
    break;
  case A_putint:
    printX_Putint(out, s);
    break;
  case A_putch:
    printX_Putch(out, s);
    break;
  default:
    fprintf(out, "Unknown statement kind!");
    break;
  }
  return;
}

/* */
void printX_AssignStm(FILE *out, A_stm s) {
#ifdef __DEBUG
  fprintf(out, "Entering printX_AssignStm...\n");
#endif
  if (!s) return;
  if (s->kind != A_assignStm) fprintf(out, "Not an assign stm!\n");
  else {
    fprintf(out, "<assignment>\n<left_id>");
    printX_Exp(out, s->u.assign.id);
    fprintf(out, "</left_id>\n");
    /* March 20, 2023. Remove redundant grammar rule.. hence the ast struct
        if (s->u.assign.pos) { //this is an array position
          fprintf(out, "[");
          printX_Exp(out, s->u.assign.pos);
          fprintf(out, "]");
        }
    */
    //fprintf(out, "=");
    fprintf(out, "<right_exp>");
    printX_Exp(out, s->u.assign.value);
    fprintf(out, "</right_exp>\n</assignment>\n");
  }
  return;
}

/* */
void printX_Putint(FILE *out, A_stm s) {
#ifdef __DEBUG
  fprintf(out, "Entering printX_Putin...\n");
#endif
  if (s->kind != A_putint) fprintf(out, "Not a putint stm!\n");
  else {
    fprintf(out, "<putint>");
    if (s->u.e) printX_Exp(out, s->u.e);
    fprintf(out, "</putint>\n");
  }
  return;
}

/* */
void printX_Putch(FILE *out, A_stm s) {
#ifdef __DEBUG
  fprintf(out, "Entering printX_Putch...\n");
#endif
  if (s->kind != A_putch) fprintf(out, "Not a putch stm!\n");
  else {
    fprintf(out, "<putch>");
    if (s->u.e) printX_Exp(out, s->u.e);
    fprintf(out, ");\n");
  }
  return;
}

/* */
void printX_Exp(FILE *out, A_exp e) {
#ifdef __DEBUG
  fprintf(out, "Entering printX_Exp...\n");
#endif
  if (!e) return;
  switch (e->kind) {
  case A_opExp:
    printX_OpExp(out, e);
    break;
  case A_numConst:
    printX_NumConst(out, e);
    break;
  case A_idExp:
    printX_IdExp(out, e);
    break;
  case A_minusExp:
    printX_MinusExp(out, e);
    break;
  case A_escExp:
    printX_EscExp(out, e);
    break;
  default:
    fprintf(out, "Unknown expression kind!\n");
    break;
  }
  return ;
}

/* */
void printX_OpExp(FILE *out, A_exp e) {
#ifdef __DEBUG
  fprintf(out, "Entering printX_OpExp...\n");
#endif
  if (!e) return;
  fprintf(out, "<leftexpression>");
  printX_Exp(out, e->u.op.left);
  fprintf(out, "</leftexpression>\n");
  switch (e->u.op.oper) {
  case A_plus:
    fprintf(out, "<plus>+</plus>\n");
    break;
  case A_minus:
    fprintf(out, "<minus>-</minus>\n");
    break;
  case A_times:
    fprintf(out, "<mul>*</mul>\n");
    break;
  case A_div:
    fprintf(out, "<div>/</div>\n");
    break;
  }
  fprintf(out, "<rightexpression>");
  printX_Exp(out, e->u.op.right);
  fprintf(out, "</rightexpression>\n");
  return;
}

/* */
void printX_NumConst(FILE *out, A_exp e) {
#ifdef __DEBUG
  fprintf(out, "Entering printX_NumConst...\n");
#endif
  if (!e) return;
  if (e->kind != A_numConst) fprintf(out, "Not Num constant!\n");
  else {
    fprintf(out, "<num>%d</num>\n", e->u.num);
  }
  return;
}

/* */
void printX_IdExp(FILE *out, A_exp e) {
#ifdef __DEBUG
  fprintf(out, "Entering printX_IdExp...\n");
#endif
  if (!e) return;
  if (e->kind != A_idExp) fprintf(out, "Not ID exp!\n");
  else {
    fprintf(out, "<id>%s</id>\n", e->u.v);
  }
  return;
}

/* */
void printX_MinusExp(FILE *out, A_exp e) {
#ifdef __DEBUG
  fprintf(out, "Entering printX_MinusExp...\n");
#endif
  if (!e) return;
  if (e->kind != A_minusExp) fprintf(out, "Not Minus exp!\n");
  else {
    fprintf(out, "<minus>-\n");
    printX_Exp(out, e->u.e);
    fprintf(out, "</minus>\n");
  }
  return;
}

/* */
void printX_EscExp(FILE *out, A_exp e) {
#ifdef __DEBUG
  fprintf(out, "Entering printX_EscExp...\n");
#endif
  if (!e) return;
  if (e->kind != A_escExp) fprintf(out, "Not Esc exp!\n");
  else {
    fprintf(out, "<escExp>\n<escStmList>\n");
    printX_StmList(out, e->u.escExp.ns);
    fprintf(out, "</escStmList>\n");  
    fprintf(out, "<tailexp>");
    printX_Exp(out, e->u.escExp.exp);
    fprintf(out, "</tailexp>\n</escExp>\n");
  }
  return;
}
