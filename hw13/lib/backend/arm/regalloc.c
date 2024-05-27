#include "regalloc.h"

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

#define ARMREGSTART 0
#define FLOATREGSTART 20

#define MAX_NUM_REG 9        // r0-r7, and r8~r10 are reserved for spill
#define MAX_NUM_FLOATREG 16  // s0-s15

typedef struct COL_tempInfo_ *COL_tempInfo;
struct COL_tempInfo_ {
  G_node node;
  int degree;
};

typedef struct COL_result_ *COL_result;
struct COL_result_ {
  Temp_map coloring;
  Temp_tempList spills;
};

static TAB_table colEnv = NULL;
static G_nodeList simplyfyWorklist = NULL;
static G_nodeList spillWorklist = NULL;
static G_nodeList selectStack = NULL;

static bool Temp_isPrecolored(Temp_temp temp) { return temp->num < 99; }

static COL_tempInfo COL_TempInfo(G_node node, int degree) {
  COL_tempInfo info = checked_malloc(sizeof(*info));
  info->node = node;
  info->degree = degree;
  return info;
}

static COL_result COL_Result(Temp_map coloring, Temp_tempList spills) {
  COL_result res = checked_malloc(sizeof(*res));
  res->coloring = coloring;
  res->spills = spills;
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
    reg = Temp_namedtemp(i + ARMREGSTART, T_int);
    Temp_enter(coloring, reg, Stringf("r%d", i));
  }
  reg = Temp_namedtemp(14 + ARMREGSTART, T_int);
  Temp_enter(coloring, reg, "lr");

  for (int i = 0; i <= 15; ++i) {
    reg = Temp_namedtemp(i + FLOATREGSTART, T_float);
    Temp_enter(coloring, reg, Stringf("s%d", i));
  }
#ifdef COLOR_DEBUG
  fprintf(stderr, "Finish init precolor\n");
#endif
}

static void COL_buildWorklist(G_graph ig) {
#ifdef COLOR_DEBUG
  fprintf(stderr, "Start building worklist\n");
#endif
  if (!colEnv) {
    colEnv = TAB_empty();
  }

  for (G_nodeList nl = G_nodes(ig); nl; nl = nl->tail) {
    G_node n = nl->head;
    Temp_temp temp = G_nodeInfo(n);
    if (Temp_isPrecolored(temp)) {
      continue;
    }
    COL_tempInfo info = COL_TempInfo(n, G_degree(n));
    TAB_enter(colEnv, temp, info);

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

static COL_result COL_color(G_graph ig) {
#ifdef COLOR_DEBUG
  fprintf(stderr, "Start coloring\n");
#endif
  COL_result res = COL_Result(Temp_empty(), NULL);
  COL_initPreColor(res->coloring);
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

  int nodeCnt = ig->nodecount;
  while (simplyfyWorklist || spillWorklist) {
    if (simplyfyWorklist) {
      COL_simplify();
    } else if (spillWorklist) {
      COL_spill(res);
    }
  }

#ifdef COLOR_DEBUG
  fprintf(stderr, "Finish coloring\n");
#endif
  return res;
}

RA_result RA_regAlloc(AS_instrList il, G_nodeList ig) {
  if (!il) return NULL;
  ASSERT(ig, "Interference graph is empty");

  COL_result res = COL_color(ig->head->mygraph);
  return RA_Result(res->coloring, il);
}