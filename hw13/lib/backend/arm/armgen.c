#include "armgen.h"

#define ASSERT_DEBUG

#ifdef ASSERT_DEBUG
#define ASSERT(condition, errInfo)         \
  do {                                     \
    if (condition)                         \
      NULL;                                \
    else                                   \
      Assert(__FILE__, __LINE__, errInfo); \
  } while (0)
#else
#define ASSERT(condition, errInfo) \
  do {                             \
    NULL;                          \
  } while (0)
#endif

static void Assert(char *filename, unsigned int lineno, char *errInfo) {
  fflush(stdout);
  fprintf(stderr, "Assertion failed at %s:%u: %s\n", filename, lineno, errInfo);
  fflush(stderr);
  abort();
}

#define ARMGEN_DEBUG
#undef ARMGEN_DEBUG

#define ARCH_SIZE 4
#define TYPELEN 10

#define IMM8_MAX 255

#define ARM_REG_START 0
#define FLOAT_REG_START 20

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
    return Temp_namedtemp(regnum + ARM_REG_START, T_int);
  } else if (reg[0] == 's') {
    int regnum = atoi(reg + 1);
    return Temp_namedtemp(regnum + FLOAT_REG_START, T_float);
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
  NAME,
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
    if (strchr(assem, '@')) {
      ret = NAME;
    } else {
      ret = P2I;
    }
    return ret;
  } else if (strstr(assem, "call")) {
    if (strchr(assem, '@')) {
      ret = EXTCALL;
    } else {
      ret = CALL;
    }
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

static Temp_tempList getCallerSavedRegs() {
  Temp_tempList callerSavedRegs = NULL;
  for (int i = 0; i <= 3; ++i) {
    callerSavedRegs = Temp_TempListSplice(
        callerSavedRegs,
        Temp_TempList(Temp_namedtemp(i + ARM_REG_START, T_int), NULL));
  }
  for (int i = 0; i <= 13; ++i) {
    callerSavedRegs = Temp_TempListSplice(
        callerSavedRegs,
        Temp_TempList(Temp_namedtemp(i + FLOAT_REG_START, T_float), NULL));
  }
  return callerSavedRegs;
}

static void emitMovImm(Temp_temp dt, string ds, uf imm) {
  ASSERT(dt || ds, "no temp for mov imm");

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
  emit(AS_Label(Stringf("%s:", Temp_labelstring(ins->u.LABEL.label)),
                ins->u.LABEL.label));
}

static void munchMove(AS_instr ins) {
  Temp_tempList dst = ins->u.MOVE.dst;
  Temp_tempList src = ins->u.MOVE.src;
  ASSERT(dst->head->type == src->head->type, "move type mismatch");

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
  emit(AS_Oper("\tb `j0", NULL, NULL, ins->u.OPER.jumps));
}

static void munchRet(AS_instr ins) {
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
  emit(AS_Oper("\tsub sp, fp, #32", NULL, NULL, NULL));
  emit(AS_Oper("\tpop {r4, r5, r6, r7, r8, r9, r10}", NULL, NULL, NULL));

  // restore the link register
  emit(AS_Oper("\tpop {lr}", Temp_TempList(armReg2Temp("lr"), NULL), NULL,
               NULL));

  // restore the frame pointer
  emit(AS_Oper("\tpop {fp}", NULL, NULL, NULL));

  // branch to the lrTemp
  emit(AS_Oper("\tbx lr", NULL, Temp_TempList(armReg2Temp("lr"), NULL), NULL));
}

typedef struct {
  uf const1;
  uf const2;
} constPair;

// parse the two const values in the op
// e.g. for "add i64 1, 2", s is at ' ' just after add
static constPair parseOpExpConstPair(char *s) {
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
      fprintf(stderr, "Error: unknown type in parseOpExpConstPair\n");
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
      int src1 = atoi(s);
      while (*s != ',') {
        s++;
      }
      s++;
      while (*s == ' ') {
        s++;
      }
      int src2 = atoi(s);
      return (constPair){.const1 = (uf){.i = src1}, .const2 = (uf){.i = src2}};
    }
    case T_float: {
      float src1 = atof(s);
      while (*s != ',') {
        s++;
      }
      s++;
      while (*s == ' ') {
        s++;
      }
      float src2 = atof(s);
      return (constPair){.const1 = (uf){.f = src1}, .const2 = (uf){.f = src2}};
    }
  }
}

// s must be at the end of op
// e.g. for "add i64 %`s0, 1", s is at ' ' just after add
static uf parseOpExpConst(char *s) {
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
    case T_float: {
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

static void munchAdd(AS_instr ins) {
  ASSERT(ins->u.OPER.dst, "no dest for add");
  Temp_temp dst = ins->u.OPER.dst->head;
  Temp_tempList srcs = ins->u.OPER.src;

  if (!srcs) {
    // parse the two const values
    constPair res = parseOpExpConstPair(ins->u.OPER.assem + 11);
    emitMovImm(dst, NULL, (uf){.i = res.const1.i + res.const2.i});
  } else if (!srcs->tail) {
    Temp_temp srct = srcs->head;
    uf srci = parseOpExpConst(ins->u.OPER.assem + 11);
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
  ASSERT(ins->u.OPER.dst, "no dest for fadd");
  Temp_temp dst = ins->u.OPER.dst->head;
  Temp_tempList srcs = ins->u.OPER.src;

  if (!srcs) {
    // parse the two const values
    constPair res = parseOpExpConstPair(ins->u.OPER.assem + 12);
    emitMovImm(dst, NULL, (uf){.f = res.const1.f + res.const2.f});
  } else if (!srcs->tail) {
    Temp_temp srct = srcs->head;
    uf srcf = parseOpExpConst(ins->u.OPER.assem + 12);
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
  ASSERT(ins->u.OPER.dst, "no dest for sub");
  Temp_temp dst = ins->u.OPER.dst->head;
  Temp_tempList srcs = ins->u.OPER.src;

  if (!srcs) {
    // parse the two const values
    constPair res = parseOpExpConstPair(ins->u.OPER.assem + 11);
    emitMovImm(dst, NULL, (uf){.i = res.const1.i - res.const2.i});
  } else if (!srcs->tail) {
    Temp_temp srct = srcs->head;
    uf srci = parseOpExpConst(ins->u.OPER.assem + 11);
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
  ASSERT(ins->u.OPER.dst, "no dest for fsub");
  Temp_temp dst = ins->u.OPER.dst->head;
  Temp_tempList srcs = ins->u.OPER.src;

  if (!srcs) {
    // parse the two const values
    constPair res = parseOpExpConstPair(ins->u.OPER.assem + 12);
    emitMovImm(dst, NULL, (uf){.f = res.const1.f - res.const2.f});
  } else if (!srcs->tail) {
    Temp_temp srct = srcs->head;
    uf srcf = parseOpExpConst(ins->u.OPER.assem + 12);
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
  ASSERT(ins->u.OPER.dst, "no dest for mul");
  Temp_temp dst = ins->u.OPER.dst->head;
  Temp_tempList srcs = ins->u.OPER.src;

  if (!srcs) {
    // parse the two const values
    constPair res = parseOpExpConstPair(ins->u.OPER.assem + 11);
    emitMovImm(dst, NULL, (uf){.i = res.const1.i * res.const2.i});
  } else if (!srcs->tail) {
    Temp_temp srct = srcs->head;
    uf srci = parseOpExpConst(ins->u.OPER.assem + 11);
    Temp_temp tmp = Temp_newtemp(T_int);
    emitMovImm(tmp, NULL, srci);
    emit(AS_Oper("\tmul `d0, `s0, `s1", Temp_TempList(dst, NULL),
                 Temp_TempList(srct, Temp_TempList(tmp, NULL)), NULL));

  } else {
    emit(AS_Oper("\tmul `d0, `s0, `s1", Temp_TempList(dst, NULL), srcs, NULL));
  }
}

static void munchFmul(AS_instr ins) {
  ASSERT(ins->u.OPER.dst, "no dest for fmul");
  Temp_temp dst = ins->u.OPER.dst->head;
  Temp_tempList srcs = ins->u.OPER.src;

  if (!srcs) {
    // parse the two const values
    constPair res = parseOpExpConstPair(ins->u.OPER.assem + 12);
    emitMovImm(dst, NULL, (uf){.f = res.const1.f * res.const2.f});
  } else if (!srcs->tail) {
    Temp_temp srct = srcs->head;
    uf srcf = parseOpExpConst(ins->u.OPER.assem + 12);
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
  ASSERT(ins->u.OPER.dst, "no dest for div");
  Temp_temp dst = ins->u.OPER.dst->head;
  Temp_tempList srcs = ins->u.OPER.src;

  if (!srcs) {
    // parse the two const values
    constPair res = parseOpExpConstPair(ins->u.OPER.assem + 11);
    emitMovImm(NULL, "r0", (uf){.i = res.const1.i});
    emitMovImm(NULL, "r1", (uf){.i = res.const2.i});
    emit(AS_Oper("\tblx __aeabi_idiv", NULL, NULL, NULL));
  } else if (!srcs->tail) {
    Temp_temp srct = srcs->head;
    emit(AS_Move("\tmov r0, `s0", Temp_TempList(armReg2Temp("r0"), NULL),
                 Temp_TempList(srct, NULL)));
    uf srci = parseOpExpConst(ins->u.OPER.assem + 12);
    emitMovImm(NULL, "r1", srci);
    emit(AS_Oper("\tblx __aeabi_idiv", NULL, NULL, NULL));
  } else {
    emit(AS_Move("\tmov r0, `s0", Temp_TempList(armReg2Temp("r0"), NULL),
                 Temp_TempList(srcs->head, NULL)));
    emit(AS_Move("\tmov r1, `s0", Temp_TempList(armReg2Temp("r1"), NULL),
                 Temp_TempList(srcs->tail->head, NULL)));
    emit(AS_Oper("\tblx __aeabi_idiv", NULL, NULL, NULL));
  }
  emit(AS_Move("\tmov `d0, r0", Temp_TempList(dst, NULL),
               Temp_TempList(armReg2Temp("r0"), NULL)));
}

static void munchFdiv(AS_instr ins) {
  ASSERT(ins->u.OPER.dst, "no dest for fdiv");
  Temp_temp dst = ins->u.OPER.dst->head;
  Temp_tempList srcs = ins->u.OPER.src;

  if (!srcs) {
    // parse the two const values
    constPair res = parseOpExpConstPair(ins->u.OPER.assem + 12);
    if (res.const2.f == 0) {
      Temp_temp tmp1 = Temp_newtemp(T_float);
      emitMovImm(tmp1, NULL, res.const1);
      Temp_temp tmp2 = Temp_newtemp(T_float);
      emitMovImm(tmp2, NULL, res.const2);
      emit(AS_Oper("\tvdiv.f32 `d0, `s0, `s1", Temp_TempList(dst, NULL),
                   Temp_TempList(tmp1, Temp_TempList(tmp2, NULL)), NULL));
    } else {
      emitMovImm(dst, NULL, (uf){.f = res.const1.f / res.const2.f});
    }
  } else if (!srcs->tail) {
    Temp_temp srct = srcs->head;
    uf srcf = parseOpExpConst(ins->u.OPER.assem + 12);
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
  ASSERT(ins->u.OPER.dst, "no dest for f2i");
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
  ASSERT(ins->u.OPER.dst, "no dest for i2f");
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

static void munchI2P(AS_instr ins) {
  ASSERT(ins->u.OPER.dst, "no dest for i2p");
  ASSERT(ins->u.OPER.dst->tail == NULL, "i2p dst should only have one temp");
  // just move the src to dst
  Temp_temp dst = ins->u.OPER.dst->head;
  Temp_tempList srcs = ins->u.OPER.src;
  if (!srcs) {
    // parse the const value
    char *s = ins->u.OPER.assem + 16;
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
    emitMovImm(dst, NULL, (uf){.i = src});
  } else {
    ASSERT(srcs->tail == NULL, "i2p srcs should only have one temp");
    emit(AS_Move("\tmov `d0, `s0", Temp_TempList(dst, NULL), srcs));
  }
}

static void munchP2I(AS_instr ins) {
  ASSERT(ins->u.OPER.dst, "no dest for p2i");
  ASSERT(ins->u.OPER.dst->tail == NULL, "p2i dst should only have one temp");
  ASSERT(ins->u.OPER.src, "no src for p2i");
  ASSERT(ins->u.OPER.src->tail == NULL, "p2i src should only have one temp");
  // just move the src to dst
  emit(AS_Move("\tmov `d0, `s0", ins->u.OPER.dst, ins->u.OPER.src));
}

static void munchName(AS_instr ins) {
  ASSERT(ins->u.OPER.dst, "no dest for name");
  ASSERT(ins->u.OPER.dst->tail == NULL, "name dst should only have one temp");
  char *s = strchr(ins->u.OPER.assem, '@');
  ASSERT(s, "name should have @");
  s++;
  char *end = s;
  while (*end != ' ') {
    end++;
  }
  char *name = checked_malloc(end - s + 1);
  strncpy(name, s, end - s);
  name[end - s] = '\0';
  emit(AS_Oper(Stringf("\tldr `d0, = %s", name), ins->u.OPER.dst, NULL, NULL));
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

static void munchCall(AS_instr ins) {
  ASSERT(ins->u.OPER.dst, "no dest for call");
  ASSERT(ins->u.OPER.dst->tail == NULL, "call dst should only have one temp");
  ASSERT(ins->u.OPER.src && ins->u.OPER.src->tail,
         "call src should have at least two temps");
  // parse the method address
  Temp_temp methAddr = ins->u.OPER.src->head;

  // parse the args
  Temp_tempList tempArgs = ins->u.OPER.src->tail;

  int intArgNum = 0, floatArgNum = 0;
  int stackArgNum = 0;
  char *s = strchr(ins->u.OPER.assem, '(');
  s++;

  // parse the args
  while (*s && *s != ')') {
    while (*s && *s == ' ') {
      s++;
    }
    T_type argType;
#ifdef ARMGEN_DEBUG
    fprintf(stderr, "arg type: %c\n", *s);
#endif
    switch (*s) {
      case 'i': {
        argType = T_int;
        break;
      }
      case 'd': {
        argType = T_float;
        break;
      }
      default: {
        fprintf(stderr, "Error: unknown arg type in call\n");
        exit(1);
      }
    }
    while (*s != ' ') {
      s++;  // skip the type
    }
    while (*s == ' ') {
      s++;
    }
    if (*s == '%') {
      // arg is a temp
      Temp_temp arg = tempArgs->head;
      tempArgs = tempArgs->tail;

      switch (argType) {
        case T_int: {
          if (intArgNum < 4) {
            emit(AS_Move(
                Stringf("\tmov r%d, `s0", intArgNum),
                Temp_TempList(armReg2Temp(Stringf("r%d", intArgNum)), NULL),
                Temp_TempList(arg, NULL)));
          } else {
            emit(AS_Oper("\tpush {`s0}", NULL, Temp_TempList(arg, NULL), NULL));
            stackArgNum++;
          }
          intArgNum++;
          break;
        }
        case T_float: {
          if (floatArgNum < 4) {
            emit(AS_Move(
                Stringf("\tvmov.f32 s%d, `s0", floatArgNum),
                Temp_TempList(armReg2Temp(Stringf("s%d", floatArgNum)), NULL),
                Temp_TempList(arg, NULL)));
          } else {
            emit(AS_Oper("\tpush {`s0}", NULL, Temp_TempList(arg, NULL), NULL));
            stackArgNum++;
          }
          floatArgNum++;
          break;
        }
        default: {
          fprintf(stderr, "Error: unknown temp type in call\n");
          exit(1);
        }
      }
    } else {
      // arg is a const val
      switch (argType) {
        case T_int: {
          int arg = atoi(s);
          if (intArgNum < 4) {
            emitMovImm(NULL, Stringf("r%d", intArgNum), (uf){.i = arg});
          } else {
            Temp_temp tmp = Temp_newtemp(T_int);
            emitMovImm(tmp, NULL, (uf){.i = arg});
            emit(AS_Oper("\tpush {`s0}", NULL, Temp_TempList(tmp, NULL), NULL));
            stackArgNum++;
          }
          intArgNum++;
          break;
        }
        case T_float: {
          float arg = atof(s);
          if (floatArgNum < 4) {
            emitMovImm(NULL, Stringf("s%d", floatArgNum), (uf){.f = arg});
          } else {
            Temp_temp tmp = Temp_newtemp(T_float);
            emitMovImm(tmp, NULL, (uf){.f = arg});
            emit(AS_Oper("\tpush {`s0}", NULL, Temp_TempList(tmp, NULL), NULL));
            stackArgNum++;
          }
          floatArgNum++;
          break;
        }
        default: {
          fprintf(stderr, "Error: unknown arg type in call\n");
          exit(1);
        }
      }
    }

    while (*s && *s != ',' && *s != ')') {
      s++;
    }
    while (*s && (*s == ' ' || *s == ',')) {
      s++;
    }
  }

  // call the method
  emit(AS_Oper("\tblx `s0",
               Temp_TempList(armReg2Temp("lr"), getCallerSavedRegs()),
               Temp_TempList(methAddr, NULL), NULL));

  // move the return value to dst
  switch (ins->u.OPER.dst->head->type) {
    case T_int: {
      emit(AS_Move("\tmov `d0, r0", ins->u.OPER.dst,
                   Temp_TempList(armReg2Temp("r0"), NULL)));
      break;
    }
    case T_float: {
      emit(AS_Move("\tvmov.f32 `d0, s0", ins->u.OPER.dst,
                   Temp_TempList(armReg2Temp("s0"), NULL)));
      break;
    }
    default: {
      fprintf(stderr, "Error: unknown temp type\n");
      exit(1);
    }
  }
}

static void munchMalloc(AS_instr ins) {
  ASSERT(ins->u.OPER.dst, "no dest for malloc");
  ASSERT(ins->u.OPER.dst->tail == NULL, "malloc dst should only have one temp");
  if (!ins->u.OPER.src) {
    // parse the const value
    char *s = strchr(ins->u.OPER.assem, '(');
    ASSERT(s, "malloc should have (");
    while (*s != ' ') {
      s++;
    }
    while (*s == ' ') {
      s++;
    }
    int size = atoi(s);
    emitMovImm(NULL, "r0", (uf){.i = size});
    emit(AS_Oper("\tblx malloc", NULL, NULL, NULL));
    emit(AS_Move("\tmov `d0, r0", ins->u.OPER.dst,
                 Temp_TempList(armReg2Temp("r0"), NULL)));
  } else {
    emit(AS_Move("\tmov r0, `s0", Temp_TempList(armReg2Temp("r0"), NULL),
                 ins->u.OPER.src));
    emit(AS_Oper("\tblx malloc", NULL, NULL, NULL));
    emit(AS_Move("\tmov `d0, r0", ins->u.OPER.dst,
                 Temp_TempList(armReg2Temp("r0"), NULL)));
  }
}

static void munchGetNumOrCh(AS_instr ins, string func) {
  ASSERT(ins->u.OPER.dst, "no dest for getIntOrCh");
  ASSERT(!ins->u.OPER.dst->tail, "getIntOrCh dst should only have one temp");
  ASSERT(!ins->u.OPER.src, "getIntOrCh should not have src");

  emit(AS_Oper(Stringf("\tblx %s", func), NULL, NULL, NULL));

  switch (ins->u.OPER.dst->head->type) {
    case T_int: {
      emit(AS_Move("\tmov `d0, r0", ins->u.OPER.dst,
                   Temp_TempList(armReg2Temp("r0"), NULL)));
      break;
    }
    case T_float: {
      emit(AS_Move("\tvmov.f32 `d0, s0", ins->u.OPER.dst,
                   Temp_TempList(armReg2Temp("s0"), NULL)));
      break;
    }
    default: {
      fprintf(stderr, "Error: unknown temp type\n");
      exit(1);
    }
  }
}

static void munchGetArray(AS_instr ins, string func) {
  ASSERT(ins->u.OPER.dst, "no dest for getArray");
  ASSERT(!ins->u.OPER.dst->tail, "getArray dst should only have one temp");
  ASSERT(ins->u.OPER.src, "getArray should have src");
  ASSERT(!ins->u.OPER.src->tail, "getArray src should only have one temp");

  emit(AS_Move("\tmov r0, `s0", Temp_TempList(armReg2Temp("r0"), NULL),
               ins->u.OPER.src));
  emit(AS_Oper(Stringf("\tblx %s", func), NULL, NULL, NULL));
  emit(AS_Move("\tmov `d0, r0", ins->u.OPER.dst,
               Temp_TempList(armReg2Temp("r0"), NULL)));
}

static void munchPutNumOrCh(AS_instr ins, string func) {
  ASSERT(!ins->u.OPER.dst, "putNumOrCh should not have dst");

  if (!ins->u.OPER.src) {
    // parse the const value
    char *s = strchr(ins->u.OPER.assem, '(');
    ASSERT(s, "putNumOrCh should have (");
    s++;
    T_type type;
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
        fprintf(stderr, "Error: unknown type in putNumOrCh\n");
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
        emitMovImm(NULL, "r0", (uf){.i = num});
        break;
      }
      case T_float: {
        float num = atof(s);
        emitMovImm(NULL, "s0", (uf){.f = num});
        break;
      }
    }
  } else {
    ASSERT(!ins->u.OPER.src->tail, "putNumOrCh src should only have one temp");
    switch (ins->u.OPER.src->head->type) {
      case T_int: {
        emit(AS_Move("\tmov r0, `s0", Temp_TempList(armReg2Temp("r0"), NULL),
                     ins->u.OPER.src));
        break;
      }
      case T_float: {
        emit(AS_Move("\tvmov.f32 s0, `s0",
                     Temp_TempList(armReg2Temp("s0"), NULL), ins->u.OPER.src));
        break;
      }
      default: {
        fprintf(stderr, "Error: unknown temp type\n");
        exit(1);
      }
    }
  }

  emit(AS_Oper(Stringf("\tblx %s", func), NULL, NULL, NULL));
}

static void munchPutArray(AS_instr ins, string func) {
  ASSERT(!ins->u.OPER.dst, "putArray should not have dst");
  ASSERT(ins->u.OPER.src, "putArray should have src");

  if (!ins->u.OPER.src->tail) {
    // parse the const value
    char *s = strchr(ins->u.OPER.assem, '(');
    ASSERT(s, "putArray should have (");
    while (*s != ' ') {
      s++;
    }
    while (*s == ' ') {
      s++;
    }

    int num = atoi(s);
    emitMovImm(NULL, "r0", (uf){.i = num});
    emit(AS_Move("\tmov r1, `s0", Temp_TempList(armReg2Temp("r1"), NULL),
                 ins->u.OPER.src));
  } else {
    Temp_temp pos = ins->u.OPER.src->head;
    Temp_temp arr = ins->u.OPER.src->tail->head;
    emit(AS_Move("\tmov r0, `s0", Temp_TempList(armReg2Temp("r0"), NULL),
                 Temp_TempList(pos, NULL)));
    emit(AS_Move("\tmov r1, `s0", Temp_TempList(armReg2Temp("r1"), NULL),
                 Temp_TempList(arr, NULL)));
  }

  emit(AS_Oper(Stringf("\tblx %s", func), NULL, NULL, NULL));
}

static inline void munchTime(string func) {
  emit(AS_Oper(Stringf("\tblx %s", func), NULL, NULL, NULL));
}

static void munchExtCall(AS_instr ins) {
  string assem = ins->u.OPER.assem;
  if (!strncmp(assem, "%`d0 = call i64* @malloc", 24)) {
    munchMalloc(ins);
  } else if (!strncmp(assem, "%`d0 = call i64 @getint", 23)) {
    munchGetNumOrCh(ins, "getint");
  } else if (!strncmp(assem, "%`d0 = call i64 @getch", 22)) {
    munchGetNumOrCh(ins, "getch");
  } else if (!strncmp(assem, "%`d0 = call double @getfloat", 28)) {
    munchGetNumOrCh(ins, "getfloat");
  } else if (!strncmp(assem, "%`d0 = call i64 @getarray", 25)) {
    munchGetArray(ins, "getarray");
  } else if (!strncmp(assem, "%`d0 = call i64 @getfarray", 26)) {
    munchGetArray(ins, "getfarray");
  } else if (!strncmp(assem, "call void @putint", 17)) {
    munchPutNumOrCh(ins, "putint");
  } else if (!strncmp(assem, "call void @putch", 16)) {
    munchPutNumOrCh(ins, "putch");
  } else if (!strncmp(assem, "call void @putfloat", 19)) {
    munchPutNumOrCh(ins, "putfloat");
  } else if (!strncmp(assem, "call void @putarray", 19)) {
    munchPutArray(ins, "putarray");
  } else if (!strncmp(assem, "call void @putfarray", 20)) {
    munchPutArray(ins, "putfarray");
  } else if (!strcmp(assem, "call void @starttime()")) {
    munchTime("starttime");
  } else if (!strcmp(assem, "call void @stoptime()")) {
    munchTime("stoptime");
  } else {
    fprintf(stderr, "Error: unknown external call\n");
    exit(1);
  }
}

static void munchIcmpBr(AS_instr cmp, AS_instr br) {
  Temp_label t = br->u.OPER.jumps->labels->head;

  string assem = cmp->u.OPER.assem;
  // first parse the comparison op
  string op;
  int prelen;
  if (!strncmp(assem, "%`d0 = icmp eq", 14)) {
    op = "eq";
    prelen = 14;
  } else if (!strncmp(assem, "%`d0 = icmp ne", 14)) {
    op = "ne";
    prelen = 14;
  } else if (!strncmp(assem, "%`d0 = icmp slt", 15)) {
    op = "lt";
    prelen = 15;
  } else if (!strncmp(assem, "%`d0 = icmp sgt", 15)) {
    op = "gt";
    prelen = 15;
  } else if (!strncmp(assem, "%`d0 = icmp sle", 15)) {
    op = "le";
    prelen = 15;
  } else if (!strncmp(assem, "%`d0 = icmp sge", 15)) {
    op = "ge";
    prelen = 15;
  } else {
    fprintf(stderr, "Error: unknown icmp op\n");
    exit(1);
  }

  if (!cmp->u.OPER.src) {
    // parse the two const values
    char *s = assem + prelen;
    while (*s == ' ') {
      s++;
    }
    while (*s != ' ') {
      s++;  // skip the type
    }
    while (*s == ' ') {
      s++;
    }
    int src1 = atoi(s);
    while (*s != ',') {
      s++;
    }
    s++;
    while (*s == ' ') {
      s++;
    }
    int src2 = atoi(s);
    bool res;
    if (!strcmp(op, "eq")) {
      res = src1 == src2;
    } else if (!strcmp(op, "ne")) {
      res = src1 != src2;
    } else if (!strcmp(op, "lt")) {
      res = src1 < src2;
    } else if (!strcmp(op, "gt")) {
      res = src1 > src2;
    } else if (!strcmp(op, "le")) {
      res = src1 <= src2;
    } else if (!strcmp(op, "ge")) {
      res = src1 >= src2;
    } else {
      fprintf(stderr, "Error: unknown icmp op\n");
      exit(1);
    }
    if (res) {
      emit(AS_Oper(Stringf("\tmovs `d0, #0", op), cmp->u.OPER.dst, NULL, NULL));
    } else {
      emit(AS_Oper(Stringf("\tmovs `d0, #1", op), cmp->u.OPER.dst, NULL, NULL));
    }
    emit(AS_Oper("\tbeq `j0", NULL, NULL, AS_Targets(Temp_LabelList(t, NULL))));
    return;
  } else if (!cmp->u.OPER.src->tail) {
    Temp_temp src1 = cmp->u.OPER.src->head;
    int src2 = 0;
    char *s = assem + prelen;
    while (*s == ' ') {
      s++;
    }
    while (*s != ' ') {
      s++;  // skip the type
    }
    while (*s == ' ') {
      s++;
    }
    if (*s == '%') {
      // the second src is a temp
      while (*s != ',') {
        s++;
      }
      s++;
      while (*s == ' ') {
        s++;
      }
      src2 = atoi(s);
    } else {
      src2 = atoi(s);
    }

    if (isImm8(src2)) {
      emit(AS_Oper(Stringf("\tcmp `s0, #%d", src2), NULL,
                   Temp_TempList(src1, NULL), NULL));
    } else {
      Temp_temp tmp = Temp_newtemp(T_int);
      emitMovImm(tmp, NULL, (uf){.i = src2});
      emit(AS_Oper("\tcmp `s0, `s1", NULL,
                   Temp_TempList(src1, Temp_TempList(tmp, NULL)), NULL));
    }
  } else {
    emit(AS_Oper("\tcmp `s0, `s1", NULL, cmp->u.OPER.src, NULL));
  }
  emit(AS_Oper(Stringf("\tb%s `j0", op), NULL, NULL,
               AS_Targets(Temp_LabelList(t, NULL))));
}

static void munchFcmpBr(AS_instr cmp, AS_instr br) {
  Temp_label t = br->u.OPER.jumps->labels->head;

  string assem = cmp->u.OPER.assem;
  // first parse the comparison op
  string op;
  int prelen;
  if (!strncmp(assem, "%`d0 = fcmp oeq", 15)) {
    op = "eq";
    prelen = 15;
  } else if (!strncmp(assem, "%`d0 = fcmp one", 15)) {
    op = "ne";
    prelen = 15;
  } else if (!strncmp(assem, "%`d0 = fcmp olt", 15)) {
    op = "lt";
    prelen = 15;
  } else if (!strncmp(assem, "%`d0 = fcmp ogt", 15)) {
    op = "gt";
    prelen = 15;
  } else if (!strncmp(assem, "%`d0 = fcmp ole", 15)) {
    op = "le";
    prelen = 15;
  } else if (!strncmp(assem, "%`d0 = fcmp oge", 15)) {
    op = "ge";
    prelen = 15;
  } else {
    fprintf(stderr, "Error: unknown fcmp op\n");
    exit(1);
  }

  if (!cmp->u.OPER.src) {
    // parse the two const values
    char *s = assem + prelen;
    while (*s == ' ') {
      s++;
    }
    while (*s != ' ') {
      s++;  // skip the type
    }
    while (*s == ' ') {
      s++;
    }
    float src1 = atof(s);
    while (*s != ',') {
      s++;
    }
    s++;
    while (*s == ' ') {
      s++;
    }
    float src2 = atof(s);
    bool res;
    if (!strcmp(op, "eq")) {
      res = src1 == src2;
    } else if (!strcmp(op, "ne")) {
      res = src1 != src2;
    } else if (!strcmp(op, "lt")) {
      res = src1 < src2;
    } else if (!strcmp(op, "gt")) {
      res = src1 > src2;
    } else if (!strcmp(op, "le")) {
      res = src1 <= src2;
    } else if (!strcmp(op, "ge")) {
      res = src1 >= src2;
    } else {
      fprintf(stderr, "Error: unknown fcmp op\n");
      exit(1);
    }
    if (res) {
      emit(AS_Oper(Stringf("\tmovs `d0, #0", op), cmp->u.OPER.dst, NULL, NULL));
    } else {
      emit(AS_Oper(Stringf("\tmovs `d0, #1", op), cmp->u.OPER.dst, NULL, NULL));
    }
    emit(AS_Oper("\tbeq `j0", NULL, NULL, AS_Targets(Temp_LabelList(t, NULL))));
    return;
  } else if (!cmp->u.OPER.src->tail) {
    Temp_temp src1 = cmp->u.OPER.src->head;
    float src2 = 0;
    char *s = assem + prelen;
    while (*s == ' ') {
      s++;
    }
    while (*s != ' ') {
      s++;  // skip the type
    }
    while (*s == ' ') {
      s++;
    }
    if (*s == '%') {
      // the second src is a temp
      while (*s != ',') {
        s++;
      }
      s++;
      while (*s == ' ') {
        s++;
      }
      src2 = atof(s);
    } else {
      src2 = atof(s);
    }

    Temp_temp tmp = Temp_newtemp(T_float);
    emitMovImm(tmp, NULL, (uf){.f = src2});
    emit(AS_Oper("\tvcmp.f32 `s0, `s1", NULL,
                 Temp_TempList(src1, Temp_TempList(tmp, NULL)), NULL));
  } else {
    emit(AS_Oper("\tvcmp.f32 `s0, `s1", NULL, cmp->u.OPER.src, NULL));
  }
  emit(AS_Oper("\tvmrs APSR_nzcv, FPSCR", NULL, NULL, NULL));
  emit(AS_Oper(Stringf("\tb%s `j0", op), NULL, NULL,
               AS_Targets(Temp_LabelList(t, NULL))));
}

AS_instrList armbody(AS_instrList il) {
  iList = last = NULL;

  for (AS_instrList p = il; p; p = p->tail) {
    AS_instr ins = p->head;
    AS_type type = gettype(ins);
    if (type == NONE) {
      fprintf(stderr, "Error: unknown instruction type, %s\n",
              ins->u.OPER.assem);
      exit(1);
    }
    if (type == ICMP || type == FCMP) {
      ASSERT(p->tail && gettype(p->tail->head) == CJUMP,
             "icmp/fcmp should be followed by cjump");
      if (type == ICMP) {
        munchIcmpBr(ins, p->tail->head);
      } else {
        munchFcmpBr(ins, p->tail->head);
      }
      p = p->tail;
      continue;
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
        munchRet(ins);
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
        munchI2P(ins);
        break;
      }
      case P2I: {
        munchP2I(ins);
        break;
      }
      case NAME: {
        munchName(ins);
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
      case CALL: {
        munchCall(ins);
        break;
      }
      case EXTCALL: {
        munchExtCall(ins);
        break;
      }
      default: {
        fprintf(stderr, "Error: unknown instruction type\n");
        exit(1);
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
  emit(AS_Oper("\tpush {fp}", NULL, NULL, NULL));
  // set the frame pointer
  emit(AS_Oper("\tmov fp, sp", NULL, NULL, NULL));
  // save the link register
  emit(AS_Oper("\tpush {lr}", NULL, Temp_TempList(armReg2Temp("lr"), NULL),
               NULL));

  // TODO: save the callee-saved registers
  emit(AS_Oper("\tpush {r4, r5, r6, r7, r8, r9, r10}", NULL, NULL, NULL));

  // parse the args
  int intArgNum = 0, floatArgNum = 0;
  int stackArgNum = 0;
  for (Temp_tempList args = ins->u.OPER.src; args; args = args->tail) {
    Temp_temp arg = args->head;
    switch (arg->type) {
      case T_int: {
        if (intArgNum < 4) {
          emit(AS_Move(
              Stringf("\tmov `d0, r%d", intArgNum), Temp_TempList(arg, NULL),
              Temp_TempList(armReg2Temp(Stringf("r%d", intArgNum)), NULL)));
        } else {
          emit(AS_Oper(
              Stringf("\tldr `d0, [fp, #%d]", (++stackArgNum) * ARCH_SIZE),
              Temp_TempList(arg, NULL), NULL, NULL));
        }
        ++intArgNum;
        break;
      }
      case T_float: {
        if (floatArgNum < 16) {
          emit(AS_Move(
              Stringf("\tvmov.f32 `d0, s%d", floatArgNum),
              Temp_TempList(arg, NULL),
              Temp_TempList(armReg2Temp(Stringf("s%d", floatArgNum)), NULL)));
        } else {
          emit(AS_Oper(
              Stringf("\tvldr.f32 `d0, [fp, #%d]", (++stackArgNum) * ARCH_SIZE),
              Temp_TempList(arg, NULL), NULL, NULL));
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

AS_instrList armepilog(AS_instrList il) {
#ifdef ARMGEN_DEBUG
  fprintf(stderr, "armepilog: assem=%s\n", il->head->u.OPER.assem);
#endif
  iList = last = NULL;

  // nothing to do
  return iList;
}