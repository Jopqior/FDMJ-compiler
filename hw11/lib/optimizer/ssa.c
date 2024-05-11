#include "ssa.h"

#include "assem.h"
#include "bitmap.h"
#include "graph.h"

#define SSA_DEBUG
// #undef SSA_DEBUG

#define DOM_DEBUG
#undef DOM_DEBUG

#define DOM_TREE_DEBUG
#undef DOM_TREE_DEBUG

#define DOM_FRONTIER_DEBUG
#undef DOM_FRONTIER_DEBUG

typedef struct mylist_ *mylist;
struct mylist_ {
  int blockid;
  mylist next;
};
static mylist Mylist(int data, mylist next) {
  mylist p = (mylist)checked_malloc(sizeof *p);
  p->blockid = data;
  p->next = next;
  return p;
}
static mylist Mylist_Splice(mylist a, mylist b) {
  if (!a) {
    return b;
  }
  if (!b) {
    return a;
  }
  mylist p = a;
  while (p->next) {
    p = p->next;
  }
  p->next = b;
  return a;
}

typedef struct SSA_block_info_ *SSA_block_info;
struct SSA_block_info_ {
  G_node mynode;
  int idom;
  mylist dom_tree_children;
  mylist dom_frontiers;
};
static SSA_block_info SSA_block_info_init(G_node node) {
  SSA_block_info info = (SSA_block_info)checked_malloc(sizeof *info);
  info->mynode = node;
  info->idom = -1;
  info->dom_tree_children = NULL;
  info->dom_frontiers = NULL;
  return info;
}

static int num_bg_nodes;
static SSA_block_info *blockInfoEnv;

static int *bg_rpo;
static bitmap *bg_doms;

static void SSA_init(G_nodeList lg, G_nodeList bg);
static void init_blockInfoEnv(G_nodeList bg);

static void sort_bg_in_RPO();
static void dfs_bg(int i);
static void print_bg_RPO(FILE *out);

static void compute_bg_doms(FILE *out);
static void print_bg_doms(FILE *out, int num_iters);

static void compute_bg_dom_tree(FILE *out);
static void construct_bg_dom_tree();
static void print_bg_idoms(FILE *out);
static void print_bg_dom_tree(FILE *out);

static void compute_bg_dom_frontiers(FILE *out);
static void compute_bg_df_recur(FILE *out, int u);
static void print_bg_dom_frontiers(FILE *out);

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

static void compute_bg_doms(FILE *out) {
  // compute the dominators of each node in bg

  // step 1: sort in RPO
  sort_bg_in_RPO();
#ifdef SSA_DEBUG
  print_bg_RPO(out);
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
  print_bg_doms(out, num_iters);
#endif
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

static void compute_bg_dom_tree(FILE *out) {
  // compute the dominator tree of bg

  // step 1: compute the dominators
  compute_bg_doms(out);

  // step 2: compute the dominator tree
  blockInfoEnv[0]->idom = -1;

  bitmap u_mask = Bitmap(num_bg_nodes);
  for (int u = 1; u < num_bg_nodes; ++u) {
    bitmap_clear_all(u_mask);
    bitmap_set(u_mask, u);
    for (int v = 0; v < num_bg_nodes; ++v) {
      // idom[u] != u
      // idom[u] dominates u
      if (v == u || !bitmap_read(bg_doms[u], v)) {
        continue;
      }
      // idom[u] is the closest dominator of u
      bitmap flag = bitmap_difference(bg_doms[u], bg_doms[v]);
      if (bitmap_equal(flag, u_mask)) {
#ifdef DOM_TREE_DEBUG
        fprintf(stderr, "set idom, u=%d, v=%d\n", u, v);
#endif
        blockInfoEnv[u]->idom = v;
        break;
      }
    }
  }
#ifdef SSA_DEBUG
  fprintf(out, "----------------- bg_idoms -----------------\n");
  print_bg_idoms(out);
#endif

  construct_bg_dom_tree();
#ifdef SSA_DEBUG
  fprintf(out, "----------------- bg_dom_tree -----------------\n");
  print_bg_dom_tree(out);
#endif
}

static void construct_bg_dom_tree() {
  for (int i = 1; i < num_bg_nodes; ++i) {
    int idom = blockInfoEnv[i]->idom;
    if (idom == -1) {
      continue;
    }

    blockInfoEnv[idom]->dom_tree_children =
        Mylist_Splice(blockInfoEnv[idom]->dom_tree_children, Mylist(i, NULL));
  }
}

static void print_bg_idoms(FILE *out) {
  for (int i = 0; i < num_bg_nodes; ++i) {
    fprintf(out, "%d: (idom %d)\n", i, blockInfoEnv[i]->idom);
  }
  fprintf(out, "\n");
}

static void print_bg_dom_tree(FILE *out) {
  for (int i = 0; i < num_bg_nodes; ++i) {
    fprintf(out, "%d: ", i);
    for (mylist p = blockInfoEnv[i]->dom_tree_children; p; p = p->next) {
      fprintf(out, "%d ", p->blockid);
    }
    fprintf(out, "\n");
  }
  fprintf(out, "\n");
}

static void compute_bg_dom_frontiers(FILE *out) {
  // compute the dominator frontiers of each node in bg

  // step 1: compute the dominator tree
  compute_bg_dom_tree(out);

  // step 2: compute the dominator frontiers
  compute_bg_df_recur(out, 0);

#ifdef SSA_DEBUG
  fprintf(out, "----------------- bg_dom_frontiers -----------------\n");
  print_bg_dom_frontiers(out);
#endif
}

static void compute_bg_df_recur(FILE *out, int u) {
  bitmap tmp = Bitmap(num_bg_nodes);

  // compute DF_local[u]
  for (G_nodeList s = G_succ(blockInfoEnv[u]->mynode); s; s = s->tail) {
    int v = s->head->mykey;
    if (blockInfoEnv[v]->idom != u) {
      bitmap_set(tmp, v);
    }
  }

  // compute DF_up[u]
  for (mylist p = blockInfoEnv[u]->dom_tree_children; p; p = p->next) {
    int w = p->blockid;
    compute_bg_df_recur(out, w);
    for (mylist q = blockInfoEnv[w]->dom_frontiers; q; q = q->next) {
      int x = q->blockid;
      if (!bitmap_read(bg_doms[x], u) || x == u) {
        bitmap_set(tmp, x);
      }
    }
  }

  // store DF[u]
  for (int i = 0; i < num_bg_nodes; ++i) {
    if (bitmap_read(tmp, i)) {
      blockInfoEnv[u]->dom_frontiers =
          Mylist_Splice(blockInfoEnv[u]->dom_frontiers, Mylist(i, NULL));
    }
  }
}

static void print_bg_dom_frontiers(FILE *out) {
  for (int i = 0; i < num_bg_nodes; ++i) {
    fprintf(out, "%d: ", i);
    for (mylist p = blockInfoEnv[i]->dom_frontiers; p; p = p->next) {
      fprintf(out, "%d ", p->blockid);
    }
    fprintf(out, "\n");
  }
  fprintf(out, "\n");
}

AS_instrList AS_instrList_to_SSA(AS_instrList bodyil, G_nodeList lg,
                                 G_nodeList bg) {
  /* here is your implementation of translating to ssa */

  if (!lg || !bg) {
    return bodyil;
  }

  SSA_init(lg, bg);

  compute_bg_dom_frontiers(stderr);

  return bodyil;
}
