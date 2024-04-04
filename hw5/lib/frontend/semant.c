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

static S_table venv;
static int whileDepth = 0;

void transA_MainMethod(FILE* out, A_mainMethod m);

void transA_VarDeclList(FILE* out, A_varDeclList vdl);
void transA_VarDecl(FILE* out, A_varDecl vd);

void transA_StmList(FILE* out, A_stmList sl);
void transA_Stm(FILE* out, A_stm s);

void transA_NestedStm(FILE* out, A_stm s);
void transA_IfStm(FILE* out, A_stm s);
void transA_WhileStm(FILE* out, A_stm s);
void transA_AssignStm(FILE* out, A_stm s);
void transA_ArrayInit(FILE* out, A_stm s);
void transA_Continue(FILE* out, A_stm s);
void transA_Break(FILE* out, A_stm s);
void transA_Return(FILE* out, A_stm s);
void transA_Putnum(FILE* out, A_stm s);
void transA_Putarray(FILE* out, A_stm s);
void transA_Putch(FILE* out, A_stm s);

void transA_ArrayInitExpList(FILE* out, A_expList el);

expty transA_Exp(FILE* out, A_exp e);
expty transA_OpExp(FILE* out, A_exp e);
expty transA_ArrayExp(FILE* out, A_exp e);
expty transA_BoolConst(FILE* out, A_exp e);
expty transA_NumConst(FILE* out, A_exp e);
expty transA_IdExp(FILE* out, A_exp e);
expty transA_LengthExp(FILE* out, A_exp e);
expty transA_NewIntArrExp(FILE* out, A_exp e);
expty transA_NewFloatArrExp(FILE* out, A_exp e);
expty transA_NotExp(FILE* out, A_exp e);
expty transA_MinusExp(FILE* out, A_exp e);
expty transA_EscExp(FILE* out, A_exp e);
expty transA_Getnum(FILE* out, A_exp e);
expty transA_Getch(FILE* out, A_exp e);
expty transA_Getarray(FILE* out, A_exp e);

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

  if (p->m) {
    transA_MainMethod(out, p->m);
  } else {
    transError(out, p->pos, String("Error: There's no main class"));
  }
}

void transA_MainMethod(FILE* out, A_mainMethod m) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_MainMethod...\n");
#endif
  S_beginScope(venv);
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
  fprintf(out, "Entering transA_VarDecList...\n");
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
    transError(out, vd->pos, String("Error: Variable already declared"));
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
        String("Error: If statement condition must be of type int or float"));
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
            "Error: While statement condition must be of type int or float"));
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
  expty right = transA_Exp(out, s->u.assign.value);
  if (!left || !right) return;

  // check if the left side is a lvalue
  if (!left->location) {
    transError(
        out, s->pos,
        String("Error: Left side of assignment must have a location value"));
  }

  // check if the types match
  if ((left->ty->kind == Ty_int || left->ty->kind == Ty_float) &&
      (right->ty->kind != Ty_int && right->ty->kind != Ty_float)) {
    if (left->ty->kind == Ty_int) {
      transError(out, s->pos,
                 String("Error: Right side of assignment must be of type int"));
    } else {
      transError(
          out, s->pos,
          String("Error: Right side of assignment must be of type float"));
    }
  }
  if (left->ty->kind == Ty_array) {
    if (right->ty->kind != Ty_array) {
      transError(
          out, s->pos,
          String("Error: Right side of assignment must be of type array"));
    }
    if (left->ty->u.array->kind != right->ty->u.array->kind) {
      transError(out, s->pos,
                 String("Error: Array types must match in assignment"));
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
        String("Error: Left side of array initialization must be an array"));
  }
  if (!arr->location) {
    transError(out, s->pos,
               String("Error: Left side of array initialization must have a "
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
        String("Error: Array initialization must be of type int or float"));
  }

  if (el->tail) {
    transA_ArrayInitExpList(out, el->tail);
  }
}

void transA_Continue(FILE* out, A_stm s) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_Continue...\n");
#endif
  if (!s) return;

  if (whileDepth == 0) {
    transError(out, s->pos,
               String("Error: Continue statement outside of loop"));
  }
}

void transA_Break(FILE* out, A_stm s) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_Break...\n");
#endif
  if (!s) return;

  if (whileDepth == 0) {
    transError(out, s->pos, String("Error: Break statement outside of loop"));
  }
}

void transA_Return(FILE* out, A_stm s) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_Return...\n");
#endif
  if (!s) return;

  expty ty = transA_Exp(out, s->u.e);
  if (!ty) return;
  if (ty->ty->kind != Ty_int && ty->ty->kind != Ty_float) {
    transError(
        out, s->pos,
        String(
            "Error: Return value of main method must be of type int or float"));
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
        String("Error: Argument of putnum() must be of type int or float"));
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
        String("Error: Argument of putch() must be of type int or float"));
  }
}

void transA_Putarray(FILE* out, A_stm s) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_Putarray...\n");
#endif
  if (!s) return;

  expty ty1 = transA_Exp(out, s->u.putarray.e1);
  expty ty2 = transA_Exp(out, s->u.putarray.e2);
  if (!ty1 || !ty2) return;
  if (ty1->ty->kind != Ty_int && ty1->ty->kind != Ty_float) {
    transError(out, s->pos,
               String("Error: First argument of putarray() must be of type int "
                      "or float"));
  }
  if (ty2->ty->kind != Ty_array) {
    transError(
        out, s->pos,
        String("Error: Second argument of putarray() must be of type array"));
  }
}

expty transA_Exp(FILE* out, A_exp e) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_Exp...\n");
#endif
  if (!e) return NULL;

  switch (e->kind) {
    case A_opExp:
      transA_OpExp(out, e);
      break;
    case A_arrayExp:
      transA_ArrayExp(out, e);
      break;
    case A_boolConst:
      transA_BoolConst(out, e);
      break;
    case A_numConst:
      transA_NumConst(out, e);
      break;
    case A_idExp:
      transA_IdExp(out, e);
      break;
    case A_lengthExp:
      transA_LengthExp(out, e);
      break;
    case A_newIntArrExp:
      transA_NewIntArrExp(out, e);
      break;
    case A_newFloatArrExp:
      transA_NewFloatArrExp(out, e);
      break;
    case A_notExp:
      transA_NotExp(out, e);
      break;
    case A_minusExp:
      transA_MinusExp(out, e);
      break;
    case A_escExp:
      transA_EscExp(out, e);
      break;
    case A_getnum:
      transA_Getnum(out, e);
      break;
    case A_getch:
      transA_Getch(out, e);
      break;
    case A_getarray:
      transA_Getarray(out, e);
      break;
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
  expty right = transA_Exp(out, e->u.op.right);
  if (!left || !right) return NULL;
  if (left->ty->kind != Ty_int && left->ty->kind != Ty_float) {
    transError(
        out, e->u.op.left->pos,
        String("Error: Left side of operator must be of type int or float"));
  }
  if (right->ty->kind != Ty_int && right->ty->kind != Ty_float) {
    transError(
        out, e->u.op.right->pos,
        String("Error: Right side of operator must be of type int or float"));
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
  expty pos = transA_Exp(out, e->u.array_pos.arr_pos);
  if (!arr || !pos) return NULL;
  if (arr->ty->kind != Ty_array) {
    transError(out, e->pos,
               String("Error: Left side of array access must be an array"));
  }
  if (pos->ty->kind != Ty_int && pos->ty->kind != Ty_float) {
    transError(
        out, e->pos,
        String(
            "Error: Right side of array access must be of type int or float"));
  }

  return Expty(TRUE, arr->ty->u.array);
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
    transError(out, e->pos, String("Error: Variable not declared"));
  }

  return Expty(TRUE, x->u.var.ty);
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
               String("Error: Argument of length() must be an array"));
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
        String("Error: New int array size must be of type int or float"));
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
        String("Error: New float array size must be of type int or float"));
  }

  return Expty(FALSE, Ty_Array(Ty_Float()));
}

expty transA_NotExp(FILE* out, A_exp e) {
#ifdef __DEBUG
  fprintf(out, "Entering transA_NotExp...\n");
#endif
  if (!e) return NULL;

  expty ty = transA_Exp(out, e->u.e);
  if (!ty) return NULL;
  if (ty->ty->kind != Ty_int && ty->ty->kind != Ty_float) {
    transError(out, e->pos, String("Error: ! must operate on int or float"));
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
    transError(out, e->pos, String("Error: - must operate on int or float"));
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
               String("Error: Argument of getarray() must be an array"));
  }

  return Expty(TRUE, Ty_Int());
}
