#include "llvmgen.h"

#include <stdarg.h>
#include <string.h>

#include "temp.h"
#include "util.h"

#define LLVMGEN_DEBUG
#undef LLVMGEN_DEBUG

// These are for collecting the AS instructions into a list (i.e., iList).
// last is the last instruction in ilist.
static AS_instrList iList = NULL, last = NULL;

static void emit(AS_instr inst) {
  if (last) {
    // add the instruction to the (nonempty) ilist to the end
    last = last->tail = AS_InstrList(inst, NULL);
  } else {
    // if this is the first instruction, make it the head of the list
    last = iList = AS_InstrList(inst, NULL);
  }
}

static Temp_tempList TL(Temp_temp t, Temp_tempList tl) {
  return Temp_TempList(t, tl);
}

static Temp_tempList TLS(Temp_tempList a, Temp_tempList b) {
  return Temp_TempListSplice(a, b);
}

static Temp_labelList LL(Temp_label l, Temp_labelList ll) {
  return Temp_LabelList(l, ll);
}

/* ********************************************************/
/* YOU ARE TO IMPLEMENT THE FOLLOWING FUNCTION FOR HW9_10 */
/* ********************************************************/
static char relOp_codes[2][10][4] = {
    {"eq", "ne", "slt", "sgt", "sle", "sge", "ult", "ule", "ugt", "uge"},
    {"oeq", "one", "olt", "ogt", "ole", "oge", "ult", "ule", "ugt", "uge"}};
static char binOp_codes[2][10][5] = {
    {"add", "sub", "mul", "sdiv", "and", "or", "shl", "lshr", "ashr", "xor"},
    {"fadd", "fsub", "fmul", "fdiv", "and", "or", "shl", "lshr", "ashr",
     "xor"}};

#ifdef LLVMGEN_DEBUG
static int depth = 0;
static void printIndent() {
  for (int i = 0; i < depth; i++) {
    fprintf(stderr, "  ");
  }
}
static char T_type2str[][12] = {"T_int", "T_float"};
static char bin_oper[][12] = {"T_plus", "T_minus", "T_mul",
                              "T_div",  "T_and",   "T_or"};
static char rel_oper[][12] = {"T_eq", "T_ne", "T_lt", "T_gt", "T_le", "T_ge"};
static char stm_kind[][12] = {"T_SEQ",  "T_LABEL", "T_JUMP",  "T_CJUMP",
                              "T_MOVE", "T_EXP",   "T_RETURN"};
static char exp_kind[][12] = {"T_BINOP", "T_MEM",     "T_TEMP",
                              "T_ESEQ",  "T_NAME",    "T_CONST",
                              "T_CALL",  "T_ExtCALL", "T_CAST"};
static void dump_TL(FILE *out, Temp_tempList tl) {
  for (Temp_tempList p = tl; p; p = p->tail) {
    fprintf(out, "%d: %s", p->head->num, T_type2str[p->head->type]);
    if (p->tail) {
      fprintf(out, ", ");
    }
  }
  fprintf(out, "\n");
}
#endif

static void munchStm(T_stm s);
static void munchLabelStm(T_stm s);
static void munchJumpStm(T_stm s);
static void munchCjumpStm(T_stm s);
static void munchMoveStm(T_stm s);
static void munchExpStm(T_stm s);
static void munchReturnStm(T_stm s);

static Temp_temp munchExp(T_exp e, Temp_temp dst);
static void munchBinOpExp(T_exp e, Temp_temp dst);
static void munchMemExp_load(T_exp e, Temp_temp dst);
static Temp_temp munchTempExp(T_exp e);
static void munchNameExp(T_exp e, Temp_temp dst);
static void munchConstExp(T_exp e, Temp_temp dst);
static void munchCallExp(T_exp e, Temp_temp dst);
static void munchExtCallExp(T_exp e, Temp_temp dst);
static Temp_tempList munchArgs(T_expList args, string argsStr, int initNo);
static void munchCastExp(T_exp e, Temp_temp dst);

static string Stringf(char *fmt, ...);

AS_instrList llvmbody(T_stmList stmList) {
#ifdef LLVMGEN_DEBUG
  printIndent();
  fprintf(stderr, "Entering llvmbody...\n");
  depth++;
#endif
  if (!stmList) return NULL;
  iList = last = NULL;

  for (T_stmList sl = stmList; sl; sl = sl->tail) {
    munchStm(sl->head);
  }

#ifdef LLVMGEN_DEBUG
  depth--;
  printIndent();
  fprintf(stderr, "Returning from llvmbody...\n");
#endif
  return iList;
}

static void munchStm(T_stm s) {
  if (!s) return;
  assert(s->kind != T_SEQ);

  switch (s->kind) {
    case T_LABEL:
      munchLabelStm(s);
      break;
    case T_JUMP:
      munchJumpStm(s);
      break;
    case T_CJUMP:
      munchCjumpStm(s);
      break;
    case T_MOVE:
      munchMoveStm(s);
      break;
    case T_EXP:
      munchExpStm(s);
      break;
    case T_RETURN:
      munchReturnStm(s);
      break;
    default:
      fprintf(stderr, "munchStm: unknown stm kind\n");
  }
}

static void munchLabelStm(T_stm s) {
#ifdef LLVMGEN_DEBUG
  printIndent();
  fprintf(stderr, "munchLabelStm: s->u.LABEL->name:%s\n",
          Temp_labelstring(s->u.LABEL));
  depth++;
#endif
  assert(s && s->kind == T_LABEL);
  emit(AS_Label(Stringf("%s:", Temp_labelstring(s->u.LABEL)), s->u.LABEL));
#ifdef LLVMGEN_DEBUG
  depth--;
#endif
}

static void munchJumpStm(T_stm s) {
#ifdef LLVMGEN_DEBUG
  printIndent();
  fprintf(stderr, "munchJumpStm: s->u.JUMP.jump->name:%s\n",
          Temp_labelstring(s->u.JUMP.jump));
  depth++;
#endif
  assert(s && s->kind == T_JUMP);
  emit(AS_Oper("br label \%`j0", NULL, NULL,
               AS_Targets(LL(s->u.JUMP.jump, NULL))));
#ifdef LLVMGEN_DEBUG
  depth--;
#endif
}

static void munchCjumpStm(T_stm s) {
#ifdef LLVMGEN_DEBUG
  printIndent();
  fprintf(stderr, "munchCjumpStm: left->type:%s, right->type:%s\n",
          T_type2str[s->u.CJUMP.left->type],
          T_type2str[s->u.CJUMP.right->type]);
  depth++;
#endif
  assert(s && s->kind == T_CJUMP);
  Temp_temp cond = Temp_newtemp(T_int);

  T_exp left = s->u.CJUMP.left;
  T_exp right = s->u.CJUMP.right;

  if (left->type == T_int && right->type == T_int) {
    string relop = String(relOp_codes[T_int][s->u.CJUMP.op]);

    // emit icmp instruction
    if (left->kind == T_CONST && right->kind == T_CONST) {
      emit(AS_Oper(Stringf("%%`d0 = icmp %s i64 %d, %d", relop, left->u.CONST.i,
                           right->u.CONST.i),
                   TL(cond, NULL), NULL, NULL));
    } else if (left->kind == T_CONST) {
      emit(AS_Oper(
          Stringf("%%`d0 = icmp %s i64 %d, %%`s0", relop, left->u.CONST.i),
          TL(cond, NULL), TL(munchExp(right, NULL), NULL), NULL));
    } else if (right->kind == T_CONST) {
      emit(AS_Oper(
          Stringf("%%`d0 = icmp %s i64 %%`s0, %d", relop, right->u.CONST.i),
          TL(cond, NULL), TL(munchExp(left, NULL), NULL), NULL));
    } else {
      emit(AS_Oper(
          Stringf("%%`d0 = icmp %s i64 %%`s0, %%`s1", relop), TL(cond, NULL),
          TL(munchExp(left, NULL), TL(munchExp(right, NULL), NULL)), NULL));
    }
  } else {
    string relop = String(relOp_codes[T_float][s->u.CJUMP.op]);
    // emit fcmp instruction
    if (left->kind == T_CONST && right->kind == T_CONST) {
      float leftConst =
          left->type == T_int ? (float)left->u.CONST.i : left->u.CONST.f;
      float rightConst =
          right->type == T_int ? (float)right->u.CONST.i : right->u.CONST.f;
      emit(AS_Oper(Stringf("%%`d0 = fcmp %s double %f, %f", relop, leftConst,
                           rightConst),
                   TL(cond, NULL), NULL, NULL));
    } else if (left->kind == T_CONST) {
      float leftConst =
          left->type == T_int ? (float)left->u.CONST.i : left->u.CONST.f;
      Temp_temp rightTemp = munchExp(right, NULL);
      if (rightTemp->type == T_int) {
        Temp_temp floatTemp = Temp_newtemp(T_float);
        emit(AS_Oper(Stringf("%%`d0 = sitofp i64 %%`s0 to double"),
                     TL(floatTemp, NULL), TL(rightTemp, NULL), NULL));
        rightTemp = floatTemp;
      }
      emit(
          AS_Oper(Stringf("%%`d0 = fcmp %s double %f, %%`s0", relop, leftConst),
                  TL(cond, NULL), TL(rightTemp, NULL), NULL));
    } else if (right->kind == T_CONST) {
      Temp_temp leftTemp = munchExp(left, NULL);
      if (leftTemp->type == T_int) {
        Temp_temp floatTemp = Temp_newtemp(T_float);
        emit(AS_Oper(Stringf("%%`d0 = sitofp i64 %%`s0 to double"),
                     TL(floatTemp, NULL), TL(leftTemp, NULL), NULL));
        leftTemp = floatTemp;
      }
      float rightConst =
          right->type == T_int ? (float)right->u.CONST.i : right->u.CONST.f;
      emit(AS_Oper(
          Stringf("%%`d0 = fcmp %s double %%`s0, %f", relop, rightConst),
          TL(cond, NULL), TL(leftTemp, NULL), NULL));
    } else {
      Temp_temp leftTemp = munchExp(left, NULL);
      if (leftTemp->type == T_int) {
        Temp_temp floatTemp = Temp_newtemp(T_float);
        emit(AS_Oper(Stringf("%%`d0 = sitofp i64 %%`s0 to double"),
                     TL(floatTemp, NULL), TL(leftTemp, NULL), NULL));
        leftTemp = floatTemp;
      }
      Temp_temp rightTemp = munchExp(right, NULL);
      if (rightTemp->type == T_int) {
        Temp_temp floatTemp = Temp_newtemp(T_float);
        emit(AS_Oper(Stringf("%%`d0 = sitofp i64 %%`s0 to double"),
                     TL(floatTemp, NULL), TL(rightTemp, NULL), NULL));
        rightTemp = floatTemp;
      }
      emit(AS_Oper(Stringf("%%`d0 = fcmp %s double %%`s0, %%`s1", relop),
                   TL(cond, NULL), TL(leftTemp, TL(rightTemp, NULL)), NULL));
    }
  }

  emit(AS_Oper("br i1 \%`s0, label \%`j0, label \%`j1", NULL, TL(cond, NULL),
               AS_Targets(LL(s->u.CJUMP.t, LL(s->u.CJUMP.f, NULL)))));
#ifdef LLVMGEN_DEBUG
  depth--;
#endif
}

static void munchMoveStm(T_stm s) {
#ifdef LLVMGEN_DEBUG
  printIndent();
  fprintf(stderr,
          "munchMoveStm: s->u.MOVE.dst->type:%s, s->u.MOVE.src->type:%s\n",
          T_type2str[s->u.MOVE.dst->type], T_type2str[s->u.MOVE.src->type]);
  depth++;
#endif
  assert(s && s->kind == T_MOVE);
  T_exp dst = s->u.MOVE.dst;
  assert(dst->kind == T_TEMP || dst->kind == T_MEM);
  T_exp src = s->u.MOVE.src;
  assert(src->kind != T_ESEQ);

  if (dst->kind == T_MEM) {  // store
    Temp_temp dstPtr = Temp_newtemp(T_int);
    if (dst->u.MEM->kind == T_CONST) {
      emit(AS_Oper(
          Stringf("%%`d0 = inttoptr i64 %d to ptr", dst->u.MEM->u.CONST.i),
          TL(dstPtr, NULL), NULL, NULL));
    } else {
      Temp_temp base = munchExp(dst->u.MEM, NULL);
      emit(AS_Oper("\%`d0 = inttoptr i64 \%`s0 to ptr", TL(dstPtr, NULL),
                   TL(base, NULL), NULL));
    }

    if (src->type == T_int) {
      if (src->kind == T_CONST) {
        emit(
            AS_Oper(Stringf("store i64 %d, ptr %%`s0, align 8", src->u.CONST.i),
                    NULL, TL(dstPtr, NULL), NULL));
      } else {
        emit(AS_Oper("store i64 \%`s0, ptr \%`s1, align 8", NULL,
                     TL(munchExp(src, NULL), TL(dstPtr, NULL)), NULL));
      }
    } else {
      if (src->kind == T_CONST) {
        emit(AS_Oper(
            Stringf("store double %f, ptr %%`s0, align 8", src->u.CONST.f),
            NULL, TL(dstPtr, NULL), NULL));
      } else {
        emit(AS_Oper("store double \%`s0, ptr \%`s1, align 8", NULL,
                     TL(munchExp(src, NULL), TL(dstPtr, NULL)), NULL));
      }
    }
    // if instructions should be as: Mem() <- Mem()
    // then we need to first load then store
    // but when processing src exp(T_MEM), we have already added the load
    // instruction
  } else if (src->kind == T_MEM) {  // load
    Temp_temp dstTemp = munchExp(dst, NULL);

    Temp_temp srcPtr = Temp_newtemp(T_int);
    if (src->u.MEM->kind == T_CONST) {
      emit(AS_Oper(
          Stringf("%%`d0 = inttoptr i64 %d to ptr", src->u.MEM->u.CONST.i),
          TL(srcPtr, NULL), NULL, NULL));
    } else {
      Temp_temp base = munchExp(src->u.MEM, NULL);
      emit(AS_Oper("\%`d0 = inttoptr i64 \%`s0 to ptr", TL(srcPtr, NULL),
                   TL(base, NULL), NULL));
    }

    if (dst->type == T_int) {
      emit(AS_Oper("\%`d0 = load i64, ptr \%`s0, align 8", TL(dstTemp, NULL),
                   TL(srcPtr, NULL), NULL));
    } else {
      emit(AS_Oper("\%`d0 = load double, ptr \%`s0, align 8", TL(dstTemp, NULL),
                   TL(srcPtr, NULL), NULL));
    }
  } else {
    switch (src->kind) {
      case T_BINOP:
      case T_CALL:
      case T_ExtCALL:
      case T_CONST: {
        munchExp(src, munchExp(dst, NULL));
        break;
      }
      case T_TEMP:
      case T_NAME:
      case T_CAST: {
        if (dst->type == T_int) {
          emit(AS_Oper("\%`d0 = add i64 \%`s0, 0",
                       TL(munchExp(dst, NULL), NULL),
                       TL(munchExp(src, NULL), NULL), NULL));
        } else {
          emit(AS_Oper("\%`d0 = fadd double \%`s0, 0.0",
                       TL(munchExp(dst, NULL), NULL),
                       TL(munchExp(src, NULL), NULL), NULL));
        }
        break;
      }
      default: {
        fprintf(stderr, "munchMoveStm: invalid src kind\n");
        assert(0);
      }
    }
  }
#ifdef LLVMGEN_DEBUG
  depth--;
#endif
}

static void munchExpStm(T_stm s) {
#ifdef LLVMGEN_DEBUG
  printIndent();
  fprintf(stderr, "munchExpStm\n");
  depth++;
#endif
  assert(s && s->kind == T_EXP);
  munchExp(s->u.EXP, NULL);
#ifdef LLVMGEN_DEBUG
  depth--;
#endif
}

static void munchReturnStm(T_stm s) {
#ifdef LLVMGEN_DEBUG
  printIndent();
  fprintf(stderr, "munchReturnStm\n");
  depth++;
#endif
  assert(s && s->kind == T_RETURN);

  T_exp ret = s->u.EXP;
  if (ret->kind == T_CONST) {
    if (ret->type == T_int) {
      emit(AS_Oper(Stringf("ret i64 %d", ret->u.CONST.i), NULL, NULL, NULL));
    } else {
      emit(AS_Oper(Stringf("ret double %f", ret->u.CONST.f), NULL, NULL, NULL));
    }
  } else {
    if (ret->type == T_int) {
      emit(AS_Oper("ret i64 \%`s0", NULL, TL(munchExp(ret, NULL), NULL), NULL));
    } else {
      emit(AS_Oper("ret double \%`s0", NULL, TL(munchExp(ret, NULL), NULL),
                   NULL));
    }
  }
#ifdef LLVMGEN_DEBUG
  depth--;
#endif
}

static Temp_temp munchExp(T_exp e, Temp_temp dst) {
  assert(e && e->type != T_ESEQ);

  Temp_temp t;
  switch (e->kind) {
    case T_BINOP: {
      Temp_temp t = dst ? dst : Temp_newtemp(e->type);
      munchBinOpExp(e, t);
      return t;
    }
    case T_MEM: {
      Temp_temp t = dst ? dst : Temp_newtemp(e->type);
      munchMemExp_load(e, t);
      return t;
    }
    case T_TEMP:
      return munchTempExp(e);
    case T_NAME: {
      Temp_temp t = dst ? dst : Temp_newtemp(e->type);
      munchNameExp(e, t);
      return t;
    }
    case T_CONST: {
      Temp_temp t = dst ? dst : Temp_newtemp(e->type);
      munchConstExp(e, t);
      return t;
    }
    case T_CALL: {
      Temp_temp t = dst ? dst : Temp_newtemp(e->type);
      munchCallExp(e, t);
      return t;
    }
    case T_ExtCALL: {
      Temp_temp t = dst ? dst : Temp_newtemp(e->type);
      munchExtCallExp(e, t);
      return t;
    }
    case T_CAST: {
      Temp_temp t = dst ? dst : Temp_newtemp(e->type);
      munchCastExp(e, t);
      return t;
    }
    default: {
      fprintf(stderr, "munchExp: invalid exp kind\n");
      assert(0);
    }
  }
}

static void munchBinOpExp(T_exp e, Temp_temp dst) {
#ifdef LLVMGEN_DEBUG
  printIndent();
  fprintf(stderr, "munchBinOpExp: e->type=%s, e->u.BINOP.op=%s\n",
          T_type2str[e->type], bin_oper[e->u.BINOP.op]);
  depth++;
#endif
  assert(e && e->kind == T_BINOP);

  T_exp left = e->u.BINOP.left;
  T_exp right = e->u.BINOP.right;
  string binop = String(binOp_codes[e->type][e->u.BINOP.op]);

  switch (e->type) {
    case T_int: {
      if (left->kind == T_CONST && right->kind == T_CONST) {
        emit(AS_Oper(Stringf("%%`d0 = %s i64 %d, %d", binop, left->u.CONST.i,
                             right->u.CONST.i),
                     TL(dst, NULL), NULL, NULL));
      } else if (left->kind == T_CONST) {
        emit(
            AS_Oper(Stringf("%%`d0 = %s i64 %d, %%`s0", binop, left->u.CONST.i),
                    TL(dst, NULL), TL(munchExp(right, NULL), NULL), NULL));
      } else if (right->kind == T_CONST) {
        emit(AS_Oper(
            Stringf("%%`d0 = %s i64 %%`s0, %d", binop, right->u.CONST.i),
            TL(dst, NULL), TL(munchExp(left, NULL), NULL), NULL));
      } else {
        emit(AS_Oper(
            Stringf("%%`d0 = %s i64 %%`s0, %%`s1", binop), TL(dst, NULL),
            TL(munchExp(left, NULL), TL(munchExp(right, NULL), NULL)), NULL));
      }

      break;
    }
    case T_float: {
      if (left->kind == T_CONST && right->kind == T_CONST) {
        float leftConst =
            left->type == T_int ? (float)left->u.CONST.i : left->u.CONST.f;
        float rightConst =
            right->type == T_int ? (float)right->u.CONST.i : right->u.CONST.f;
        emit(AS_Oper(
            Stringf("%%`d0 = %s double %f, %f", binop, leftConst, rightConst),
            TL(dst, NULL), NULL, NULL));
      } else if (left->kind == T_CONST) {
        float leftConst =
            left->type == T_int ? (float)left->u.CONST.i : left->u.CONST.f;
        Temp_temp rightTemp = munchExp(right, NULL);
        if (rightTemp->type == T_int) {
          Temp_temp floatTemp = Temp_newtemp(T_float);
          emit(AS_Oper(Stringf("%%`d0 = sitofp i64 %%`s0 to double"),
                       TL(floatTemp, NULL), TL(rightTemp, NULL), NULL));
          rightTemp = floatTemp;
        }
        emit(AS_Oper(Stringf("%%`d0 = %s double %f, %%`s0", binop, leftConst),
                     TL(dst, NULL), TL(rightTemp, NULL), NULL));
      } else if (right->kind == T_CONST) {
        Temp_temp leftTemp = munchExp(left, NULL);
        if (leftTemp->type == T_int) {
          Temp_temp floatTemp = Temp_newtemp(T_float);
          emit(AS_Oper(Stringf("%%`d0 = sitofp i64 %%`s0 to double"),
                       TL(floatTemp, NULL), TL(leftTemp, NULL), NULL));
          leftTemp = floatTemp;
        }
        float rightConst =
            right->type == T_int ? (float)right->u.CONST.i : right->u.CONST.f;
        emit(AS_Oper(Stringf("%%`d0 = %s double %%`s0, %f", binop, rightConst),
                     TL(dst, NULL), TL(leftTemp, NULL), NULL));
      } else {
        Temp_temp leftTemp = munchExp(left, NULL);
        if (leftTemp->type == T_int) {
          Temp_temp floatTemp = Temp_newtemp(T_float);
          emit(AS_Oper(Stringf("%%`d0 = sitofp i64 %%`s0 to double"),
                       TL(floatTemp, NULL), TL(leftTemp, NULL), NULL));
          leftTemp = floatTemp;
        }
        Temp_temp rightTemp = munchExp(right, NULL);
        if (rightTemp->type == T_int) {
          Temp_temp floatTemp = Temp_newtemp(T_float);
          emit(AS_Oper(Stringf("%%`d0 = sitofp i64 %%`s0 to double"),
                       TL(floatTemp, NULL), TL(rightTemp, NULL), NULL));
          rightTemp = floatTemp;
        }
        emit(AS_Oper(Stringf("%%`d0 = %s double %%`s0, %%`s1", binop),
                     TL(dst, NULL), TL(leftTemp, TL(rightTemp, NULL)), NULL));
      }

      break;
    }
    default:
      assert(0);
  }
#ifdef LLVMGEN_DEBUG
  depth--;
#endif
}

static void munchMemExp_load(T_exp e, Temp_temp dst) {
#ifdef LLVMGEN_DEBUG
  printIndent();
  fprintf(stderr, "munchMemExp_load: e->u.MEM->kind:%s\n",
          exp_kind[e->u.MEM->kind]);
  depth++;
#endif
  assert(e && e->kind == T_MEM);

  Temp_temp srcPtr = Temp_newtemp(e->type);
  if (e->u.MEM->kind == T_CONST) {
    emit(AS_Oper(Stringf("%%`d0 = inttoptr i64 %d to ptr", e->u.MEM->u.CONST.i),
                 TL(srcPtr, NULL), NULL, NULL));
  } else {
    Temp_temp base = munchExp(e->u.MEM, NULL);
    emit(AS_Oper("\%`d0 = inttoptr i64 \%`s0 to ptr", TL(srcPtr, NULL),
                 TL(base, NULL), NULL));
  }

  if (dst->type == T_int) {
    emit(AS_Oper("\%`d0 = load i64, ptr \%`s0, align 8", TL(dst, NULL),
                 TL(srcPtr, NULL), NULL));
  } else {
    emit(AS_Oper("\%`d0 = load double, ptr \%`s0, align 8", TL(dst, NULL),
                 TL(srcPtr, NULL), NULL));
  }
#ifdef LLVMGEN_DEBUG
  depth--;
#endif
}

static Temp_temp munchTempExp(T_exp e) {
#ifdef LLVMGEN_DEBUG
  printIndent();
  fprintf(stderr,
          "munchTempExp: e->u.TEMP->num:%d, e->u.TEMP->type:%s, e->type:%s\n",
          e->u.TEMP->num, T_type2str[e->u.TEMP->type], T_type2str[e->type]);
#endif
  assert(e && e->kind == T_TEMP);
  return e->u.TEMP;
#ifdef LLVMGEN_DEBUG
  depth--;
#endif
}

static void munchNameExp(T_exp e, Temp_temp dst) {
#ifdef LLVMGEN_DEBUG
  printIndent();
  fprintf(stderr, "munchNameExp: e->u.NAME->name:%s\n",
          Temp_labelstring(e->u.NAME));
  depth++;
#endif
  assert(e && e->kind == T_NAME);

  emit(AS_Oper(
      Stringf("%%`d0 = ptrtoint ptr @%s to i64", Temp_labelstring(e->u.NAME)),
      TL(dst, NULL), NULL, NULL));
#ifdef LLVMGEN_DEBUG
  depth--;
#endif
}

static void munchConstExp(T_exp e, Temp_temp dst) {
#ifdef LLVMGEN_DEBUG
  printIndent();
  if (e->type == T_int) {
    fprintf(stderr, "munchConstExp: int, e->u.CONST->i:%d\n", e->u.CONST.i);
  } else {
    fprintf(stderr, "munchConstExp: float, e->u.CONST->f:%f\n", e->u.CONST.f);
  }
  depth++;
#endif
  assert(e && e->kind == T_CONST);

  if (e->type == T_int) {
    emit(AS_Oper(Stringf("%%`d0 = add i64 %d, 0", e->u.CONST.i), TL(dst, NULL),
                 NULL, NULL));
  } else {
    emit(AS_Oper(Stringf("%%`d0 = fadd double %f, 0.0", e->u.CONST.f),
                 TL(dst, NULL), NULL, NULL));
  }
#ifdef LLVMGEN_DEBUG
  depth--;
#endif
}

static void munchCallExp(T_exp e, Temp_temp dst) {
#ifdef LLVMGEN_DEBUG
  printIndent();
  fprintf(stderr, "munchCallExp: e->u.CALL->id:%s\n", e->u.CALL.id);
  depth++;
#endif
  assert(e && e->kind == T_CALL);

  // get the method address
  assert(e->u.CALL.obj->kind == T_MEM);
  Temp_temp methAddr = munchExp(e->u.CALL.obj, NULL);
  Temp_temp methPtr = Temp_newtemp(T_int);
  emit(AS_Oper("\%`d0 = inttoptr i64 \%`s0 to ptr", TL(methPtr, NULL),
               TL(methAddr, NULL), NULL));

  // get the arguments
  string argsStr = String("");
  Temp_tempList args = munchArgs(e->u.CALL.args, argsStr, 1);
#ifdef LLVMGEN_DEBUG
  depth--;
  printIndent();
  fprintf(stderr, "munchCallExp: argsStr:%s\n", argsStr);
  printIndent();
  fprintf(stderr, "munchCallExp: args: ");
  dump_TL(stderr, TL(methPtr, args));
  depth++;
#endif

  emit(AS_Oper(Stringf("%%`d0 = call i64 %%`s0(%s)", argsStr), TL(dst, NULL),
               TL(methPtr, args), NULL));
#ifdef LLVMGEN_DEBUG
  depth--;
  printIndent();
  fprintf(stderr, "munchCallExp ===end\n");
#endif
}

static void munchExtCallExp(T_exp e, Temp_temp dst) {
#ifdef LLVMGEN_DEBUG
  printIndent();
  fprintf(stderr, "munchExtCallExp: e->u.ExtCALL->extfun:%s, dst->num:%d\n",
          e->u.ExtCALL.extfun, dst->num);
  depth++;
#endif
  assert(e && e->kind == T_ExtCALL);

  if (!strcmp("malloc", e->u.ExtCALL.extfun)) {
    Temp_temp ptr = Temp_newtemp(T_int);
    string argsStr = String("");
    Temp_tempList args = munchArgs(e->u.ExtCALL.args, argsStr, 0);
    emit(AS_Oper(Stringf("%%`d0 = call ptr @malloc(%s)", argsStr),
                 TL(ptr, NULL), args, NULL));
    emit(AS_Oper(Stringf("%%`d0 = ptrtoint ptr %%`s0 to i64"), TL(dst, NULL),
                 TL(ptr, NULL), NULL));
  } else if (!strcmp("getint", e->u.ExtCALL.extfun) ||
             !strcmp("getch", e->u.ExtCALL.extfun)) {
    Temp_temp num = Temp_newtemp(T_int);
    emit(AS_Oper(Stringf("%%`d0 = call i64 @%s()", e->u.ExtCALL.extfun),
                 TL(num, NULL), NULL, NULL));
  } else if (!strcmp("getfloat", e->u.ExtCALL.extfun)) {
    Temp_temp num = Temp_newtemp(T_float);
    emit(AS_Oper("\%`d0 = call double @getfloat()", TL(num, NULL), NULL, NULL));
  } else if (!strcmp("getarray", e->u.ExtCALL.extfun) ||
             !strcmp("getfarray", e->u.ExtCALL.extfun)) {
    Temp_temp num = Temp_newtemp(T_int);
    string argsStr = String("");
    Temp_tempList args = munchArgs(e->u.ExtCALL.args, argsStr, 0);
    emit(AS_Oper(
        Stringf("%%`d0 = call i64 @%s(%s)", e->u.ExtCALL.extfun, argsStr),
        TL(num, NULL), args, NULL));
  } else if (!strcmp("putint", e->u.ExtCALL.extfun) ||
             !strcmp("putfloat", e->u.ExtCALL.extfun) ||
             !strcmp("putch", e->u.ExtCALL.extfun) ||
             !strcmp("putarray", e->u.ExtCALL.extfun) ||
             !strcmp("putfarray", e->u.ExtCALL.extfun)) {
    string argsStr = String("");
    Temp_tempList args = munchArgs(e->u.ExtCALL.args, argsStr, 0);
    emit(AS_Oper(Stringf("call void @%s(%s)", e->u.ExtCALL.extfun, argsStr),
                 NULL, args, NULL));
  } else if (!strcmp("starttime", e->u.ExtCALL.extfun) ||
             !strcmp("stoptime", e->u.ExtCALL.extfun)) {
    emit(AS_Oper(Stringf("call void @%s()", e->u.ExtCALL.extfun), NULL, NULL,
                 NULL));
  } else {
    assert(0);
  }
#ifdef LLVMGEN_DEBUG
  depth--;
#endif
}

static Temp_tempList munchArgs(T_expList args, string argsStr, int initNo) {
  if (!args) return NULL;

  T_exp arg = args->head;
  if (arg->kind == T_CONST) {
    if (strcmp(argsStr, "")) {
      strcat(argsStr, ", ");
    }
    switch (arg->type) {
      case T_int:
        strcat(argsStr, Stringf("i64 %d", arg->u.CONST.i));
        break;
      case T_float:
        strcat(argsStr, Stringf("double %f", arg->u.CONST.f));
        break;
      default:
        assert(0);
    }
    return munchArgs(args->tail, argsStr, initNo);
  } else {
    Temp_temp argTemp = munchExp(arg, NULL);
    if (strcmp(argsStr, "")) {
      strcat(argsStr, ", ");
    }
    switch (argTemp->type) {
      case T_int:
        strcat(argsStr, Stringf("i64 %%`s%d", initNo));
        break;
      case T_float:
        strcat(argsStr, Stringf("double %%`s%d", initNo));
        break;
      default:
        assert(0);
    }
    return TL(argTemp, munchArgs(args->tail, argsStr, initNo + 1));
  }
}

static void munchCastExp(T_exp e, Temp_temp dst) {
#ifdef LLVMGEN_DEBUG
  printIndent();
  fprintf(stderr, "munchCastExp: e->u.CAST->type:%s, e->type:%s\n",
          T_type2str[e->u.CAST->type], T_type2str[e->type]);
  depth++;
#endif
  assert(e && e->kind == T_CAST);

  Temp_temp src = munchExp(e->u.CAST, NULL);
  if (e->type == T_int) {
    emit(AS_Oper(Stringf("%%`d0 = fptosi double %%`s0 to i64"), TL(dst, NULL),
                 TL(src, NULL), NULL));
  } else {
    emit(AS_Oper(Stringf("%%`d0 = sitofp i64 %%`s0 to double"), TL(dst, NULL),
                 TL(src, NULL), NULL));
  }
#ifdef LLVMGEN_DEBUG
  depth--;
#endif
}

static char buf[IR_MAXLEN];
static string Stringf(char *fmt, ...) {
  va_list argp;
  va_start(argp, fmt);
  vsnprintf(buf, IR_MAXLEN, fmt, argp);
  va_end(argp);
  return String(buf);
}

/* ********************************************************/
/* YOU ARE TO IMPLEMENT THE ABOVE FUNCTION FOR HW9_10 */
/* ********************************************************/

/* The following are some auxiliary functions to be used by the main */

// This function is to make the beginning of the function that jumps to the
// beginning label (lbeg) of a block (in case the lbeg is NOT at the beginning
// of the block).
AS_instrList llvmbodybeg(Temp_label lbeg) {
  if (!lbeg) return NULL;
  iList = last = NULL;
  Temp_label lstart = Temp_newlabel();
  string ir = (string)checked_malloc(IR_MAXLEN);
  sprintf(ir, "%s:", Temp_labelstring(lstart));
  emit(AS_Label(ir, lstart));
  ir = (string)checked_malloc(IR_MAXLEN);
  sprintf(ir, "br label %%`j0");
  emit(AS_Oper(ir, NULL, NULL, AS_Targets(LL(lbeg, NULL))));
  return iList;
}

// This function is to make the prolog of the function that takes the method
// name and the arguments.
// WE ARE MISSING THE RETURN TYPE in tigherirp.h. YOU NEED TO ADD IT!
AS_instrList llvmprolog(string methodname, Temp_tempList args, T_type rettype) {
#ifdef LLVMGEN_DEBUG
  fprintf(stderr, "llvmprolog: methodname=%s, rettype=%d\n", methodname,
          rettype);
#endif
  if (!methodname) return NULL;
  iList = last = NULL;
  string ir = (string)checked_malloc(IR_MAXLEN);
  if (rettype == T_int)
    sprintf(ir, "define i64 @%s(", methodname);
  else if (rettype == T_float)
    sprintf(ir, "define double @%s(", methodname);
#ifdef LLVMGEN_DEBUG
  fprintf(stderr, "llvmprolog: ir=%s\n", ir);
#endif
  if (args) {
    int i = 0;
    for (Temp_tempList arg = args; arg; arg = arg->tail, i += 1) {
      if (i != 0) sprintf(ir + strlen(ir), ", ");
      if (arg->head->type == T_int) {
        sprintf(ir + strlen(ir), "i64 %%`s%d", i);
#ifdef LLVMGEN_DEBUG
        fprintf(stderr, "%d, llvmprolog: ir=%s\n", i, ir);
#endif
      } else if (arg->head->type == T_float) {
#ifdef LLVMGEN_DEBUG
        fprintf(stderr, "%d, llvmprolog: ir=%s\n", i, ir);
#endif
        sprintf(ir + strlen(ir), "double %%`s%d", i);
      }
#ifdef LLVMGEN_DEBUG
      fprintf(stderr, "llvmprolog args: arg=%s\n", ir);
#endif
    }
  }
  sprintf(ir + strlen(ir), ") {");
#ifdef LLVMGEN_DEBUG
  fprintf(stderr, "llvmprolog final: ir=%s\n", ir);
#endif
  emit(AS_Oper(ir, NULL, args, NULL));
  return iList;
}

// This function is to make the epilog of the function that jumps to the end
// label (lend) of a block
AS_instrList llvmepilog(Temp_label lend) {
  if (!lend) return NULL;
  iList = last = NULL;
  // string ir = (string) checked_malloc(IR_MAXLEN);
  // sprintf(ir, "%s:", Temp_labelstring(lend));
  // emit(AS_Label(ir, lend));
  // emit(AS_Oper("ret i64 -1", NULL, NULL, NULL));
  emit(AS_Oper("}", NULL, NULL, NULL));
  return iList;
}