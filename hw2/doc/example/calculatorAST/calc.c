#include <stdio.h>
#include <stdlib.h>
#include "util.h"
#include "calc.h"

extern int yyparse();

A_stm A_AssignStm(int id, A_exp exp) {
  A_stm s = checked_malloc(sizeof *s);
  s->id=id; s->exp=exp;
  return s;
}

A_exp A_IdExp(int id) {
  A_exp e = checked_malloc(sizeof *e);
  e->kind=A_idExp; e->u.id=id;
  return e;
}

A_exp A_NumExp(int num) {
  A_exp e = checked_malloc(sizeof *e);
  e->kind=A_numExp; e->u.num=num;
  return e;
}

A_exp A_OpExp(A_exp left, A_binop oper, A_exp right) {
  A_exp e = checked_malloc(sizeof *e);
  e->kind=A_opExp; e->u.op.left=left; e->u.op.oper=oper; e->u.op.right=right;
  return e;
}

int main()
{
 return(yyparse());
}

/* Test prinStm()-------
A_exp exp1(void) {
    return A_OpExp(A_IdExp('c'-'a'), A_times, A_OpExp(A_NumExp(1), A_plus, A_NumExp(3)));
}
A_stm stm1(void) {
    return A_AssignStm('a', exp1());
}
int main(void) {
    printStm(stm1());
    printf("\n");
}
*/

