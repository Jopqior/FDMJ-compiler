#include "armgen.h"

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

#define ARMGEN_DEBUG
#undef ARMGEN_DEBUG

#define ARCH_SIZE 4
#define TYPELEN 10

#define IMM8_MAX 255

static Temp_temp lrTemp = NULL;

#define ARMREGSTART 0
#define FLOATREGSTART 20

static Temp_temp armReg2Temp(string reg) {
  if (!strcmp(reg, "pc")) {
    return Temp_namedtemp(15, T_int);
  } else if (!strcmp(reg, "lr")) {
    return Temp_namedtemp(14, T_int);
  } else if (!strcmp(reg, "sp")) {
    return Temp_namedtemp(13, T_int);
  } else if (!strcmp(reg, "fp")) {
    return Temp_namedtemp(11, T_int);
  } else if (reg[0] == 'r') {
    int regnum = atoi(reg + 1);
    return Temp_namedtemp(regnum + ARMREGSTART, T_int);
  } else if (reg[0] == 's') {
    int regnum = atoi(reg + 1);
    return Temp_namedtemp(regnum + FLOATREGSTART, T_float);
  } else {
    fprintf(stderr, "Error: unknown register\n");
    exit(1);
  }
}

// Return if the imm is formed by
// right-rotating an 8-bit value by an even number of bits
static bool isImm8(unsigned int imm) {
  for (int rotate = 0; rotate < 32; rotate += 2) {
    unsigned int imm8 = (imm << rotate) | (imm >> (32 - rotate));
    if (imm8 <= IMM8_MAX) {
      return TRUE;
    }
  }
  return FALSE;
}

#define IMM16_MAX 65535
#define getImm16U(imm) ((((unsigned int)imm) >> 16) & IMM16_MAX)
#define getImm16L(imm) (((unsigned int)imm) & IMM16_MAX)
typedef union uf uf;
union uf {
  int i;
  float f;
  unsigned int u;
};

static AS_instrList iList = NULL, last = NULL;
static void emit(AS_instr inst) {
  if (last) {
    last = last->tail = AS_InstrList(inst, NULL);
  } else {
    last = iList = AS_InstrList(inst, NULL);
  }
}

typedef enum {
  NONE,
  BR,
  RET,
  ADD,
  SUB,
  MUL,
  DIV,
  FADD,
  FSUB,
  FMUL,
  FDIV,
  F2I,
  I2F,
  I2P,
  P2I,
  LOAD,
  STORE,
  CALL,
  EXTCALL,
  ICMP,
  FCMP,
  LABEL,
  CJUMP,
  MOVE
} AS_type;

AS_type gettype(AS_instr ins) {
  AS_type ret = NONE;
  string assem = ins->u.OPER.assem;
  if (ins->kind == I_MOVE) {
    ret = MOVE;
    return ret;
  } else if (ins->kind == I_LABEL) {
    ret = LABEL;
    return ret;
  }
  if (!strncmp(assem, "br label", 8)) {
    ret = BR;
    return ret;
  } else if (!strncmp(assem, "ret", 3)) {
    ret = RET;
    return ret;
  } else if (!strncmp(assem, "%`d0 = fadd", 11)) {
    ret = FADD;
    return ret;
  } else if (!strncmp(assem, "%`d0 = add", 10)) {
    ret = ADD;
    return ret;
  } else if (!strncmp(assem, "%`d0 = fsub", 11)) {
    ret = FSUB;
    return ret;
  } else if (!strncmp(assem, "%`d0 = sub", 10)) {
    ret = SUB;
    return ret;
  } else if (!strncmp(assem, "%`d0 = fmul", 11)) {
    ret = FMUL;
    return ret;
  } else if (!strncmp(assem, "%`d0 = mul", 10)) {
    ret = MUL;
    return ret;
  } else if (!strncmp(assem, "%`d0 = fdiv", 11)) {
    ret = FDIV;
    return ret;
  } else if (!strncmp(assem, "%`d0 = sdiv", 11)) {
    ret = DIV;
    return ret;
  } else if (!strncmp(assem, "%`d0 = fptosi", 13)) {
    ret = F2I;
    return ret;
  } else if (!strncmp(assem, "%`d0 = sitofp", 13)) {
    ret = I2F;
    return ret;
  } else if (!strncmp(assem, "%`d0 = inttoptr", 15)) {
    ret = I2P;
    return ret;
  } else if (!strncmp(assem, "%`d0 = load", 11)) {
    ret = LOAD;
    return ret;
  } else if (!strncmp(assem, "store", 5)) {
    ret = STORE;
    return ret;
  } else if (!strncmp(assem, "%`d0 = ptrtoint", 15)) {
    ret = P2I;
    return ret;
  } else if (strstr(assem, "call") && !strchr(assem, '@')) {
    ret = CALL;
    return ret;
  } else if (strstr(assem, "call") && strchr(assem, '@')) {
    ret = EXTCALL;
    return ret;
  } else if (!strncmp(assem, "%`d0 = icmp", 11)) {
    ret = ICMP;
    return ret;
  } else if (!strncmp(assem, "%`d0 = fcmp", 11)) {
    ret = FCMP;
    return ret;
  } else if (!strncmp(assem, "br i1", 5)) {
    ret = CJUMP;
    return ret;
  }
  return ret;
}

static void emitMovImm(Temp_temp dt, string ds, uf imm) {
  if (!dt && !ds) {
    fprintf(stderr, "Error: no temp for mov imm\n");
    exit(1);
  }
  int immu = getImm16U(imm.u);
  int imml = getImm16L(imm.u);
  if (dt) {
    switch (dt->type) {
      case T_int: {
        if (imm.i < 0 && isImm8(-imm.i - 1)) {
          emit(AS_Oper(Stringf("\tmvn `d0, #%d", -imm.i - 1),
                       Temp_TempList(dt, NULL), NULL, NULL));
        } else {
          emit(AS_Oper(Stringf("\tmov `d0, #%d", imml), Temp_TempList(dt, NULL),
                       NULL, NULL));
          if (immu) {
            emit(AS_Oper(Stringf("\tmovt `d0, #%d", immu),
                         Temp_TempList(dt, NULL), NULL, NULL));
          }
        }
        break;
      }
      case T_float: {
        Temp_temp tmp = Temp_newtemp(T_int);
        emit(AS_Oper(Stringf("\tmov `d0, #%d", imml), Temp_TempList(tmp, NULL),
                     NULL, NULL));
        if (immu) {
          emit(AS_Oper(Stringf("\tmovt `d0, #%d", immu),
                       Temp_TempList(tmp, NULL), NULL, NULL));
        }
        emit(AS_Oper("\tvmov.f32 `d0, `s0", Temp_TempList(dt, NULL),
                     Temp_TempList(tmp, NULL), NULL));
        break;
      }
      default: {
        fprintf(stderr, "Error: unknown temp type\n");
        exit(1);
      }
    }
  } else {
    switch (ds[0]) {
      case 'r': {
        if (imm.i < 0 && isImm8(-imm.i - 1)) {
          emit(AS_Oper(Stringf("\tmvn %s, #%d", ds, -imm.i - 1),
                       Temp_TempList(armReg2Temp(ds), NULL), NULL, NULL));
        } else {
          emit(AS_Oper(Stringf("\tmov %s, #%d", ds, imml),
                       Temp_TempList(armReg2Temp(ds), NULL), NULL, NULL));
          if (immu) {
            emit(AS_Oper(Stringf("\tmovt %s, #%d", ds, immu),
                         Temp_TempList(armReg2Temp(ds), NULL), NULL, NULL));
          }
        }
        break;
      }
      case 's': {
        Temp_temp tmp = Temp_newtemp(T_int);
        emit(AS_Oper(Stringf("\tmov `d0, #%d", imml), Temp_TempList(tmp, NULL),
                     NULL, NULL));
        if (immu) {
          emit(AS_Oper(Stringf("\tmovt `d0, #%d", immu),
                       Temp_TempList(tmp, NULL), NULL, NULL));
        }
        emit(AS_Oper(Stringf("\tvmov.f32 %s, `s0", ds),
                     Temp_TempList(armReg2Temp(ds), NULL),
                     Temp_TempList(tmp, NULL), NULL));
        break;
      }
      default: {
        fprintf(stderr, "Error: unknown dest type\n");
        exit(1);
      }
    }
  }
}

static void munchLabel(AS_instr ins) {
  emit(AS_Label(Stringf(".%s:", Temp_labelstring(ins->u.LABEL.label)),
                ins->u.LABEL.label));
}

static void munchMove(AS_instr ins) {
  Temp_tempList dst = ins->u.MOVE.dst;
  Temp_tempList src = ins->u.MOVE.src;
  ASSERT(dst->head->type == src->head->type);

  switch (dst->head->type) {
    case T_int: {
      emit(AS_Move("\tmov `d0, `s0", dst, src));
      break;
    }
    case T_float: {
      emit(AS_Move("\tvmov.f32 `d0, `s0", dst, src));
      break;
    }
  }
}

static void munchBr(AS_instr ins) {
  emit(AS_Oper("\tb .`j0", NULL, NULL, ins->u.OPER.jumps));
}

static void munchRet(AS_instr ins, Temp_label retLabel) {
  if (!ins->u.OPER.src) {
    // parse the const ret value
    string assem = ins->u.OPER.assem;
    char *s = assem + 4;
    while (*s == ' ') {
      s++;
    }

    switch (*s) {
      case 'i': {
        while (*s != ' ') {
          s++;
        }
        while (*s == ' ') {
          s++;
        }
        int ret = atoi(s);
        emitMovImm(NULL, "r0", (uf){.i = ret});
        break;
      }
      case 'd': {
        while (*s != ' ') {
          s++;
        }
        while (*s == ' ') {
          s++;
        }
        float ret = atof(s);
        emitMovImm(NULL, "s0", (uf){.f = ret});
        break;
      }
      default: {
        fprintf(stderr, "Error: unknown return type\n");
        exit(1);
      }
    }
  } else {
    Temp_temp ret = ins->u.OPER.src->head;
    switch (ret->type) {
      case T_int: {
        emit(AS_Move("\tmov r0, `s0", Temp_TempList(armReg2Temp("r0"), NULL),
                     Temp_TempList(ret, NULL)));
        break;
      }
      case T_float: {
        emit(AS_Move("\tvmov.f32 s0, `s0",
                     Temp_TempList(armReg2Temp("s0"), NULL),
                     Temp_TempList(ret, NULL)));
        break;
      }
    }
  }

  // TODO: restore the callee-saved registers

  // restore the frame pointer
  emit(AS_Move("\tmov sp, fp", Temp_TempList(armReg2Temp("sp"), NULL),
               Temp_TempList(armReg2Temp("fp"), NULL)));
  emit(AS_Oper("\tpop {fp}", Temp_TempList(armReg2Temp("fp"), NULL),
               Temp_TempList(armReg2Temp("sp"), NULL), NULL));

  // branch to the lrTemp
  emit(AS_Oper("\tbx `s0", NULL, Temp_TempList(lrTemp, NULL), NULL));
}

typedef enum { OP_ADD, OP_SUB, OP_MUL, OP_DIV } OP_type;

// s must be at the end of op
// e.g. for "add %r0, %r1, 1", s is at ' ' just after add
static uf parseOpExpConst(char *s, OP_type optype, bool isBothConst) {
  T_type type;
  while (*s == ' ') {
    s++;
  }
  switch (*s) {
    case 'i': {
      type = T_int;
      break;
    }
    case 'd': {
      type = T_float;
      break;
    }
    default: {
      fprintf(stderr, "Error: unknown type in parseOpExpConst\n");
      exit(1);
    }
  }
  while (*s != ' ') {
    s++;
  }
  while (*s == ' ') {
    s++;
  }
  switch (type) {
    case T_int: {
      if (isBothConst) {
        int src1 = atoi(s);
        while (*s != ',') {
          s++;
        }
        s++;
        while (*s == ' ') {
          s++;
        }
        int src2 = atoi(s);
        int res;
        switch (optype) {
          case OP_ADD: {
            res = src1 + src2;
            break;
          }
          case OP_SUB: {
            res = src1 - src2;
            break;
          }
          case OP_MUL: {
            res = src1 * src2;
            break;
          }
          case OP_DIV: {
            if (src2 == 0) {
              fprintf(stderr, "Error: division by zero\n");
              exit(1);
            }
            res = src1 / src2;
            break;
          }
        }
        return (uf){.i = res};
      } else {
        int src;
        if (*s == '%') {
          // first src is a temp, second src is a const
          while (*s != ',') {
            s++;
          }
          s++;
          while (*s == ' ') {
            s++;
          }
          src = atoi(s);
        } else {
          src = atoi(s);
        }
        return (uf){.i = src};
      }
    }
    case T_float: {
      if (isBothConst) {
        float src1 = atof(s);
        while (*s != ',') {
          s++;
        }
        s++;
        while (*s == ' ') {
          s++;
        }
        float src2 = atof(s);
        float res;
        switch (optype) {
          case OP_ADD: {
            res = src1 + src2;
            break;
          }
          case OP_SUB: {
            res = src1 - src2;
            break;
          }
          case OP_MUL: {
            res = src1 * src2;
            break;
          }
          case OP_DIV: {
            if (src2 == 0) {
              fprintf(stderr, "Error: division by zero\n");
              exit(1);
            }
            res = src1 / src2;
            break;
          }
        }
        return (uf){.f = res};
      } else {
        float src;
        if (*s == '%') {
          // first src is a temp, second src is a const
          while (*s != ',') {
            s++;
          }
          s++;
          while (*s == ' ') {
            s++;
          }
          src = atof(s);
        } else {
          src = atof(s);
        }
        return (uf){.f = src};
      }
    }
  }
}

static void munchAdd(AS_instr ins) {
  Temp_temp dst = ins->u.OPER.dst->head;
  Temp_tempList srcs = ins->u.OPER.src;

  if (!srcs) {
    // parse the two const values
    uf res = parseOpExpConst(ins->u.OPER.assem + 11, OP_ADD, TRUE);
    emitMovImm(dst, NULL, res);
  } else if (!srcs->tail) {
    Temp_temp srct = srcs->head;
    uf srci = parseOpExpConst(ins->u.OPER.assem + 11, OP_ADD, FALSE);
    if (isImm8(srci.u)) {
      emit(AS_Oper(Stringf("\tadd `d0, `s0, #%d", srci.i),
                   Temp_TempList(dst, NULL), srcs, NULL));
    } else {
      Temp_temp tmp = Temp_newtemp(T_int);
      emitMovImm(tmp, NULL, srci);
      emit(AS_Oper("\tadd `d0, `s0, `s1", Temp_TempList(dst, NULL),
                   Temp_TempList(srct, Temp_TempList(tmp, NULL)), NULL));
    }
  } else {
    emit(AS_Oper("\tadd `d0, `s0, `s1", Temp_TempList(dst, NULL), srcs, NULL));
  }
}

static void munchFadd(AS_instr ins) {
  Temp_temp dst = ins->u.OPER.dst->head;
  Temp_tempList srcs = ins->u.OPER.src;

  if (!srcs) {
    // parse the two const values
    uf res = parseOpExpConst(ins->u.OPER.assem + 12, OP_ADD, TRUE);
    emitMovImm(dst, NULL, res);
  } else if (!srcs->tail) {
    Temp_temp srct = srcs->head;
    uf srcf = parseOpExpConst(ins->u.OPER.assem + 12, OP_ADD, FALSE);
    Temp_temp tmp = Temp_newtemp(T_float);
    emitMovImm(tmp, NULL, srcf);
    emit(AS_Oper("\tvadd.f32 `d0, `s0, `s1", Temp_TempList(dst, NULL),
                 Temp_TempList(srct, Temp_TempList(tmp, NULL)), NULL));
  } else {
    emit(AS_Oper("\tvadd.f32 `d0, `s0, `s1", Temp_TempList(dst, NULL), srcs,
                 NULL));
  }
}

static void munchSub(AS_instr ins) {
  Temp_temp dst = ins->u.OPER.dst->head;
  Temp_tempList srcs = ins->u.OPER.src;

  if (!srcs) {
    // parse the two const values
    uf res = parseOpExpConst(ins->u.OPER.assem + 11, OP_SUB, TRUE);
    emitMovImm(dst, NULL, res);
  } else if (!srcs->tail) {
    Temp_temp srct = srcs->head;
    uf srci = parseOpExpConst(ins->u.OPER.assem + 11, OP_SUB, FALSE);
    if (isImm8(srci.u)) {
      emit(AS_Oper(Stringf("\tsub `d0, `s0, #%d", srci.i),
                   Temp_TempList(dst, NULL), srcs, NULL));
    } else {
      Temp_temp tmp = Temp_newtemp(T_int);
      emitMovImm(tmp, NULL, srci);
      emit(AS_Oper("\tsub `d0, `s0, `s1", Temp_TempList(dst, NULL),
                   Temp_TempList(srct, Temp_TempList(tmp, NULL)), NULL));
    }
  } else {
    emit(AS_Oper("\tsub `d0, `s0, `s1", Temp_TempList(dst, NULL), srcs, NULL));
  }
}

static void munchFsub(AS_instr ins) {
  Temp_temp dst = ins->u.OPER.dst->head;
  Temp_tempList srcs = ins->u.OPER.src;

  if (!srcs) {
    // parse the two const values
    uf res = parseOpExpConst(ins->u.OPER.assem + 12, OP_SUB, TRUE);
    emitMovImm(dst, NULL, res);
  } else if (!srcs->tail) {
    Temp_temp srct = srcs->head;
    uf srcf = parseOpExpConst(ins->u.OPER.assem + 12, OP_SUB, FALSE);
    Temp_temp tmp = Temp_newtemp(T_float);
    emitMovImm(tmp, NULL, srcf);
    emit(AS_Oper("\tvsub.f32 `d0, `s0, `s1", Temp_TempList(dst, NULL),
                 Temp_TempList(srct, Temp_TempList(tmp, NULL)), NULL));
  } else {
    emit(AS_Oper("\tvsub.f32 `d0, `s0, `s1", Temp_TempList(dst, NULL), srcs,
                 NULL));
  }
}

static void munchMul(AS_instr ins) {
  Temp_temp dst = ins->u.OPER.dst->head;
  Temp_tempList srcs = ins->u.OPER.src;

  if (!srcs) {
    // parse the two const values
    uf res = parseOpExpConst(ins->u.OPER.assem + 11, OP_MUL, TRUE);
    emitMovImm(dst, NULL, res);
  } else if (!srcs->tail) {
    Temp_temp srct = srcs->head;
    uf srci = parseOpExpConst(ins->u.OPER.assem + 11, OP_MUL, FALSE);
    Temp_temp tmp = Temp_newtemp(T_int);
    emitMovImm(tmp, NULL, srci);
    emit(AS_Oper("\tmul `d0, `s0, `s1", Temp_TempList(dst, NULL),
                 Temp_TempList(srct, Temp_TempList(tmp, NULL)), NULL));

  } else {
    emit(AS_Oper("\tmul `d0, `s0, `s1", Temp_TempList(dst, NULL), srcs, NULL));
  }
}

static void munchFmul(AS_instr ins) {
  Temp_temp dst = ins->u.OPER.dst->head;
  Temp_tempList srcs = ins->u.OPER.src;

  if (!srcs) {
    // parse the two const values
    uf res = parseOpExpConst(ins->u.OPER.assem + 12, OP_MUL, TRUE);
    emitMovImm(dst, NULL, res);
  } else if (!srcs->tail) {
    Temp_temp srct = srcs->head;
    uf srcf = parseOpExpConst(ins->u.OPER.assem + 12, OP_MUL, FALSE);
    Temp_temp tmp = Temp_newtemp(T_float);
    emitMovImm(tmp, NULL, srcf);
    emit(AS_Oper("\tvmul.f32 `d0, `s0, `s1", Temp_TempList(dst, NULL),
                 Temp_TempList(srct, Temp_TempList(tmp, NULL)), NULL));
  } else {
    emit(AS_Oper("\tvmul.f32 `d0, `s0, `s1", Temp_TempList(dst, NULL), srcs,
                 NULL));
  }
}

static void munchDiv(AS_instr ins) {
  Temp_temp dst = ins->u.OPER.dst->head;
  Temp_tempList srcs = ins->u.OPER.src;

  if (!srcs) {
    // parse the two const values
    uf res = parseOpExpConst(ins->u.OPER.assem + 12, OP_DIV, TRUE);
    emitMovImm(dst, NULL, res);
  } else if (!srcs->tail) {
    Temp_temp srct = srcs->head;
    uf srci = parseOpExpConst(ins->u.OPER.assem + 12, OP_DIV, FALSE);
    Temp_temp tmp = Temp_newtemp(T_int);
    emitMovImm(tmp, NULL, srci);
    emit(AS_Oper("\tsdiv `d0, `s0, `s1", Temp_TempList(dst, NULL),
                 Temp_TempList(srct, Temp_TempList(tmp, NULL)), NULL));
  } else {
    emit(AS_Oper("\tsdiv `d0, `s0, `s1", Temp_TempList(dst, NULL), srcs, NULL));
  }
}

static void munchFdiv(AS_instr ins) {
  Temp_temp dst = ins->u.OPER.dst->head;
  Temp_tempList srcs = ins->u.OPER.src;

  if (!srcs) {
    // parse the two const values
    uf res = parseOpExpConst(ins->u.OPER.assem + 12, OP_DIV, TRUE);
    emitMovImm(dst, NULL, res);
  } else if (!srcs->tail) {
    Temp_temp srct = srcs->head;
    uf srcf = parseOpExpConst(ins->u.OPER.assem + 12, OP_DIV, FALSE);
    Temp_temp tmp = Temp_newtemp(T_float);
    emitMovImm(tmp, NULL, srcf);
    emit(AS_Oper("\tvdiv.f32 `d0, `s0, `s1", Temp_TempList(dst, NULL),
                 Temp_TempList(srct, Temp_TempList(tmp, NULL)), NULL));
  } else {
    emit(AS_Oper("\tvdiv.f32 `d0, `s0, `s1", Temp_TempList(dst, NULL), srcs,
                 NULL));
  }
}

static void munchF2I(AS_instr ins) {
  Temp_temp dst = ins->u.OPER.dst->head;
  if (!ins->u.OPER.src) {
    // parse the const value
    char *s = ins->u.OPER.assem + 14;
    while (*s == ' ') {
      s++;
    }
    while (*s != ' ') {
      s++;
    }
    while (*s == ' ') {
      s++;
    }
    float src = atof(s);
    emitMovImm(dst, NULL, (uf){.i = (int)src});
  } else {
    Temp_temp src = ins->u.OPER.src->head;
    emit(AS_Oper("\tvcvt.s32.f32 `d0, `s0", Temp_TempList(dst, NULL),
                 Temp_TempList(src, NULL), NULL));
  }
}

static void munchI2F(AS_instr ins) {
  Temp_temp dst = ins->u.OPER.dst->head;
  if (!ins->u.OPER.src) {
    // parse the const value
    char *s = ins->u.OPER.assem + 14;
    while (*s == ' ') {
      s++;
    }
    while (*s != ' ') {
      s++;
    }
    while (*s == ' ') {
      s++;
    }
    int src = atoi(s);
    emitMovImm(dst, NULL, (uf){.f = (float)src});
  } else {
    Temp_temp src = ins->u.OPER.src->head;
    emit(AS_Oper("\tvcvt.f32.s32 `d0, `s0", Temp_TempList(dst, NULL),
                 Temp_TempList(src, NULL), NULL));
  }
}

static void munchLoad(AS_instr ins) {
  Temp_temp dst = ins->u.OPER.dst->head;
  Temp_temp src = ins->u.OPER.src->head;
  switch (dst->type) {
    case T_int: {
      emit(AS_Oper("\tldr `d0, [`s0]", Temp_TempList(dst, NULL),
                   Temp_TempList(src, NULL), NULL));
      break;
    }
    case T_float: {
      emit(AS_Oper("\tvldr.f32 `d0, [`s0]", Temp_TempList(dst, NULL),
                   Temp_TempList(src, NULL), NULL));
      break;
    }
    default: {
      fprintf(stderr, "Error: unknown temp type\n");
      exit(1);
    }
  }
}

static void munchStore(AS_instr ins) {
  Temp_tempList srcs = ins->u.OPER.src;
  if (!srcs->tail) {
    // the number to store is a const
    char *s = ins->u.OPER.assem + 6;
    T_type type;
    while (*s == ' ') {
      s++;
    }
    switch (*s) {
      case 'i': {
        type = T_int;
        break;
      }
      case 'd': {
        type = T_float;
        break;
      }
      default: {
        fprintf(stderr, "Error: unknown type in store\n");
        exit(1);
      }
    }
    while (*s != ' ') {
      s++;
    }
    while (*s == ' ') {
      s++;
    }
    switch (type) {
      case T_int: {
        int num = atoi(s);
        Temp_temp tmp = Temp_newtemp(T_int);
        emitMovImm(tmp, NULL, (uf){.i = num});
        emit(AS_Oper("\tstr `s0, [`s1]", NULL, Temp_TempList(tmp, srcs), NULL));
        break;
      }
      case T_float: {
        float num = atof(s);
        Temp_temp tmp = Temp_newtemp(T_float);
        emitMovImm(tmp, NULL, (uf){.f = num});
        emit(AS_Oper("\tvstr.f32 `s0, [`s1]", NULL, Temp_TempList(tmp, srcs),
                     NULL));
        break;
      }
    }
  } else {
    Temp_temp num = srcs->head;
    switch (num->type) {
      case T_int: {
        emit(AS_Oper("\tstr `s0, [`s1]", NULL, srcs, NULL));
        break;
      }
      case T_float: {
        emit(AS_Oper("\tvstr.f32 `s0, [`s1]", NULL, srcs, NULL));
        break;
      }
      default: {
        fprintf(stderr, "Error: unknown temp type\n");
        exit(1);
      }
    }
  }
}

AS_instrList armbody(AS_instrList il, Temp_label retLabel) {
  iList = last = NULL;

  for (AS_instrList p = il; p; p = p->tail) {
    AS_instr ins = p->head;
    AS_type type = gettype(ins);
    if (type == NONE) {
      fprintf(stderr, "Error: unknown instruction type, %s\n",
              ins->u.OPER.assem);
      exit(1);
    }
    switch (type) {
      case LABEL: {
        munchLabel(ins);
        break;
      }
      case MOVE: {
        munchMove(ins);
        break;
      }
      case BR: {
        munchBr(ins);
        break;
      }
      case RET: {
        munchRet(ins, retLabel);
        break;
      }
      case ADD: {
        munchAdd(ins);
        break;
      }
      case FADD: {
        munchFadd(ins);
        break;
      }
      case SUB: {
        munchSub(ins);
        break;
      }
      case FSUB: {
        munchFsub(ins);
        break;
      }
      case MUL: {
        munchMul(ins);
        break;
      }
      case FMUL: {
        munchFmul(ins);
        break;
      }
      case DIV: {
        munchDiv(ins);
        break;
      }
      case FDIV: {
        munchFdiv(ins);
        break;
      }
      case F2I: {
        munchF2I(ins);
        break;
      }
      case I2F: {
        munchI2F(ins);
        break;
      }
      case I2P: {
        // nothing to do
        break;
      }
      case P2I: {
        // nothing to do
        break;
      }
      case LOAD: {
        munchLoad(ins);
        break;
      }
      case STORE: {
        munchStore(ins);
        break;
      }
    }
  }

  return iList;
}

AS_instrList armprolog(AS_instrList il) {
#ifdef ARMGEN_DEBUG
  fprintf(stderr, "armprolog: assem=%s\n", il->head->u.OPER.assem);
#endif
  iList = last = NULL;

  // check if the prolog is valid
  if (il->tail) {
    fprintf(stderr, "Error: prolog should only have one instruction\n");
    exit(1);
  }
  AS_instr ins = il->head;
  if (strncmp(ins->u.OPER.assem, "define ", 7)) {
    fprintf(stderr, "Error: prolog should start with define\n");
    exit(1);
  }

  // prolog must be like: define type @funcname(args) {

  // parse the function name
  char *nameSuperStart = strchr(ins->u.OPER.assem, '@');
  if (!nameSuperStart) {
    fprintf(stderr,
            "Error: prolog should contain function name starting with @\n");
    exit(1);
  }
  char *nameEnd = nameSuperStart + 1;
  while (*nameEnd != ' ' && *nameEnd != '(') {
    nameEnd++;
  }
  if (nameEnd == nameSuperStart + 1) {
    fprintf(stderr, "Error: function name should not be empty\n");
    exit(1);
  }
  char *funcname = checked_malloc(nameEnd - nameSuperStart);
  strncpy(funcname, nameSuperStart + 1, nameEnd - nameSuperStart - 1);
  funcname[nameEnd - nameSuperStart - 1] = '\0';
#ifdef ARMGEN_DEBUG
  fprintf(stderr, "armprolog: funcname=%s\n", funcname);
#endif

  // emit the function name
  emit(AS_Oper(Stringf("%s:", funcname), NULL, NULL, NULL));

  // save the frame pointer
  emit(AS_Oper(
      "\tpush {fp}", NULL,
      Temp_TempList(armReg2Temp("fp"), Temp_TempList(armReg2Temp("sp"), NULL)),
      NULL));
  // set the frame pointer
  emit(AS_Move("\tmov fp, sp", Temp_TempList(armReg2Temp("fp"), NULL),
               Temp_TempList(armReg2Temp("sp"), NULL)));
  // save the link register in lrTemp
  if (!lrTemp) {
    lrTemp = Temp_newtemp(T_int);
  }
  emit(AS_Move("\tmov `d0, lr", Temp_TempList(lrTemp, NULL),
               Temp_TempList(armReg2Temp("lr"), NULL)));

  // TODO: save the callee-saved registers

  // parse the args
  int intArgNum = 0, floatArgNum = 0;
  int stackArgNum = 0;
  for (Temp_tempList args = ins->u.OPER.src; args; args = args->tail) {
    Temp_temp arg = args->head;
    switch (arg->type) {
      case T_int: {
        if (intArgNum < 4) {
          // TODO: MAP
          emit(AS_Move(
              Stringf("\tmov `d0, r%d", intArgNum), Temp_TempList(arg, NULL),
              Temp_TempList(armReg2Temp(Stringf("r%d", intArgNum)), NULL)));
        } else {
          emit(AS_Oper(
              Stringf("\tldr `d0, [fp, #%d]", (++stackArgNum) * ARCH_SIZE),
              Temp_TempList(arg, NULL), Temp_TempList(armReg2Temp("fp"), NULL),
              NULL));
        }
        ++intArgNum;
        break;
      }
      case T_float: {
        if (floatArgNum < 16) {
          // TODO: MAP
          emit(AS_Move(
              Stringf("\tvmov.f32 `d0, s%d", floatArgNum),
              Temp_TempList(arg, NULL),
              Temp_TempList(armReg2Temp(Stringf("s%d", floatArgNum)), NULL)));
        } else {
          emit(AS_Oper(
              Stringf("\tvldr.f32 `d0, [fp, #%d]", (++stackArgNum) * ARCH_SIZE),
              Temp_TempList(arg, NULL), Temp_TempList(armReg2Temp("fp"), NULL),
              NULL));
        }
        ++floatArgNum;
        break;
      }
      default: {
        fprintf(stderr, "Error: unknown arg type\n");
        exit(1);
      }
    }
  }

  return iList;
}

AS_instrList armepilog(AS_instrList il, Temp_label retLabel) {
#ifdef ARMGEN_DEBUG
  fprintf(stderr, "armepilog: assem=%s\n", il->head->u.OPER.assem);
#endif
  iList = last = NULL;

  // nothing to do
  return iList;
}