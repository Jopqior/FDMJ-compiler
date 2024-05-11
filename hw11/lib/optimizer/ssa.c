#include "ssa.h"

#include "assem.h"
#include "bitmap.h"
#include "graph.h"

#define SSA_DEBUG
// #undef SSA_DEBUG

#define DOM_DEBUG
#undef DOM_DEBUG

typedef struct SSA_block_info_ *SSA_block_info;
struct SSA_block_info_ {
  G_node mynode;
  int idom;
};
static SSA_block_info SSA_block_info_init(G_node node) {
  SSA_block_info info = (SSA_block_info)checked_malloc(sizeof *info);
  info->mynode = node;
  return info;
}

static int num_bg_nodes;
static SSA_block_info *blockInfoEnv;

static int *bg_rpo;
static bitmap *bg_doms;

static void SSA_init(G_nodeList lg, G_nodeList bg);
static void init_blockInfoEnv(G_nodeList bg);

static void compute_bg_doms();
static void sort_bg_in_RPO();
static void dfs_bg(int i);
static void print_bg_RPO(FILE *out);
static void print_bg_doms(FILE *out, int num_iters);

static void SSA_init(G_nodeList lg, G_nodeList bg) {
  num_bg_nodes = bg->head->mygraph->nodecount;

  init_blockInfoEnv(bg);
}

static void init_blockInfoEnv(G_nodeList bg) {
  blockInfoEnv =
      (SSA_block_info *)checked_malloc(num_bg_nodes * sizeof *blockInfoEnv);
  for (G_nodeList p = bg; p; p = p->tail) {
    blockInfoEnv[p->head->mykey] = SSA_block_info_init(p->head);
  }
}

static bool *marked;
static int dfs_N;
static void sort_bg_in_RPO() {
  // sort the nodes in bg in reverse post order
  bg_rpo = (int *)checked_malloc(num_bg_nodes * (sizeof *bg_rpo));
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

static void print_bg_doms(FILE *out, int num_iters) {
  fprintf(out, "----------------- bg_doms -----------------\n");
  fprintf(out, "Number of iterations=%d\n", num_iters);
  for (int i = 0; i < num_bg_nodes; ++i) {
    fprintf(out, "%d: ", i);
    bitmap_print(out, bg_doms[i]);
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
  bg_doms = (bitmap *)checked_malloc(num_bg_nodes * (sizeof *bg_doms));
  bg_doms[0] = Bitmap(num_bg_nodes);
  bitmap_set(bg_doms[0], 0);
  for (int i = 1; i < num_bg_nodes; ++i) {
    bg_doms[i] = Bitmap(num_bg_nodes);
    bitmap_set_all(bg_doms[i]);
  }

  bool changed = TRUE;
  int num_iters = 0;
  bitmap tmp = Bitmap(num_bg_nodes);
  while (changed) {
    changed = FALSE;
    num_iters++;
    for (int i = 0; i < num_bg_nodes; ++i) {
      int u = bg_rpo[i];
      if (u == 0) {
        continue;
      }

      bitmap_set_all(tmp);

      // D[u] = {u} U (intersection of D[p] for all p in preds[u])
      for (G_nodeList p = G_pred(blockInfoEnv[u]->mynode); p; p = p->tail) {
        int v = p->head->mykey;
#ifdef DOM_DEBUG
        fprintf(stderr, "u: %d, v: %d, v.doms: ", u, v);
        bitmap_print(stderr, bg_doms[v]);
#endif
        bitmap_intersection_into(tmp, bg_doms[v]);
      }
      bitmap_set(tmp, u);

      if (!bitmap_equal(tmp, bg_doms[u])) {
#ifdef DOM_DEBUG
        fprintf(stderr, "changed: %d\n", u);
        fprintf(stderr, "old: ");
        bitmap_print(stderr, bg_doms[u]);
        fprintf(stderr, "new: ");
        bitmap_print(stderr, tmp);
        fprintf(stderr, "\n");
#endif
        changed = TRUE;
        bitmap_copy(bg_doms[u], tmp);
      }
    }
  }
#ifdef SSA_DEBUG
  print_bg_doms(stderr, num_iters);
#endif
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
