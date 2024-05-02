#include "llvmgen.h"

#include <stdarg.h>

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
static char relOp_codes[][4] = {"eq",  "ne",  "slt", "sgt", "sle",
                                "sge", "ult", "ule", "ugt", "uge"};

static void munchStm(T_stm s);
static void munchSeqStm(T_stm s);
static void munchLabelStm(T_stm s);
static void munchJumpStm(T_stm s);
static void munchCjumpStm(T_stm s);
static void munchMoveStm(T_stm s);
static void munchExpStm(T_stm s);
static void munchReturnStm(T_stm s);

static Temp_temp munchExp(T_exp e);
static void munchExp_givendst(T_exp e, Temp_temp dst);
static void munchBinOpExp(T_exp e, Temp_temp dst);
static void munchMemExp(T_exp e, Temp_temp dst);
static Temp_temp munchTempExp(T_exp e);
static void munchNameExp(T_exp e, Temp_temp dst);
static void munchConstExp(T_exp e, Temp_temp dst);
static void munchCallExp(T_exp e, Temp_temp dst);
static void munchExtCallExp(T_exp e, Temp_temp dst);
static void munchCastExp(T_exp e, Temp_temp dst);

static string Stringf(char *fmt, ...);

AS_instrList llvmbody(T_stmList stmList) {
  if (!stmList) return NULL;
  iList = last = NULL;

  // **The following is to make up an example of the instruction selection
  // result.
  //   You are supposed to implement this function. When you are done, remove
  //   the following code Follow the instruction from the class and the book!
  Temp_label l = Temp_newlabel();
  string rd = checked_malloc(IR_MAXLEN);
  sprintf(rd, "%s:", Temp_labelstring(l));
  emit(AS_Label(rd, l));
  emit(AS_Oper("\%`d0 = add i64 \%`s0, \%`s1", TL(Temp_newtemp(T_int), NULL),
               TL(Temp_newtemp(T_int), TL(Temp_newtemp(T_int), NULL)), NULL));
  emit(AS_Oper("br label \%`j0", NULL, NULL, AS_Targets(LL(l, NULL))));
  /***** The above is to be removed! *****/

  return iList;
}

static void munchStm(T_stm s) {
  if (!s) return;
  assert(s->kind != T_SEQ);

  switch (s->kind) {
    case T_SEQ:
      munchSeqStm(s);
      break;
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

static void munchSeqStm(T_stm s) {
  if (!s) return;
  munchStm(s->u.SEQ.left);
  munchStm(s->u.SEQ.right);
}

static void munchLabelStm(T_stm s) {
  if (!s) return;
  emit(AS_Label(Stringf("%s:", Temp_labelstring(s->u.LABEL)), s->u.LABEL));
}

static void munchJumpStm(T_stm s) {
  if (!s) return;
  emit(AS_Oper("br label \%`j0", NULL, NULL,
               AS_Targets(LL(s->u.JUMP.jump, NULL))));
}

static void munchCjumpStm(T_stm s) {
  if (!s) return;
  Temp_temp cond = Temp_newtemp(T_int);

  T_exp left = s->u.CJUMP.left;
  T_exp right = s->u.CJUMP.right;
  string relop = String(relOp_codes[s->u.CJUMP.op]);

  if (left->type == T_int && right->type == T_int) {
    // emit icmp instruction
    if (left->kind == T_CONST && right->kind == T_CONST) {
      emit(AS_Oper(Stringf("%%`d0 = icmp %s i64 %d, %d", relop, left->u.CONST.i,
                           right->u.CONST.i),
                   TL(cond, NULL), NULL, NULL));
    } else if (left->kind == T_CONST) {
      emit(AS_Oper(
          Stringf("%%`d0 = icmp %s i64 %d, %%`s0", relop, left->u.CONST.i),
          TL(cond, NULL), TL(munchExp(right), NULL), NULL));
    } else if (right->kind == T_CONST) {
      emit(AS_Oper(
          Stringf("%%`d0 = icmp %s i64 %%`s0, %d", relop, right->u.CONST.i),
          TL(cond, NULL), TL(munchExp(left), NULL), NULL));
    } else {
      emit(AS_Oper(Stringf("%%`d0 = icmp %s i64 %%`s0, %%`s1", relop),
                   TL(cond, NULL),
                   TL(munchExp(left), TL(munchExp(right), NULL)), NULL));
    }
  } else {
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
      Temp_temp rightTemp = munchExp(right);
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
      Temp_temp leftTemp = munchExp(left);
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
      Temp_temp leftTemp = munchExp(left);
      if (leftTemp->type == T_int) {
        Temp_temp floatTemp = Temp_newtemp(T_float);
        emit(AS_Oper(Stringf("%%`d0 = sitofp i64 %%`s0 to double"),
                     TL(floatTemp, NULL), TL(leftTemp, NULL), NULL));
        leftTemp = floatTemp;
      }
      Temp_temp rightTemp = munchExp(right);
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

  emit(AS_Oper("br i1 %%`s0, label \%`j0, label \%`j1", NULL, TL(cond, NULL),
               AS_Targets(LL(s->u.CJUMP.t, LL(s->u.CJUMP.f, NULL)))));
}

static void munchMoveStm(T_stm s) {
  if (!s) return;
  T_exp dst = s->u.MOVE.dst;
  assert(dst->kind != T_ESEQ);
  T_exp src = s->u.MOVE.src;
  assert(src->kind != T_ESEQ);

  if (dst->kind == T_MEM) {  // store
    Temp_temp dstPtr = Temp_newtemp(T_int);
    if (dst->u.MEM->kind == T_CONST) {
      emit(AS_Oper(
          Stringf("%%`d0 = inttoptr i64 %d to i64*", dst->u.MEM->u.CONST.i),
          TL(dstPtr, NULL), NULL, NULL));
    } else {
      Temp_temp base = munchExp(dst->u.MEM);
      emit(AS_Oper("%%`d0 = inttoptr i64 %%`s0 to i64*", TL(dstPtr, NULL),
                   TL(base, NULL), NULL));
    }

    if (src->type == T_int) {
      if (src->kind == T_CONST) {
        emit(AS_Oper(
            Stringf("store i64 %d, i64* %%`s0, align 8", src->u.CONST.i), NULL,
            TL(dstPtr, NULL), NULL));
      } else {
        emit(AS_Oper("store i64 %%`s0, i64* %%`s1, align 8", NULL,
                     TL(munchExp(src), TL(dstPtr, NULL)), NULL));
      }
    } else {
      if (src->kind == T_CONST) {
        emit(AS_Oper(
            Stringf("store double %f, i64* %%`s0, align 8", src->u.CONST.f),
            NULL, TL(dstPtr, NULL), NULL));
      } else {
        emit(AS_Oper("store double %%`s0, i64* %%`s1, align 8", NULL,
                     TL(munchExp(src), TL(dstPtr, NULL)), NULL));
      }
    }
    // if instructions should be as: Mem() <- Mem()
    // then we need to first load then store
    // but when processing src exp(T_MEM), we have already added the load
    // instruction
  } else if (src->kind == T_MEM) {  // load
    Temp_temp dstTemp = munchExp(dst);

    Temp_temp srcPtr = Temp_newtemp(T_int);
    if (src->u.MEM->kind == T_CONST) {
      emit(AS_Oper(
          Stringf("%%`d0 = inttoptr i64 %d to i64*", src->u.MEM->u.CONST.i),
          TL(srcPtr, NULL), NULL, NULL));
    } else {
      Temp_temp base = munchExp(src->u.MEM);
      emit(AS_Oper("%%`d0 = inttoptr i64 %%`s0 to i64*", TL(srcPtr, NULL),
                   TL(base, NULL), NULL));
    }

    if (dst->type == T_int) {
      emit(AS_Oper("%%`d0 = load i64, i64* %%`s0, align 8", TL(dstTemp, NULL),
                   TL(srcPtr, NULL), NULL));
    } else {
      emit(AS_Oper("%%`d0 = load double, i64* %%`s0, align 8",
                   TL(dstTemp, NULL), TL(srcPtr, NULL), NULL));
    }
  } else {
    switch (src->kind) {
      case T_BINOP:
      case T_CALL:
      case T_ExtCALL:
      case T_CONST: {
        munchExp_givendst(src, munchExp(dst));
        break;
      }
      case T_TEMP:
      case T_NAME:
      case T_CAST: {
        if (dst->type == T_int) {
          emit(AS_Oper("%%`d0 = add i64 %%`s0, 0", TL(munchExp(dst), NULL),
                       TL(munchExp(src), NULL), NULL));
        } else {
          emit(AS_Oper("%%`d0 = fadd double %%`s0, 0.0",
                       TL(munchExp(dst), NULL), TL(munchExp(src), NULL), NULL));
        }
        break;
      }
      default: {
        fprintf(stderr, "munchMoveStm: invalid src kind\n");
        assert(0);
      }
    }
  }
}

static void munchExpStm(T_stm s) {
  if (!s) return;
  munchExp(s->u.EXP);
}

static void munchReturnStm(T_stm s) {
  if (!s) return;

  T_exp ret = s->u.EXP;
  if (ret->kind == T_CONST) {
    if (ret->type == T_int) {
      emit(AS_Oper(Stringf("ret i64 %d", ret->u.CONST.i), NULL, NULL, NULL));
    } else {
      emit(AS_Oper(Stringf("ret double %f", ret->u.CONST.f), NULL, NULL, NULL));
    }
  } else {
    if (ret->type == T_int) {
      emit(AS_Oper("ret i64 %%`s0", NULL, TL(munchExp(ret), NULL), NULL));
    } else {
      emit(AS_Oper("ret double %%`s0", NULL, TL(munchExp(ret), NULL), NULL));
    }
  }
}

static Temp_temp munchExp(T_exp e) {
  assert(e && e->type != T_ESEQ);

  switch (e->kind) {
    case T_BINOP: {
      Temp_temp t = Temp_newtemp(e->type);
      munchBinOpExp(e, t);
      return t;
    }
    case T_MEM: {
      Temp_temp t = Temp_newtemp(e->type);
      munchMemExp(e, t);
      return t;
    }
    case T_TEMP:
      return munchTempExp(e);
    case T_NAME: {
      Temp_temp t = Temp_newtemp(e->type);
      munchNameExp(e, t);
      return t;
    }
    case T_CONST: {
      Temp_temp t = Temp_newtemp(e->type);
      munchConstExp(e, t);
      return t;
    }
    case T_CALL: {
      Temp_temp t = Temp_newtemp(e->type);
      munchCallExp(e, t);
      return t;
    }
    case T_ExtCALL: {
      Temp_temp t = Temp_newtemp(e->type);
      munchExtCallExp(e, t);
      return t;
    }
    case T_CAST: {
      Temp_temp t = Temp_newtemp(e->type);
      munchCastExp(e, t);
      return t;
    }
    default: {
      fprintf(stderr, "munchExp: invalid exp kind\n");
      assert(0);
    }
  }
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