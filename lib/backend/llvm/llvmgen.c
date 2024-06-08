#include "llvmgen.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "temp.h"
#include "util.h"

#define ASSERT_DEBUG

#ifdef ASSERT_DEBUG
#define ASSERT(condition)         \
  do {                            \
    if (condition)                \
      NULL;                       \
    else                          \
      Assert(__FILE__, __LINE__); \
  } while (0)
#else
#define ASSERT(condition) \
  do {                    \
    NULL;                 \
  } while (0)
#endif

void Assert(char *filename, unsigned int lineno) {
  fflush(stdout);
  fprintf(stderr, "\nAssert failed： %s, line %u\n", filename, lineno);
  fflush(stderr);
  abort();
}

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

static char llvm_types[2][7] = {"i64", "double"};
static char relOp_codes[2][10][4] = {
    {"eq", "ne", "slt", "sgt", "sle", "sge", "ult", "ule", "ugt", "uge"},
    {"oeq", "one", "olt", "ogt", "ole", "oge", "ult", "ule", "ugt", "uge"}};
static char binOp_codes[2][10][5] = {
    {"add", "sub", "mul", "sdiv", "and", "or", "shl", "lshr", "ashr", "xor"},
    {"fadd", "fsub", "fmul", "fdiv", "and", "or", "shl", "lshr", "ashr",
     "xor"}};

typedef struct expres_ *expres;
struct expres_ {
  enum { res_const, res_temp } kind;
  T_type type;
  union {
    int i;
    double f;
    Temp_temp t;
  } u;
};
static expres IntConstRes(int i) {
#ifdef LLVMGEN_DEBUG
  printIndent();
  fprintf(stderr, "IntConstRes: i=%d\n", i);
#endif
  expres res = checked_malloc(sizeof(*res));
  res->kind = res_const;
  res->type = T_int;
  res->u.i = i;
  return res;
}
static expres FloatConstRes(float f) {
#ifdef LLVMGEN_DEBUG
  printIndent();
  fprintf(stderr, "FloatConstRes: f=%f\n", f);
#endif
  expres res = checked_malloc(sizeof(*res));
  res->kind = res_const;
  res->type = T_float;
  res->u.f = f;
  return res;
}
static expres TempRes(Temp_temp t) {
#ifdef LLVMGEN_DEBUG
  printIndent();
  fprintf(stderr, "TempRes: t->num=%d, t->type=%s\n", t->num,
          T_type2str[t->type]);
#endif
  expres res = checked_malloc(sizeof(*res));
  res->kind = res_temp;
  res->type = t->type;
  res->u.t = t;
  return res;
}

bool OPT_CONST = TRUE;
bool OPT_CAST_CONST = TRUE;

static void munchStm(T_stm s);
static void munchLabelStm(T_stm s);
static void munchJumpStm(T_stm s);
static void munchCjumpStm(T_stm s);
static void munchMoveStm(T_stm s);
static void munchExpStm(T_stm s);
static void munchReturnStm(T_stm s);

static expres munchExp(T_exp e, Temp_temp dst);
static expres munchBinOpExp(T_exp e, Temp_temp dst);
static expres munchMemExp_load(T_exp e, Temp_temp dst);
static expres munchTempExp(T_exp e);
static expres munchNameExp(T_exp e, Temp_temp dst);
static expres munchConstExp(T_exp e, Temp_temp dst);
static expres munchCallExp(T_exp e, Temp_temp dst);
static expres munchExtCallExp(T_exp e, Temp_temp dst);
static Temp_tempList munchArgs(T_expList args, string argsStr, int initNo);
static expres munchCastExp(T_exp e, Temp_temp dst);

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
  ASSERT(s->kind != T_SEQ);

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
    default: {
      fprintf(stderr, "munchStm: unknown stm kind\n");
      ASSERT(0);
    }
  }
}

static void munchLabelStm(T_stm s) {
#ifdef LLVMGEN_DEBUG
  printIndent();
  fprintf(stderr, "munchLabelStm: s->u.LABEL->name:%s\n",
          Temp_labelstring(s->u.LABEL));
  depth++;
#endif
  ASSERT(s && s->kind == T_LABEL);
  if (!s) return;
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
  ASSERT(s && s->kind == T_JUMP);
  if (!s) return;
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
  ASSERT(s && s->kind == T_CJUMP);
  if (!s) return;

  ASSERT(s->u.CJUMP.left->type == s->u.CJUMP.right->type);

  expres left = munchExp(s->u.CJUMP.left, NULL);
  expres right = munchExp(s->u.CJUMP.right, NULL);
  ASSERT(left->type == right->type && left->type == s->u.CJUMP.left->type);

  string relop = String(relOp_codes[left->type][s->u.CJUMP.op]);
  Temp_temp cond = Temp_newtemp(T_int);
  if (left->kind == res_const && right->kind == res_const) {
    switch (left->type) {
      case T_int: {
        emit(AS_Oper(
            Stringf("%%`d0 = icmp %s i64 %d, %d", relop, left->u.i, right->u.i),
            TL(cond, NULL), NULL, NULL));
        break;
      }
      case T_float: {
        emit(AS_Oper(Stringf("%%`d0 = fcmp %s double %f, %f", relop, left->u.f,
                             right->u.f),
                     TL(cond, NULL), NULL, NULL));
        break;
      }
      default:
        ASSERT(0);
    }
  } else if (left->kind == res_const) {
    switch (left->type) {
      case T_int: {
        emit(AS_Oper(Stringf("%%`d0 = icmp %s i64 %d, %%`s0", relop, left->u.i),
                     TL(cond, NULL), TL(right->u.t, NULL), NULL));
        break;
      }
      case T_float: {
        emit(AS_Oper(
            Stringf("%%`d0 = fcmp %s double %f, %%`s0", relop, left->u.f),
            TL(cond, NULL), TL(right->u.t, NULL), NULL));
        break;
      }
      default:
        ASSERT(0);
    }
  } else if (right->kind == res_const) {
    switch (left->type) {
      case T_int: {
        emit(
            AS_Oper(Stringf("%%`d0 = icmp %s i64 %%`s0, %d", relop, right->u.i),
                    TL(cond, NULL), TL(left->u.t, NULL), NULL));
        break;
      }
      case T_float: {
        emit(AS_Oper(
            Stringf("%%`d0 = fcmp %s double %%`s0, %f", relop, right->u.f),
            TL(cond, NULL), TL(left->u.t, NULL), NULL));
        break;
      }
      default:
        ASSERT(0);
    }
  } else {
    switch (left->type) {
      case T_int: {
        emit(AS_Oper(Stringf("%%`d0 = icmp %s i64 %%`s0, %%`s1", relop),
                     TL(cond, NULL), TL(left->u.t, TL(right->u.t, NULL)),
                     NULL));
        break;
      }
      case T_float: {
        emit(AS_Oper(Stringf("%%`d0 = fcmp %s double %%`s0, %%`s1", relop),
                     TL(cond, NULL), TL(left->u.t, TL(right->u.t, NULL)),
                     NULL));
        break;
      }
      default:
        ASSERT(0);
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
  ASSERT(s && s->kind == T_MOVE);
  if (!s) return;

  T_exp dst = s->u.MOVE.dst;
  ASSERT(dst->kind == T_TEMP || dst->kind == T_MEM);
  T_exp src = s->u.MOVE.src;
  ASSERT(dst->type == src->type);

  if (dst->kind == T_MEM) {  // store
    expres dstAddr = munchExp(dst->u.MEM, NULL);
    Temp_temp dstPtr = Temp_newtemp(T_int);
    if (dstAddr->kind == res_const) {
      emit(AS_Oper(Stringf("%%`d0 = inttoptr i64 %d to i64*", dstAddr->u.i),
                   TL(dstPtr, NULL), NULL, NULL));
    } else {
      emit(AS_Oper("\%`d0 = inttoptr i64 \%`s0 to i64*", TL(dstPtr, NULL),
                   TL(dstAddr->u.t, NULL), NULL));
    }

    expres srcValue = munchExp(src, NULL);
    if (srcValue->kind == res_const) {
      switch (src->type) {
        case T_int: {
          emit(AS_Oper(Stringf("store i64 %d, i64* %%`s0", srcValue->u.i), NULL,
                       TL(dstPtr, NULL), NULL));
          break;
        }
        case T_float: {
          emit(AS_Oper(Stringf("store double %f, i64* %%`s0", srcValue->u.f),
                       NULL, TL(dstPtr, NULL), NULL));
          break;
        }
        default:
          ASSERT(0);
      }
    } else {
      switch (src->type) {
        case T_int: {
          emit(AS_Oper("store i64 \%`s0, i64* \%`s1", NULL,
                       TL(srcValue->u.t, TL(dstPtr, NULL)), NULL));
          break;
        }
        case T_float: {
          emit(AS_Oper("store double \%`s0, i64* \%`s1", NULL,
                       TL(srcValue->u.t, TL(dstPtr, NULL)), NULL));
          break;
        }
        default:
          ASSERT(0);
      }
    }
    // if instructions should be as: Mem() <- Mem()
    // then we need to first load then store
    // but when processing src exp(T_MEM), we have already added the load
    // instruction
  } else {
    switch (src->kind) {
      case T_BINOP:
      case T_MEM:
      case T_NAME:
      case T_CALL:
      case T_ExtCALL:
      case T_CONST:
      case T_CAST: {
        munchExp(src, dst->u.TEMP);
        break;
      }
      case T_TEMP: {
        switch (dst->type) {
          case T_int: {
            emit(AS_Move("\%`d0 = add i64 \%`s0, 0", TL(dst->u.TEMP, NULL),
                         TL(src->u.TEMP, NULL)));
            break;
          }
          case T_float: {
            emit(AS_Move("\%`d0 = fadd double \%`s0, 0.0",
                         TL(dst->u.TEMP, NULL), TL(src->u.TEMP, NULL)));
            break;
          }
          default:
            ASSERT(0);
        }
        break;
      }
      default: {
        fprintf(stderr, "munchMoveStm: invalid src kind\n");
        ASSERT(0);
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
  ASSERT(s && s->kind == T_EXP);
  if (!s) return;
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
  ASSERT(s && s->kind == T_RETURN);

  Temp_label exitLabel = Temp_newlabel_prefix('Z');
  AS_targets targets = AS_Targets(Temp_LabelList(exitLabel, NULL));

  expres ret = munchExp(s->u.EXP, NULL);
  if (ret->kind == res_const) {
    switch (ret->type) {
      case T_int: {
        emit(AS_Oper(Stringf("ret i64 %d", ret->u.i), NULL, NULL, targets));
        break;
      }
      case T_float: {
        emit(AS_Oper(Stringf("ret double %f", ret->u.f), NULL, NULL, targets));
        break;
      }
      default:
        ASSERT(0);
    }
  } else {
    switch (ret->type) {
      case T_int: {
        emit(AS_Oper("ret i64 \%`s0", NULL, TL(ret->u.t, NULL), targets));
        break;
      }
      case T_float: {
        emit(AS_Oper("ret double \%`s0", NULL, TL(ret->u.t, NULL), targets));
        break;
      }
      default:
        ASSERT(0);
    }
  }
#ifdef LLVMGEN_DEBUG
  depth--;
#endif
}

static expres munchExp(T_exp e, Temp_temp dst) {
  ASSERT(e && e->type != T_ESEQ);

  switch (e->kind) {
    case T_BINOP: {
      return munchBinOpExp(e, dst);
    }
    case T_MEM: {
      return munchMemExp_load(e, dst);
    }
    case T_TEMP: {
      return munchTempExp(e);
    }
    case T_NAME: {
      return munchNameExp(e, dst);
    }
    case T_CONST: {
      return munchConstExp(e, dst);
    }
    case T_CALL: {
      return munchCallExp(e, dst);
    }
    case T_ExtCALL: {
      return munchExtCallExp(e, dst);
    }
    case T_CAST: {
      return munchCastExp(e, dst);
    }
    default: {
      fprintf(stderr, "munchExp: invalid exp kind\n");
      ASSERT(0);
    }
  }
}

static expres munchBinOpExp(T_exp e, Temp_temp dst) {
#ifdef LLVMGEN_DEBUG
  printIndent();
  fprintf(stderr, "munchBinOpExp: e->type=%s, e->u.BINOP.op=%s\n",
          T_type2str[e->type], bin_oper[e->u.BINOP.op]);
  depth++;
#endif
  ASSERT(e && e->kind == T_BINOP);
  ASSERT(e->u.BINOP.left && e->u.BINOP.right &&
         e->u.BINOP.left->type == e->u.BINOP.right->type);

  expres left = munchExp(e->u.BINOP.left, NULL);
  expres right = munchExp(e->u.BINOP.right, NULL);
  string binop = String(binOp_codes[e->type][e->u.BINOP.op]);

  if (!dst) {
    dst = Temp_newtemp(e->type);
  }

  if (left->kind == res_const && right->kind == res_const) {
    switch (e->type) {
      case T_int: {
        emit(AS_Oper(
            Stringf("%%`d0 = %s i64 %d, %d", binop, left->u.i, right->u.i),
            TL(dst, NULL), NULL, NULL));
        break;
      }
      case T_float: {
        emit(AS_Oper(
            Stringf("%%`d0 = %s double %f, %f", binop, left->u.f, right->u.f),
            TL(dst, NULL), NULL, NULL));
        break;
      }
      default:
        ASSERT(0);
    }
  } else if (left->kind == res_const) {
    switch (e->type) {
      case T_int: {
        emit(AS_Oper(Stringf("%%`d0 = %s i64 %d, %%`s0", binop, left->u.i),
                     TL(dst, NULL), TL(right->u.t, NULL), NULL));
        break;
      }
      case T_float: {
        emit(AS_Oper(Stringf("%%`d0 = %s double %f, %%`s0", binop, left->u.f),
                     TL(dst, NULL), TL(right->u.t, NULL), NULL));
        break;
      }
      default:
        ASSERT(0);
    }
  } else if (right->kind == res_const) {
    switch (e->type) {
      case T_int: {
        emit(AS_Oper(Stringf("%%`d0 = %s i64 %%`s0, %d", binop, right->u.i),
                     TL(dst, NULL), TL(left->u.t, NULL), NULL));
        break;
      }
      case T_float: {
        emit(AS_Oper(Stringf("%%`d0 = %s double %%`s0, %f", binop, right->u.f),
                     TL(dst, NULL), TL(left->u.t, NULL), NULL));
        break;
      }
      default:
        ASSERT(0);
    }
  } else {
    switch (e->type) {
      case T_int: {
        emit(AS_Oper(Stringf("%%`d0 = %s i64 %%`s0, %%`s1", binop),
                     TL(dst, NULL), TL(left->u.t, TL(right->u.t, NULL)), NULL));
        break;
      }
      case T_float: {
        emit(AS_Oper(Stringf("%%`d0 = %s double %%`s0, %%`s1", binop),
                     TL(dst, NULL), TL(left->u.t, TL(right->u.t, NULL)), NULL));
        break;
      }
      default:
        ASSERT(0);
    }
  }
#ifdef LLVMGEN_DEBUG
  depth--;
#endif
  return TempRes(dst);
}

static expres munchMemExp_load(T_exp e, Temp_temp dst) {
#ifdef LLVMGEN_DEBUG
  printIndent();
  fprintf(stderr, "munchMemExp_load: e->u.MEM->kind:%s\n",
          exp_kind[e->u.MEM->kind]);
  depth++;
#endif
  ASSERT(e && e->kind == T_MEM && e->u.MEM->type == T_int);

  expres srcAddr = munchExp(e->u.MEM, NULL);
  Temp_temp srcPtr = Temp_newtemp(T_int);

  if (srcAddr->kind == res_const) {
    emit(AS_Oper(Stringf("%%`d0 = inttoptr i64 %d to i64*", srcAddr->u.i),
                 TL(srcPtr, NULL), NULL, NULL));
  } else {
    emit(AS_Oper("\%`d0 = inttoptr i64 \%`s0 to i64*", TL(srcPtr, NULL),
                 TL(srcAddr->u.t, NULL), NULL));
  }

  if (!dst) {
    dst = Temp_newtemp(e->type);
  }
  if (dst->type == T_int) {
    emit(AS_Oper("\%`d0 = load i64, i64* \%`s0", TL(dst, NULL),
                 TL(srcPtr, NULL), NULL));
  } else {
    emit(AS_Oper("\%`d0 = load double, i64* \%`s0", TL(dst, NULL),
                 TL(srcPtr, NULL), NULL));
  }
#ifdef LLVMGEN_DEBUG
  depth--;
#endif
  return TempRes(dst);
}

static expres munchTempExp(T_exp e) {
#ifdef LLVMGEN_DEBUG
  printIndent();
  fprintf(stderr,
          "munchTempExp: e->u.TEMP->num:%d, e->u.TEMP->type:%s, e->type:%s\n",
          e->u.TEMP->num, T_type2str[e->u.TEMP->type], T_type2str[e->type]);
#endif
  ASSERT(e && e->kind == T_TEMP);
  return TempRes(e->u.TEMP);
#ifdef LLVMGEN_DEBUG
  depth--;
#endif
}

static expres munchNameExp(T_exp e, Temp_temp dst) {
#ifdef LLVMGEN_DEBUG
  printIndent();
  fprintf(stderr, "munchNameExp: e->u.NAME->name:%s\n",
          Temp_labelstring(e->u.NAME));
  depth++;
#endif
  ASSERT(e && e->kind == T_NAME);

  if (!dst) {
    dst = Temp_newtemp(e->type);
  }
  emit(AS_Oper(
      Stringf("%%`d0 = ptrtoint i64* @%s to i64", Temp_labelstring(e->u.NAME)),
      TL(dst, NULL), NULL, NULL));
#ifdef LLVMGEN_DEBUG
  depth--;
#endif
  return TempRes(dst);
}

static expres munchConstExp(T_exp e, Temp_temp dst) {
#ifdef LLVMGEN_DEBUG
  printIndent();
  if (e->type == T_int) {
    fprintf(stderr, "munchConstExp: int, e->u.CONST->i:%d\n", e->u.CONST.i);
  } else {
    fprintf(stderr, "munchConstExp: float, e->u.CONST->f:%f\n", e->u.CONST.f);
  }
  depth++;
#endif
  ASSERT(e && e->kind == T_CONST);

  if (OPT_CONST && !dst) {
#ifdef LLVMGEN_DEBUG
    depth--;
#endif
    switch (e->type) {
      case T_int: {
        return IntConstRes(e->u.CONST.i);
      }
      case T_float: {
        return FloatConstRes(e->u.CONST.f);
      }
      default:
        ASSERT(0);
    }
  }

  if (!dst) {
    dst = Temp_newtemp(e->type);
  }
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
  return TempRes(dst);
}

static expres munchCallExp(T_exp e, Temp_temp dst) {
#ifdef LLVMGEN_DEBUG
  printIndent();
  fprintf(stderr, "munchCallExp: e->u.CALL->id:%s\n", e->u.CALL.id);
  depth++;
#endif
  ASSERT(e && e->kind == T_CALL);

  // get the method address
  expres methAddr = munchExp(e->u.CALL.obj, NULL);
  Temp_temp methPtr = Temp_newtemp(T_int);
  emit(AS_Oper("\%`d0 = inttoptr i64 \%`s0 to i64*", TL(methPtr, NULL),
               TL(methAddr->u.t, NULL), NULL));

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
  if (!dst) {
    dst = Temp_newtemp(e->type);
  }
  emit(AS_Oper(
      Stringf("%%`d0 = call %s %%`s0(%s)", llvm_types[e->type], argsStr),
      TL(dst, NULL), TL(methPtr, args), NULL));
#ifdef LLVMGEN_DEBUG
  depth--;
  printIndent();
  fprintf(stderr, "munchCallExp ===end\n");
#endif
  return TempRes(dst);
}

static expres munchExtCallExp(T_exp e, Temp_temp dst) {
#ifdef LLVMGEN_DEBUG
  printIndent();
  fprintf(stderr, "munchExtCallExp: e->u.ExtCALL->extfun:%s\n",
          e->u.ExtCALL.extfun);
  depth++;
#endif
  ASSERT(e && e->kind == T_ExtCALL);

  if (!strcmp("malloc", e->u.ExtCALL.extfun)) {
    ASSERT(e->u.ExtCALL.args->head->type == T_int);
    expres size = munchExp(e->u.ExtCALL.args->head, NULL);
    Temp_temp ptr = Temp_newtemp(T_int);

    if (size->kind == res_const) {
      emit(AS_Oper(Stringf("%%`d0 = call i64* @malloc(i64 %d)", size->u.i),
                   TL(ptr, NULL), NULL, NULL));
    } else {
      emit(AS_Oper("\%`d0 = call i64* @malloc(i64 \%`s0)", TL(ptr, NULL),
                   TL(size->u.t, NULL), NULL));
    }

    if (!dst) {
      dst = Temp_newtemp(T_int);
    }
    emit(AS_Oper(Stringf("%%`d0 = ptrtoint i64* %%`s0 to i64"), TL(dst, NULL),
                 TL(ptr, NULL), NULL));

    return TempRes(dst);
  } else if (!strcmp("getint", e->u.ExtCALL.extfun) ||
             !strcmp("getch", e->u.ExtCALL.extfun)) {
    if (!dst) {
      dst = Temp_newtemp(T_int);
    }
    emit(AS_Oper(Stringf("%%`d0 = call i64 @%s()", e->u.ExtCALL.extfun),
                 TL(dst, NULL), NULL, NULL));

    return TempRes(dst);
  } else if (!strcmp("getfloat", e->u.ExtCALL.extfun)) {
    if (!dst) {
      dst = Temp_newtemp(T_float);
    }
    emit(AS_Oper(Stringf("%%`d0 = call double @%s()", e->u.ExtCALL.extfun),
                 TL(dst, NULL), NULL, NULL));

    return TempRes(dst);
  } else if (!strcmp("getarray", e->u.ExtCALL.extfun) ||
             !strcmp("getfarray", e->u.ExtCALL.extfun)) {
    expres arrAddr = munchExp(e->u.ExtCALL.args->head, NULL);
    Temp_temp arrPtr = Temp_newtemp(T_int);
    if (arrAddr->kind == res_const) {
      emit(AS_Oper(Stringf("%%`d0 = inttoptr i64 %d to i64*", arrAddr->u.i),
                   TL(arrPtr, NULL), NULL, NULL));
    } else {
      emit(AS_Oper("\%`d0 = inttoptr i64 \%`s0 to i64*", TL(arrPtr, NULL),
                   TL(arrAddr->u.t, NULL), NULL));
    }
    if (!dst) {
      dst = Temp_newtemp(T_int);
    }
    emit(AS_Oper(
        Stringf("%%`d0 = call i64 @%s(i64* %%`s0)", e->u.ExtCALL.extfun),
        TL(dst, NULL), TL(arrPtr, NULL), NULL));

    return TempRes(dst);
  } else if (!strcmp("putint", e->u.ExtCALL.extfun) ||
             !strcmp("putch", e->u.ExtCALL.extfun)) {
    expres num = munchExp(e->u.ExtCALL.args->head, NULL);
    if (num->kind == res_const) {
      emit(AS_Oper(
          Stringf("call void @%s(i64 %d)", e->u.ExtCALL.extfun, num->u.i), NULL,
          NULL, NULL));
    } else {
      emit(AS_Oper(Stringf("call void @%s(i64 %%`s0)", e->u.ExtCALL.extfun),
                   NULL, TL(num->u.t, NULL), NULL));
    }

    return NULL;
  } else if (!strcmp("putfloat", e->u.ExtCALL.extfun)) {
    expres num = munchExp(e->u.ExtCALL.args->head, NULL);
    if (num->kind == res_const) {
      emit(AS_Oper(
          Stringf("call void @%s(double %f)", e->u.ExtCALL.extfun, num->u.f),
          NULL, NULL, NULL));
    } else {
      emit(AS_Oper(Stringf("call void @%s(double %%`s0)", e->u.ExtCALL.extfun),
                   NULL, TL(num->u.t, NULL), NULL));
    }

    return NULL;
  } else if (!strcmp("putarray", e->u.ExtCALL.extfun) ||
             !strcmp("putfarray", e->u.ExtCALL.extfun)) {
    expres len = munchExp(e->u.ExtCALL.args->head, NULL);
    expres arrAddr = munchExp(e->u.ExtCALL.args->tail->head, NULL);
    Temp_temp arrPtr = Temp_newtemp(T_int);
    if (arrAddr->kind == res_const) {
      emit(AS_Oper(Stringf("%%`d0 = inttoptr i64 %d to i64*", arrAddr->u.i),
                   TL(arrPtr, NULL), NULL, NULL));
    } else {
      emit(AS_Oper("\%`d0 = inttoptr i64 \%`s0 to i64*", TL(arrPtr, NULL),
                   TL(arrAddr->u.t, NULL), NULL));
    }

    if (len->kind == res_const) {
      emit(AS_Oper(Stringf("call void @%s(i64 %d, i64* %%`s0)",
                           e->u.ExtCALL.extfun, len->u.i),
                   NULL, TL(arrPtr, NULL), NULL));
    } else {
      emit(AS_Oper(
          Stringf("call void @%s(i64 %%`s0, i64* %%`s1)", e->u.ExtCALL.extfun),
          NULL, TL(len->u.t, TL(arrPtr, NULL)), NULL));
    }

    return NULL;
  } else if (!strcmp("starttime", e->u.ExtCALL.extfun) ||
             !strcmp("stoptime", e->u.ExtCALL.extfun)) {
    emit(AS_Oper(Stringf("call void @%s()", e->u.ExtCALL.extfun), NULL, NULL,
                 NULL));
    return NULL;
  } else {
    ASSERT(0);
  }
#ifdef LLVMGEN_DEBUG
  depth--;
#endif
}

static Temp_tempList munchArgs(T_expList args, string argsStr, int initNo) {
  if (!args) return NULL;

  expres arg = munchExp(args->head, NULL);
  if (arg->kind == res_const) {
    if (strcmp(argsStr, "")) {
      strcat(argsStr, ", ");
    }
    switch (arg->type) {
      case T_int:
        strcat(argsStr, Stringf("i64 %d", arg->u.i));
        break;
      case T_float:
        strcat(argsStr, Stringf("double %f", arg->u.f));
        break;
      default:
        ASSERT(0);
    }
    return munchArgs(args->tail, argsStr, initNo);
  } else {
    if (strcmp(argsStr, "")) {
      strcat(argsStr, ", ");
    }
    switch (arg->type) {
      case T_int:
        strcat(argsStr, Stringf("i64 %%`s%d", initNo));
        break;
      case T_float:
        strcat(argsStr, Stringf("double %%`s%d", initNo));
        break;
      default:
        ASSERT(0);
    }
    return TL(arg->u.t, munchArgs(args->tail, argsStr, initNo + 1));
  }
}

static expres munchCastExp(T_exp e, Temp_temp dst) {
#ifdef LLVMGEN_DEBUG
  printIndent();
  fprintf(stderr, "munchCastExp: e->u.CAST->type:%s, e->type:%s\n",
          T_type2str[e->u.CAST->type], T_type2str[e->type]);
  depth++;
#endif
  ASSERT(e && e->kind == T_CAST && e->type != e->u.CAST->type);

  expres src = munchExp(e->u.CAST, NULL);
  if (OPT_CAST_CONST && src->kind == res_const && !dst) {
#ifdef LLVMGEN_DEBUG
    depth--;
#endif
    switch (e->type) {
      case T_int: {
        return IntConstRes((int)(src->u.f));
      }
      case T_float: {
        return FloatConstRes((float)(src->u.i));
      }
      default:
        ASSERT(0);
    }
  }

  if (!dst) {
    dst = Temp_newtemp(e->type);
  }
  if (src->kind == res_const) {
    switch (e->type) {
      case T_int: {
        emit(AS_Oper(Stringf("%%`d0 = fptosi double %f to i64", src->u.f),
                     TL(dst, NULL), NULL, NULL));
        break;
      }
      case T_float: {
        emit(AS_Oper(Stringf("%%`d0 = sitofp i64 %d to double", src->u.i),
                     TL(dst, NULL), NULL, NULL));
        break;
      }
      default:
        ASSERT(0);
    }
  } else {
    switch (e->type) {
      case T_int: {
        emit(AS_Oper("\%`d0 = fptosi double \%`s0 to i64", TL(dst, NULL),
                     TL(src->u.t, NULL), NULL));
        break;
      }
      case T_float: {
        emit(AS_Oper("\%`d0 = sitofp i64 \%`s0 to double", TL(dst, NULL),
                     TL(src->u.t, NULL), NULL));
        break;
      }
      default:
        ASSERT(0);
    }
  }
#ifdef LLVMGEN_DEBUG
  depth--;
#endif
  return TempRes(dst);
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