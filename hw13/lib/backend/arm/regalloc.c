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

#define REGALLOC_DEBUG
#undef REGALLOC_DEBUG

#define ARMREGSTART 0
#define FLOATREGSTART 20

#define MAX_NUM_REG 8        // r0-r7, and r8~r10 are reserved for spill
#define MAX_NUM_FLOATREG 16  // s0-s15

typedef struct RA_tempInfo_ *RA_tempInfo;
struct RA_tempInfo_ {
  Temp_temp temp;
  G_node node;
  bool precolored;
  bool spilled;
  int degree;
  int color;
};

static RA_result RA_Result(Temp_map coloring, AS_instrList il) {
  RA_result res = checked_malloc(sizeof(*res));
  res->coloring = coloring;
  res->il = il;
  return res;
}

static void init_precolored_regs(Temp_map coloring) {
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
}

RA_result RA_regAlloc(AS_instrList il, G_nodeList ig_local) {
  if (!il) return NULL;
  // TODO regalloc
}