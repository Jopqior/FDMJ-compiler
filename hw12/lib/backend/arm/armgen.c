#include "armgen.h"

#define ARMGEN_DEBUG
// #undef ARMGEN_DEBUG

#define ARCH_SIZE 4
#define TYPELEN 10

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
  CJUMP
} AS_type;

AS_type gettype(AS_instr ins) {
  AS_type ret = NONE;
  string assem = ins->u.OPER.assem;
  if (ins->kind == I_MOVE) {
    assem = ins->u.MOVE.assem;
  } else if (ins->kind == I_LABEL) {
    ret = LABEL;
    return ret;
  }
  if (!strncmp(assem, "br", TYPELEN)) {
    ret = BR;
    return ret;
  } else if (!strncmp(assem, "ret", TYPELEN)) {
    ret = RET;
    return ret;
  } else if (!strncmp(assem, "%`d0 = fadd", TYPELEN)) {
    ret = FADD;
    return ret;
  } else if (!strncmp(assem, "%`d0 = add", TYPELEN)) {
    ret = ADD;
    return ret;
  } else if (!strncmp(assem, "%`d0 = fsub", TYPELEN)) {
    ret = FSUB;
    return ret;
  } else if (!strncmp(assem, "%`d0 = sub", TYPELEN)) {
    ret = SUB;
    return ret;
  } else if (!strncmp(assem, "%`d0 = fmul", TYPELEN)) {
    ret = FMUL;
    return ret;
  } else if (!strncmp(assem, "%`d0 = mul", TYPELEN)) {
    ret = MUL;
    return ret;
  } else if (!strncmp(assem, "%`d0 = fdiv", TYPELEN)) {
    ret = FDIV;
    return ret;
  } else if (!strncmp(assem, "%`d0 = div", TYPELEN)) {
    ret = DIV;
    return ret;
  } else if (!strncmp(assem, "%`d0 = fptosi", TYPELEN)) {
    ret = F2I;
    return ret;
  } else if (!strncmp(assem, "%`d0 = sitofp", TYPELEN)) {
    ret = I2F;
    return ret;
  } else if (!strncmp(assem, "%`d0 = inttoptr", TYPELEN)) {
    ret = I2P;
    return ret;
  } else if (!strncmp(assem, "%`d0 = load", TYPELEN)) {
    ret = LOAD;
    return ret;
  } else if (!strncmp(assem, "%`store", TYPELEN)) {
    ret = STORE;
    return ret;
  } else if (!strncmp(assem, "%`d0 = ptrtoint", TYPELEN)) {
    ret = P2I;
    return ret;
  } else if (!strncmp(assem, "%`d0 = call", TYPELEN)) {
    ret = CALL;
    return ret;
  } else if (!strncmp(assem, "call", TYPELEN)) {
    ret = EXTCALL;
    return ret;
  } else if (!strncmp(assem, "%`d0 = icmp", TYPELEN)) {
    ret = ICMP;
    return ret;
  } else if (!strncmp(assem, "%`d0 = fcmp", TYPELEN)) {
    ret = FCMP;
    return ret;
  } else if (!strncmp(assem, "br i1", TYPELEN)) {
    ret = CJUMP;
    return ret;
  }
  return ret;
}

AS_instrList armbody(AS_instrList il) {
#ifdef ARMGEN_DEBUG
  fprintf(stderr, "armbody: assem=%s\n", il->head->u.OPER.assem);
#endif
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

AS_instrList armepilog(AS_instrList il) {
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

  // restore the callee-saved registers
  for (int i = 10; i >= 4; i--) {
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