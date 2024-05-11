#include "ssa.h"

#include "assem.h"
#include "bitmap.h"
#include "graph.h"

#define SSA_DEBUG
// #undef SSA_DEBUG

typedef struct SSA_block_info_ *SSA_block_info;
typedef SSA_block_info *SSA_block_infoEnv;
struct SSA_block_info_ {
  G_node mynode;
};
static SSA_block_info SSA(G_node node) {
  SSA_block_info info = (SSA_block_info)checked_malloc(sizeof *info);
  info->mynode = node;
  return info;
}

static int num_bg_nodes;
static SSA_block_infoEnv blockInfoEnv;

typedef int *sortorder;
static sortorder bg_rpo;
static bool *marked;

static void SSA_init(G_nodeList lg, G_nodeList bg);
static void init_blockInfoEnv(G_nodeList bg);

static void sort_bg_in_RPO();
static void dfs_bg(int i);
static void print_bg_RPO(FILE *out);

static void SSA_init(G_nodeList lg, G_nodeList bg) {
  num_bg_nodes = bg->head->mygraph->nodecount;

  init_blockInfoEnv(bg);
}

static void init_blockInfoEnv(G_nodeList bg) {
  blockInfoEnv =
      (SSA_block_infoEnv)checked_malloc(num_bg_nodes * sizeof *blockInfoEnv);
  for (G_nodeList p = bg; p; p = p->tail) {
    blockInfoEnv[p->head->mykey] = SSA(p->head);
  }
}

static int dfs_N;
static void sort_bg_in_RPO() {
  // sort the nodes in bg in reverse post order
  bg_rpo = (sortorder)checked_malloc(num_bg_nodes * (sizeof *bg_rpo));
  marked = (bool *)checked_malloc(num_bg_nodes * (sizeof *marked));
  for (int i = 0; i < num_bg_nodes; ++i) {
    marked[i] = FALSE;
  }

  dfs_N = num_bg_nodes - 1;
  dfs_bg(0);
}

static void dfs_bg(int i) {
  // perform a depth first search on bg
  if (marked[i]) {
    return;
  }

  marked[i] = TRUE;
  for (G_nodeList l = G_succ(blockInfoEnv[i]->mynode); l; l = l->tail) {
    dfs_bg(l->head->mykey);
  }

  bg_rpo[i] = dfs_N--;
}

static void print_bg_RPO(FILE *out) {
  fprintf(out, "----------------- bg_rpo -----------------\n");
  for (int i = 0; i < num_bg_nodes; ++i) {
    fprintf(out, "%d: (rpo %d)\n", i, bg_rpo[i]);
  }
  fprintf(out, "\n");
}

static void compute_bg_doms() {
  // compute the dominators of each node in bg

  // step 1: sort in RPO
  sort_bg_in_RPO();
#ifdef SSA_DEBUG
  print_bg_RPO(stderr);
#endif

  // step 2: compute the dominators
}

AS_instrList AS_instrList_to_SSA(AS_instrList bodyil, G_nodeList lg,
                                 G_nodeList bg) {
  /* here is your implementation of translating to ssa */

  if (!lg || !bg) {
    return bodyil;
  }

  SSA_init(lg, bg);

  compute_bg_doms();

  return bodyil;
}
