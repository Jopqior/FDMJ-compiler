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
// #undef ARMGEN_DEBUG

#define ARCH_SIZE 4
#define TYPELEN 10

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
        emit(AS_Oper(Stringf("\tmov `d0, #%d", imml), Temp_TempList(dt, NULL),
                     NULL, NULL));
        if (immu) {
          emit(AS_Oper(Stringf("\tmovt `d0, #%d", immu),
                       Temp_TempList(dt, NULL), NULL, NULL));
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
        emit(AS_Oper(Stringf("\tmov %s, #%d", ds, imml), NULL, NULL, NULL));
        if (immu) {
          emit(AS_Oper(Stringf("\tmovt %s, #%d", ds, immu), NULL, NULL, NULL));
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
        emit(AS_Oper(Stringf("\tvmov.f32 %s, `s0", ds), NULL,
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
        // TODO: MAP
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
        // TODO: MAP
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
        // TODO: MAP
        emit(AS_Move("\tmov r0, `s0", NULL, Temp_TempList(ret, NULL)));
        break;
      }
      case T_float: {
        // TODO: MAP
        emit(AS_Move("\tvmov.f32 s0, `s0", NULL, Temp_TempList(ret, NULL)));
        break;
      }
    }
  }

  emit(AS_Oper("\tb .`j0", NULL, NULL,
               AS_Targets(Temp_LabelList(retLabel, NULL))));
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
  emit(AS_Oper(Stringf("\tstr fp, [sp, #-%d]!", ARCH_SIZE * 2), NULL, NULL,
               NULL));
  // save the link register
  emit(AS_Oper(Stringf("\tstr lr, [sp, #%d]", ARCH_SIZE), NULL, NULL, NULL));
  // set the frame pointer
  emit(AS_Oper(Stringf("\tadd fp, sp, #%d", ARCH_SIZE), NULL, NULL, NULL));

  // save the callee-saved registers
  emit(AS_Oper(Stringf("\tsub sp, sp, #%d", ARCH_SIZE * 7), NULL, NULL, NULL));
  for (int i = 10; i >= 4; i--) {
    emit(AS_Oper(Stringf("\tstr r%d, [fp, #-%d]", i, (12 - i) * ARCH_SIZE),
                 NULL, NULL, NULL));
  }

  // parse the args
  int intArgNum = 0, floatArgNum = 0;
  int stackArgNum = 0;
  for (Temp_tempList args = ins->u.OPER.src; args; args = args->tail) {
    Temp_temp arg = args->head;
    switch (arg->type) {
      case T_int: {
        if (intArgNum < 4) {
          // TODO: MAP
          emit(AS_Move(Stringf("\tmov `d0, r%d", intArgNum),
                       Temp_TempList(arg, NULL), NULL));
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
          // TODO: MAP
          emit(AS_Move(Stringf("\tvmov.f32 `d0, s%d", floatArgNum),
                       Temp_TempList(arg, NULL), NULL));
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

AS_instrList armepilog(AS_instrList il, Temp_label retLabel) {
#ifdef ARMGEN_DEBUG
  fprintf(stderr, "armepilog: assem=%s\n", il->head->u.OPER.assem);
#endif
  iList = last = NULL;

  // check if the epilog is valid
  if (il->tail) {
    fprintf(stderr, "Error: epilog should only have one instruction\n");
    exit(1);
  }
  AS_instr ins = il->head;
  if (strncmp(ins->u.OPER.assem, "}", 1)) {
    fprintf(stderr, "Error: epilog should end with }\n");
    exit(1);
  }

  // emit some epilog code
  emit(AS_Label(Stringf(".%s:", Temp_labelstring(retLabel)), retLabel));

  // restore the callee-saved registers
  for (int i = 10; i >= 4; i--) {
    // TODO: MAP
    emit(AS_Oper(Stringf("\tldr r%d, [fp, #-%d]", i, (12 - i) * ARCH_SIZE),
                 NULL, NULL, NULL));
  }

  // restore the frame pointer
  emit(AS_Oper(Stringf("\tsub sp, fp, #%d", ARCH_SIZE), NULL, NULL, NULL));
  emit(AS_Oper("\tldr fp, [sp]", NULL, NULL, NULL));

  // jump back to the link register
  emit(AS_Oper(Stringf("\tadd sp, sp, #%d", ARCH_SIZE), NULL, NULL, NULL));
  emit(AS_Oper(Stringf("\tldr pc, [sp], #%d", ARCH_SIZE), NULL, NULL, NULL));

  return iList;
}