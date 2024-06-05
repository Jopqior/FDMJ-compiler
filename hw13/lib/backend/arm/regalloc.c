#include "regalloc.h"

#include "bitmap.h"

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

#define COLOR_DEBUG
#undef COLOR_DEBUG

#define REGALLOC_DEBUG
#undef REGALLOC_DEBUG

#define ARCH_SIZE 4

#define ARM_REG_START 0
#define FLOAT_REG_START 20

#define MAX_NUM_REG 10       // r0-r8 + lr, and r9~r10 are reserved for spill
#define MAX_NUM_FLOATREG 30  // s0-s31, and s14~s15 are reserved for spill

typedef struct COL_tempInfo_ *COL_tempInfo;
struct COL_tempInfo_ {
  G_node node;
  int degree;
  int regId;
};

typedef struct COL_result_ *COL_result;
struct COL_result_ {
  Temp_map coloring;
  Temp_tempList spills;
  int maxRegId;
  int maxFloatRegId;
};

static TAB_table colEnv = NULL;
static G_nodeList simplyfyWorklist = NULL;
static G_nodeList spillWorklist = NULL;
static G_nodeList selectStack = NULL;

static int regId2Color[] = {0, 1, 2, 3, 14, 4, 5, 6, 7, 8};
static int floatRegId2Color[] = {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,
                                 10, 11, 12, 13, 16, 17, 18, 19, 20, 21,
                                 22, 23, 24, 25, 26, 27, 28, 29, 30, 31};
static string regId2Name[] = {"r0", "r1", "r2", "r3", "lr",
                              "r4", "r5", "r6", "r7", "r8"};
static string floatRegId2Name[] = {
    "s0",  "s1",  "s2",  "s3",  "s4",  "s5",  "s6",  "s7",  "s8",  "s9",
    "s10", "s11", "s12", "s13", "s16", "s17", "s18", "s19", "s20", "s21",
    "s22", "s23", "s24", "s25", "s26", "s27", "s28", "s29", "s30", "s31"};

static bool Temp_isPrecolored(Temp_temp temp) { return temp->num < 99; }

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

static inline int COL_regId2Color(int regId) {
  ASSERT(regId < 10, "regId >= 10");
  return regId2Color[regId];
}

static inline int COL_floatRegId2Color(int regId) {
  ASSERT(regId < 30, "regId >= 30");
  return floatRegId2Color[regId];
}

static inline int COL_color2RegId(int color) {
  for (int i = 0; i < 10; ++i) {
    if (regId2Color[i] == color) return i;
  }
  return -1;
}

static inline int COL_floatColor2RegId(int color) {
  ASSERT(color >= 0 && color < 32 && color != 14 && color != 15,
         "Invalid color");
  if (color < 14) return color;
  return color - 2;
}

static COL_tempInfo COL_TempInfo(G_node node, int degree, int regId) {
  COL_tempInfo info = checked_malloc(sizeof(*info));
  info->node = node;
  info->degree = degree;
  info->regId = regId;
  return info;
}

static COL_result COL_Result(Temp_map coloring, Temp_tempList spills,
                             int maxRegId, int maxFloatRegId) {
  COL_result res = checked_malloc(sizeof(*res));
  res->coloring = coloring;
  res->spills = spills;
  res->maxRegId = maxRegId;
  res->maxFloatRegId = maxFloatRegId;
  return res;
}

static RA_result RA_Result(Temp_map coloring, AS_instrList il) {
  RA_result res = checked_malloc(sizeof(*res));
  res->coloring = coloring;
  res->il = il;
  return res;
}

static void COL_removeNode(G_node n, G_nodeList *list) {
  if (*list == NULL) return;
  if ((*list)->head == n) {
    *list = (*list)->tail;
    return;
  }
  for (G_nodeList nl = *list; nl->tail; nl = nl->tail) {
    if (nl->tail->head == n) {
      nl->tail = nl->tail->tail;
      return;
    }
  }
}

static void COL_initPreColor(Temp_map coloring) {
#ifdef COLOR_DEBUG
  fprintf(stderr, "Init precolor\n");
#endif
  ASSERT(coloring, "coloring is NULL");
  Temp_temp reg;
  for (int i = 0; i <= 10; ++i) {
    reg = Temp_namedtemp(i + ARM_REG_START, T_int);
    Temp_enter(coloring, reg, Stringf("r%d", i));
  }
  reg = Temp_namedtemp(14 + ARM_REG_START, T_int);
  Temp_enter(coloring, reg, "lr");

  for (int i = 0; i <= 15; ++i) {
    reg = Temp_namedtemp(i + FLOAT_REG_START, T_float);
    Temp_enter(coloring, reg, Stringf("s%d", i));
  }
#ifdef COLOR_DEBUG
  fprintf(stderr, "Finish init precolor\n");
#endif
}

static void COL_initColEnv(G_graph ig) {
  colEnv = TAB_empty();

  for (G_nodeList nl = G_nodes(ig); nl; nl = nl->tail) {
    G_node n = nl->head;
    Temp_temp temp = G_nodeInfo(n);
    COL_tempInfo info = COL_TempInfo(n, G_degree(n), -1);

    if (Temp_isPrecolored(temp)) {
      switch (temp->type) {
        case T_int:
          info->regId = COL_color2RegId(temp->num - ARM_REG_START);
          break;
        case T_float:
          info->regId = COL_floatColor2RegId(temp->num - FLOAT_REG_START);
          break;
        default:
          ASSERT(0, "Unknown temp type");
      }
    }

    TAB_enter(colEnv, temp, info);
  }
}

static void COL_buildWorklist(G_graph ig) {
#ifdef COLOR_DEBUG
  fprintf(stderr, "Start building worklist\n");
#endif
  for (G_nodeList nl = G_nodes(ig); nl; nl = nl->tail) {
    G_node n = nl->head;
    Temp_temp temp = G_nodeInfo(n);
    if (Temp_isPrecolored(temp)) {
      continue;
    }
    COL_tempInfo info = TAB_look(colEnv, temp);
    switch (temp->type) {
      case T_int:
        if (info->degree >= MAX_NUM_REG) {
          spillWorklist = G_NodeList(n, spillWorklist);
        } else {
          simplyfyWorklist = G_NodeList(n, simplyfyWorklist);
        }
        break;
      case T_float:
        if (info->degree >= MAX_NUM_FLOATREG) {
          spillWorklist = G_NodeList(n, spillWorklist);
        } else {
          simplyfyWorklist = G_NodeList(n, simplyfyWorklist);
        }
        break;
      default:
        ASSERT(0, "Unknown temp type");
    }
  }
#ifdef COLOR_DEBUG
  fprintf(stderr, "Finish building worklist\n");
#endif
}

static void COL_decrementDegree(G_node m) {
  ASSERT(m, "m is NULL");
  Temp_temp temp = G_nodeInfo(m);
  if (Temp_isPrecolored(temp)) return;

  COL_tempInfo info = TAB_look(colEnv, temp);
#ifdef COLOR_DEBUG
  fprintf(stderr, "Decrement degree of t%d, %d -> %d\n", temp->num,
          info->degree, info->degree - 1);
#endif
  info->degree--;
  switch (temp->type) {
    case T_int: {
      if (info->degree == MAX_NUM_REG - 1) {
        COL_removeNode(m, &spillWorklist);
        simplyfyWorklist = G_NodeList(m, simplyfyWorklist);
      }
      break;
    }
    case T_float: {
      if (info->degree == MAX_NUM_FLOATREG - 1) {
        COL_removeNode(m, &spillWorklist);
        simplyfyWorklist = G_NodeList(m, simplyfyWorklist);
      }
      break;
    }
    default:
      ASSERT(0, "Unknown temp type");
  }
#ifdef COLOR_DEBUG
  fprintf(stderr, "Finish decrement degree\n");
#endif
}

static void COL_simplify() {
#ifdef COLOR_DEBUG
  fprintf(stderr, "Start simplify\n");
#endif
  G_node n = simplyfyWorklist->head;

  // remove n from simplyfyWorklist
  simplyfyWorklist = simplyfyWorklist->tail;

  // push n to selectStack
  selectStack = G_NodeList(n, selectStack);
#ifdef COLOR_DEBUG
  fprintf(stderr, "Select node t%d\n", ((Temp_temp)G_nodeInfo(n))->num);
#endif

#ifdef COLOR_DEBUG
  fprintf(stderr, "Remove edges\n");
  fprintf(stderr, "Edges: ");
  for (G_nodeList nl = G_adj(n); nl; nl = nl->tail) {
    fprintf(stderr, "t%d ", ((Temp_temp)G_nodeInfo(nl->head))->num);
  }
  fprintf(stderr, "\n");
#endif

  for (G_nodeList nl = G_adj(n); nl; nl = nl->tail) {
    G_node m = nl->head;
    COL_decrementDegree(m);
  }
#ifdef COLOR_DEBUG
  fprintf(stderr, "Finish simplify\n");
#endif
}

static inline int COL_spillCost(G_node n) {
  COL_tempInfo info = TAB_look(colEnv, G_nodeInfo(n));
  return info->degree;
}

static void COL_spill(COL_result res) {
  G_node m = spillWorklist->head;
  for (G_nodeList nl = spillWorklist->tail; nl; nl = nl->tail) {
    if (COL_spillCost(nl->head) > COL_spillCost(m)) {
      m = nl->head;
    }
  }
  COL_removeNode(m, &spillWorklist);
  // to simplify, actually spill it, don't use potential spill
  res->spills = Temp_TempList(G_nodeInfo(m), res->spills);
  for (G_nodeList nl = G_adj(m); nl; nl = nl->tail) {
    COL_decrementDegree(nl->head);
  }
}

static void COL_assignColor(COL_result res) {
  Temp_map coloring = res->coloring;
  for (G_nodeList nl = selectStack; nl; nl = nl->tail) {
    G_node n = nl->head;
    Temp_temp temp = G_nodeInfo(n);
    switch (temp->type) {
      case T_int: {
        bitmap b = Bitmap(MAX_NUM_REG);
        bitmap_set_all(b);
        for (G_nodeList nl = G_adj(n); nl; nl = nl->tail) {
          Temp_temp t = G_nodeInfo(nl->head);
          COL_tempInfo info = TAB_look(colEnv, t);
          ASSERT(info, Stringf("t%d's info is NULL", t->num));
          if (info->regId != -1) {
            bitmap_clear(b, info->regId);
          }
        }
        int c = bitmap_get_first(b);
        ASSERT(c != -1, "No available color");
        if (c > res->maxRegId) {
          res->maxRegId = c;
        }

        COL_tempInfo info = TAB_look(colEnv, temp);
        info->regId = c;
        Temp_enter(coloring, temp, Stringf("r%d", COL_regId2Color(c)));
        break;
      }
      case T_float: {
        bitmap b = Bitmap(MAX_NUM_FLOATREG);
        bitmap_set_all(b);
        for (G_nodeList nl = G_adj(n); nl; nl = nl->tail) {
          Temp_temp t = G_nodeInfo(nl->head);
          COL_tempInfo info = TAB_look(colEnv, t);
          ASSERT(info, "info is NULL");
          if (info->regId != -1) {
            bitmap_clear(b, info->regId);
          }
        }
        int c = bitmap_get_first(b);
        ASSERT(c != -1, "No available color");
        if (c > res->maxFloatRegId) {
          res->maxFloatRegId = c;
        }

        COL_tempInfo info = TAB_look(colEnv, temp);
        info->regId = c;
        Temp_enter(coloring, temp, Stringf("s%d", COL_floatRegId2Color(c)));
        break;
      }
      default:
        ASSERT(0, "type error");
    }
  }
}

static COL_result COL_color(G_graph ig) {
#ifdef COLOR_DEBUG
  fprintf(stderr, "Start coloring\n");
#endif
  COL_result res = COL_Result(Temp_empty(), NULL, -1, -1);
  COL_initPreColor(res->coloring);
  COL_initColEnv(ig);
  COL_buildWorklist(ig);
#ifdef COLOR_DEBUG
  fprintf(stderr, "simplifyWorklist: ");
  for (G_nodeList nl = simplyfyWorklist; nl; nl = nl->tail) {
    fprintf(stderr, "t%d ", ((Temp_temp)G_nodeInfo(nl->head))->num);
  }
  fprintf(stderr, "\n");
  fprintf(stderr, "spillWorklist: ");
  for (G_nodeList nl = spillWorklist; nl; nl = nl->tail) {
    fprintf(stderr, "t%d ", ((Temp_temp)G_nodeInfo(nl->head))->num);
  }
  fprintf(stderr, "\n");
#endif

  while (simplyfyWorklist || spillWorklist) {
    if (simplyfyWorklist) {
      COL_simplify();
    } else if (spillWorklist) {
      COL_spill(res);
    }
  }
#ifdef COLOR_DEBUG
  fprintf(stderr, "Select stack: ");
  for (G_nodeList nl = selectStack; nl; nl = nl->tail) {
    fprintf(stderr, "t%d ", ((Temp_temp)G_nodeInfo(nl->head))->num);
  }
  fprintf(stderr, "\n");
  fprintf(stderr, "Spills: ");
  for (Temp_tempList tl = res->spills; tl; tl = tl->tail) {
    fprintf(stderr, "t%d ", tl->head->num);
  }
  fprintf(stderr, "\n");
#endif

  COL_assignColor(res);
#ifdef COLOR_DEBUG
  fprintf(stderr, "Coloring: \n");
  for (G_nodeList nl = G_nodes(ig); nl; nl = nl->tail) {
    Temp_temp temp = G_nodeInfo(nl->head);
    if (Temp_isPrecolored(temp)) continue;
    fprintf(stderr, "t%d: %s\n", temp->num, Temp_look(res->coloring, temp));
  }
#endif

  // free worklist and env
  simplyfyWorklist = NULL;
  spillWorklist = NULL;
  selectStack = NULL;
  colEnv = NULL;

#ifdef COLOR_DEBUG
  fprintf(stderr, "Finish coloring\n");
#endif
  return res;
}

static void RA_intMapEnter(TAB_table t, Temp_temp temp, int value) {
  ASSERT(t, "t is NULL");
  int *p = TAB_look(t, temp);
  if (p) {
    *p = value;
  } else {
    p = checked_malloc(sizeof(*p));
    *p = value;
    TAB_enter(t, temp, p);
  }
}

static int RA_intMapLook(TAB_table t, Temp_temp temp) {
  ASSERT(t, "t is NULL");
  int *p = TAB_look(t, temp);
  if (p) {
    return *p;
  }
  return -1;
}

typedef struct {
  TAB_table spillMap;
  int spillStackSize;
  int num_reservedRegUsed;
  int regs;
  int floatRegs;
} RA_spillInfo;

static inline int RA_getCalleeStackSize(int maxRegId) {
  return maxRegId > 4 ? maxRegId - 4 : 0;
}

static inline int RA_getFloatCalleeStackSize(int maxFloatRegId) {
  return maxFloatRegId > 13 ? maxFloatRegId - 13 : 0;
}

static RA_spillInfo RA_computeStackSize(RA_result res, COL_result colRes) {
#ifdef REGALLOC_DEBUG
  fprintf(stderr, "Start computing stack size\n");
#endif
  Temp_tempList spills = colRes->spills;

  // record the spill offset
  int spillStackSize = 0;
  TAB_table spillMap = TAB_empty();
  for (Temp_tempList tl = spills; tl; tl = tl->tail) {
    Temp_temp temp = tl->head;
    RA_intMapEnter(spillMap, temp, spillStackSize++);
  }

  // insert spill code
  int num_reservedRegUsed = 0;
  if (spills) {
    for (AS_instrList cur = res->il; cur; cur = cur->tail) {
      AS_instr instr = cur->head;
      if (instr->kind == I_OPER) {
        Temp_tempList def = instr->u.OPER.dst, use = instr->u.OPER.src;
        int tmp_num_reservedRegUsed = 0;
        for (Temp_tempList tl = use; tl; tl = tl->tail) {
          Temp_temp temp = tl->head;
          if (temp->type == T_float) {
            continue;
          }
          int offset = RA_intMapLook(spillMap, temp);
          if (offset != -1) {
            ++tmp_num_reservedRegUsed;
          }
        }
        if (tmp_num_reservedRegUsed > num_reservedRegUsed) {
          num_reservedRegUsed = tmp_num_reservedRegUsed;
        }
      }
      if (num_reservedRegUsed == 2) {
        break;
      }
    }
  }

  // calculate total stack size
  int regs = RA_getCalleeStackSize(colRes->maxRegId);
  int floatRegs = RA_getFloatCalleeStackSize(colRes->maxFloatRegId);

  RA_spillInfo spillInfo = {.spillMap = spillMap,
                            .spillStackSize = spillStackSize,
                            .num_reservedRegUsed = num_reservedRegUsed,
                            .regs = regs,
                            .floatRegs = floatRegs};
#ifdef REGALLOC_DEBUG
  fprintf(stderr, "Finish computing stack size\n");
  fprintf(stderr, "Spill stack size: %d\n", spillInfo.spillStackSize);
  fprintf(stderr, "Num reserved reg used: %d\n", spillInfo.num_reservedRegUsed);
  fprintf(stderr, "Regs: %d\n", spillInfo.regs);
  fprintf(stderr, "FloatRegs: %d\n", spillInfo.floatRegs);
#endif
  return spillInfo;
}

static inline bool RA_isProlog(AS_instr instr) {
  return instr->kind == I_OPER &&
         !strcmp(instr->u.OPER.assem, "\tpush {r4, r5, r6, r7, r8, r9, r10}");
}

static inline bool RA_isReturn(AS_instr instr) {
  return instr->kind == I_OPER &&
         !strcmp(instr->u.OPER.assem, "\tsub sp, fp, #32");
}

static inline string RA_getToStoreRegs(int regs, int num_reservedRegUsed) {
  string toPushAndPop = checked_calloc(IR_MAXLEN, sizeof(char));
  strcat(toPushAndPop, "{");

  for (int i = 0; i < regs; ++i) {
    strcat(toPushAndPop, regId2Name[4 + i]);
    if (i != regs - 1) {
      strcat(toPushAndPop, ", ");
    }
  }

  for (int i = 0; i < num_reservedRegUsed; ++i) {
    strcat(toPushAndPop, Stringf("r%d", 9 + i));
    if (i != num_reservedRegUsed - 1) {
      strcat(toPushAndPop, ", ");
    }
  }

  strcat(toPushAndPop, "}");

  return toPushAndPop;
}

static inline string RA_getToStoreFloatRegs(int floatRegs) {
  string toPushAndPop = checked_calloc(IR_MAXLEN, sizeof(char));
  strcat(toPushAndPop, "{");

  for (int i = 0; i < floatRegs; ++i) {
    strcat(toPushAndPop, floatRegId2Name[14 + i]);
    if (i != floatRegs - 1) {
      strcat(toPushAndPop, ", ");
    }
  }

  strcat(toPushAndPop, "}");

  return toPushAndPop;
}

static void RA_realSpill(RA_result res, RA_spillInfo spillInfo) {
#ifdef REGALLOC_DEBUG
  fprintf(stderr, "Start real spill\n");
#endif
  string toStoreRegs = NULL;
  if (spillInfo.regs || spillInfo.num_reservedRegUsed) {
    toStoreRegs =
        RA_getToStoreRegs(spillInfo.regs, spillInfo.num_reservedRegUsed);
  }
  string toStoreFloatRegs = NULL;
  if (spillInfo.floatRegs) {
    toStoreFloatRegs = RA_getToStoreFloatRegs(spillInfo.floatRegs);
  }

  AS_instrList pre = res->il;
  AS_instrList cur = pre->tail;
  while (cur) {
    if (RA_isProlog(cur->head)) {
      // delete origin push instruction
      pre->tail = cur->tail;
      cur = cur->tail;
      AS_instrList tmpCur = cur;

      // if need, add spill stack size
      if (spillInfo.spillStackSize) {
        AS_instr subSp = AS_Oper(
            Stringf("\tsub sp, sp, #%d", spillInfo.spillStackSize * ARCH_SIZE),
            NULL, NULL, NULL);
        pre->tail = AS_InstrList(subSp, cur);
        cur = pre->tail;
      }

      if (toStoreFloatRegs) {
        AS_instr newInstr =
            AS_Oper(Stringf("\tvpush %s", toStoreFloatRegs), NULL, NULL, NULL);
        pre->tail = AS_InstrList(newInstr, cur);
        cur = pre->tail;
      }

      if (toStoreRegs) {
        AS_instr newInstr =
            AS_Oper(Stringf("\tpush %s", toStoreRegs), NULL, NULL, NULL);
        pre->tail = AS_InstrList(newInstr, cur);
        cur = pre->tail;
      }

      cur = tmpCur;
      while (pre->tail != cur) {
        pre = pre->tail;
      }
    } else if (RA_isReturn(cur->head)) {
      // delete origin sub&pop instruction
      ASSERT(cur->tail, "No pop instruction");
      pre->tail = cur->tail->tail;
      cur = pre->tail;
      AS_instrList tmpCur = cur;

      if (toStoreRegs) {
        AS_instr newInstr =
            AS_Oper(Stringf("\tpop %s", toStoreRegs), NULL, NULL, NULL);
        pre->tail = AS_InstrList(newInstr, cur);
        cur = pre->tail;
      }

      if (toStoreFloatRegs) {
        AS_instr newInstr =
            AS_Oper(Stringf("\tvpop %s", toStoreFloatRegs), NULL, NULL, NULL);
        pre->tail = AS_InstrList(newInstr, cur);
        cur = pre->tail;
      }

      if (spillInfo.spillStackSize) {
        AS_instr addSp = AS_Oper(
            Stringf("\tadd sp, sp, #%d", spillInfo.spillStackSize * ARCH_SIZE),
            NULL, NULL, NULL);
        pre->tail = AS_InstrList(addSp, cur);
        cur = pre->tail;
      }

      cur = tmpCur;
      while (pre->tail != cur) {
        pre = pre->tail;
      }
    } else if (cur->head->kind == I_OPER) {
      Temp_tempList def = cur->head->u.OPER.dst, use = cur->head->u.OPER.src;

      // spill use
      int ints = 9, floats = 14;
      AS_instrList tmpCur = cur;
      for (Temp_tempList tl = use; tl; tl = tl->tail) {
        Temp_temp temp = tl->head;
        int offset = RA_intMapLook(spillInfo.spillMap, temp);
        if (offset != -1) {
          AS_instr spillInstr;
          switch (temp->type) {
            case T_int: {
              if (offset == 0) {
                spillInstr =
                    AS_Oper(Stringf("\tldr r%d, [sp]", ints), NULL, NULL, NULL);
              } else {
                spillInstr = AS_Oper(
                    Stringf("\tldr r%d, [sp, #%d]", ints, offset * ARCH_SIZE),
                    NULL, NULL, NULL);
              }
              break;
            }
            case T_float: {
              if (offset == 0) {
                spillInstr = AS_Oper(Stringf("\tvldr s%d, [sp]", floats), NULL,
                                     NULL, NULL);
              } else {
                spillInstr = AS_Oper(Stringf("\tvldr s%d, [sp, #%d]", floats,
                                             offset * ARCH_SIZE),
                                     NULL, NULL, NULL);
              }
              break;
            }
            default:
              ASSERT(0, "Unknown temp type");
          }

          pre->tail = AS_InstrList(spillInstr, cur);
          cur = pre->tail;

          // enter color
          switch (temp->type) {
            case T_int: {
              tl->head = armReg2Temp(Stringf("r%d", ints++));
              break;
            }
            case T_float: {
              tl->head = armReg2Temp(Stringf("s%d", floats++));
              break;
            }
          }
        }
      }
      cur = tmpCur->tail;
      while (pre->tail != cur) {
        pre = pre->tail;
      }

      // spill def
      tmpCur = cur;
      for (Temp_tempList tl = def; tl; tl = tl->tail) {
        Temp_temp temp = tl->head;
        int offset = RA_intMapLook(spillInfo.spillMap, temp);
        if (offset != -1) {
          AS_instr spillInstr;
          switch (temp->type) {
            case T_int: {
              if (offset == 0) {
                spillInstr = AS_Oper("\tstr r9, [sp]", NULL, NULL, NULL);
              } else {
                spillInstr =
                    AS_Oper(Stringf("\tstr r9, [sp, #%d]", offset * ARCH_SIZE),
                            NULL, NULL, NULL);
              }
              break;
            }
            case T_float: {
              if (offset == 0) {
                spillInstr = AS_Oper("\tvstr s14, [sp]", NULL, NULL, NULL);
              } else {
                spillInstr = AS_Oper(
                    Stringf("\tvstr s14, [sp, #%d]", offset * ARCH_SIZE), NULL,
                    NULL, NULL);
              }
              break;
            }
            default:
              ASSERT(0, "Unknown temp type");
          }

          pre->tail = AS_InstrList(spillInstr, cur);
          cur = pre->tail;

          // enter color
          switch (temp->type) {
            case T_int: {
              tl->head = armReg2Temp("r9");
              break;
            }
            case T_float: {
              tl->head = armReg2Temp("s14");
              break;
            }
          }
        }
      }
      cur = tmpCur;
      while (pre->tail != cur) {
        pre = pre->tail;
      }
    } else if (cur->head->kind == I_MOVE) {
      ASSERT(cur->head->u.MOVE.src,
             Stringf("src is NULL, %s", cur->head->u.MOVE.assem));
      ASSERT(cur->head->u.MOVE.dst, "dst is NULL");
      ASSERT(!cur->head->u.MOVE.src->tail, "src is not a single temp");
      ASSERT(!cur->head->u.MOVE.dst->tail, "dst is not a single temp");

      Temp_tempList dst = cur->head->u.MOVE.dst;
      Temp_tempList src = cur->head->u.MOVE.src;

      int srcOffset = RA_intMapLook(spillInfo.spillMap, src->head);
      int dstOffset = RA_intMapLook(spillInfo.spillMap, dst->head);

      if (srcOffset == -1 && dstOffset == -1) {
        string srcReg = Temp_look(res->coloring, src->head);
        string dstReg = Temp_look(res->coloring, dst->head);
        if (!strcmp(srcReg, dstReg)) {
          // remove redundant move
          pre->tail = cur->tail;
          cur = pre->tail;
        } else {
          cur = cur->tail;
          pre = pre->tail;
        }
        continue;
      }

      AS_instrList tmpCur = cur;
      if (srcOffset != -1) {
        AS_instr spillInstr;
        switch (src->head->type) {
          case T_int: {
            if (srcOffset == 0) {
              spillInstr = AS_Oper(Stringf("\tldr r9, [sp]"), NULL, NULL, NULL);
            } else {
              spillInstr =
                  AS_Oper(Stringf("\tldr r9, [sp, #%d]", srcOffset * ARCH_SIZE),
                          NULL, NULL, NULL);
            }
            break;
          }
          case T_float: {
            if (srcOffset == 0) {
              spillInstr =
                  AS_Oper(Stringf("\tvldr s14, [sp]"), NULL, NULL, NULL);
            } else {
              spillInstr = AS_Oper(
                  Stringf("\tvldr s14, [sp, #%d]", srcOffset * ARCH_SIZE), NULL,
                  NULL, NULL);
            }
            break;
          }
          default:
            ASSERT(0, "Unknown temp type");
        }

        pre->tail = AS_InstrList(spillInstr, cur);
        cur = pre->tail;

        // enter color
        switch (src->head->type) {
          case T_int: {
            src->head = armReg2Temp("r9");
            break;
          }
          case T_float: {
            src->head = armReg2Temp("s14");
            break;
          }
        }
      }
      cur = tmpCur->tail;
      while (pre->tail != cur) {
        pre = pre->tail;
      }

      tmpCur = cur;
      if (dstOffset != -1) {
        AS_instr spillInstr;
        switch (dst->head->type) {
          case T_int: {
            if (dstOffset == 0) {
              spillInstr = AS_Oper(Stringf("\tstr r9, [sp]"), NULL, NULL, NULL);
            } else {
              spillInstr =
                  AS_Oper(Stringf("\tstr r9, [sp, #%d]", dstOffset * ARCH_SIZE),
                          NULL, NULL, NULL);
            }
            break;
          }
          case T_float: {
            if (dstOffset == 0) {
              spillInstr =
                  AS_Oper(Stringf("\tvstr s14, [sp]"), NULL, NULL, NULL);
            } else {
              spillInstr = AS_Oper(
                  Stringf("\tvstr s14, [sp, #%d]", dstOffset * ARCH_SIZE), NULL,
                  NULL, NULL);
            }
            break;
          }
          default:
            ASSERT(0, "Unknown temp type");
        }

        pre->tail = AS_InstrList(spillInstr, cur);
        cur = pre->tail;

        // enter color
        switch (dst->head->type) {
          case T_int: {
            dst->head = armReg2Temp("r9");
            break;
          }
          case T_float: {
            dst->head = armReg2Temp("s14");
            break;
          }
        }
      }
      cur = tmpCur;
      while (pre->tail != cur) {
        pre = pre->tail;
      }
    } else {
      cur = cur->tail;
      pre = pre->tail;
    }
  }
}

static void RA_rewriteProgram(RA_result res, COL_result colRes) {
#ifdef REGALLOC_DEBUG
  fprintf(stderr, "Start rewriting program\n");
#endif
  ASSERT(res, "res is NULL");
  ASSERT(colRes, "colRes is NULL");

  RA_spillInfo spillInfo = RA_computeStackSize(res, colRes);
  RA_realSpill(res, spillInfo);
}

RA_result RA_regAlloc(AS_instrList il, G_nodeList ig) {
  if (!il) return NULL;
  ASSERT(ig, "Interference graph is empty");

  COL_result colRes = COL_color(ig->head->mygraph);
  RA_result res = RA_Result(colRes->coloring, il);
  RA_rewriteProgram(res, colRes);
  return res;
}