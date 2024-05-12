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

static FILE *out;

typedef struct blockIdList_ *blockIdList;
struct blockIdList_ {
  int blockid;
  blockIdList next;
};
static blockIdList BlockIdList(int data, blockIdList next) {
  blockIdList p = (blockIdList)checked_malloc(sizeof *p);
  p->blockid = data;
  p->next = next;
  return p;
}
static blockIdList BlockIdlist_Splice(blockIdList a, blockIdList b) {
  if (!a) {
    return b;
  }
  if (!b) {
    return a;
  }
  blockIdList p = a;
  while (p->next) {
    p = p->next;
  }
  p->next = b;
  return a;
}

typedef struct SSA_block_info_ *SSA_block_info;
struct SSA_block_info_ {
  G_node mynode;
  int rpo;
  bitmap doms;
  int idom;
  blockIdList dom_tree_children;
  blockIdList dom_frontiers;
  Temp_tempList orig_vars;
  Temp_tempList phi_vars;
};
static SSA_block_info SSA_block_info_init(G_node node, int num_bg_nodes) {
  SSA_block_info info = (SSA_block_info)checked_malloc(sizeof *info);
  info->mynode = node;
  info->rpo = -1;
  info->doms = Bitmap(num_bg_nodes);
  info->idom = -1;
  info->dom_tree_children = NULL;
  info->dom_frontiers = NULL;
  info->orig_vars = NULL;
  info->phi_vars = NULL;
  return info;
}

typedef struct SSA_var_info_ *SSA_var_info;
struct SSA_var_info_ {
  bitmap defsites;
};
static SSA_var_info SSA_var_info_init(int num_bg_nodes) {
  SSA_var_info info = (SSA_var_info)checked_malloc(sizeof *info);
  info->defsites = Bitmap(num_bg_nodes);
  return info;
}

static int num_bg_nodes;
static SSA_block_info *blockInfoEnv;
static TAB_table varInfoEnv;

static void SSA_init(G_nodeList lg, G_nodeList bg);
static void init_blockInfoEnv(G_nodeList bg);
static void init_blockOrigVars(G_nodeList lg);
static void print_blockOrigVars(FILE *out);
static void init_varInfoEnv();

static void sort_bg_in_RPO();
static void dfs_bg(int i);
static void print_bg_RPO(FILE *out);

static void compute_bg_doms();
static void print_bg_doms(FILE *out, int num_iters);

static void compute_bg_dom_tree();
static void construct_bg_dom_tree();
static void print_bg_idoms(FILE *out);
static void print_bg_dom_tree(FILE *out);

static void compute_bg_dom_frontiers();
static void compute_bg_df_recur(int u);
static void print_bg_dom_frontiers(FILE *out);

static void compute_phi_functions(G_nodeList lg, G_nodeList bg);
static void print_var_defsites(FILE *out);
static void print_bg_phi_functions(FILE *out);

static void SSA_init(G_nodeList lg, G_nodeList bg) {
  num_bg_nodes = bg->head->mygraph->nodecount;

  init_blockInfoEnv(bg);
  init_blockOrigVars(lg);
  init_varInfoEnv();
}

static void init_blockInfoEnv(G_nodeList bg) {
  blockInfoEnv =
      (SSA_block_info *)checked_malloc(num_bg_nodes * sizeof *blockInfoEnv);
  for (G_nodeList p = bg; p; p = p->tail) {
    blockInfoEnv[p->head->mykey] = SSA_block_info_init(p->head, num_bg_nodes);
  }
}

static void init_blockOrigVars(G_nodeList lg) {
  G_nodeList p = lg;
  for (int i = 0; i < num_bg_nodes; ++i) {
    AS_block block = G_nodeInfo(blockInfoEnv[i]->mynode);
    for (AS_instrList q = block->instrs; q; q = q->tail, p = p->tail) {
      if (q->head != G_nodeInfo(p->head)) {
        fprintf(stderr, "Error: lg and bg are not in sync\n");
        fprintf(stderr, "lg: ");
        FG_Showinfo(stderr, G_nodeInfo(p->head), Temp_name());
        fprintf(stderr, "\n");
        fprintf(stderr, "bg: ");
        FG_Showinfo(stderr, q->head, Temp_name());
        fprintf(stderr, "\n");
        exit(1);
      }

      Temp_tempList tl = FG_def(p->head);
      if (!tl) {
        continue;
      }
      if (!blockInfoEnv[i]->orig_vars) {
        blockInfoEnv[i]->orig_vars = tl;
      } else {
        blockInfoEnv[i]->orig_vars =
            Temp_TempListUnion(blockInfoEnv[i]->orig_vars, tl);
      }
    }
  }
}

static void print_blockOrigVars(FILE *out) {
  for (int i = 0; i < num_bg_nodes; ++i) {
    fprintf(out, "%d: ", i);
    for (Temp_tempList p = blockInfoEnv[i]->orig_vars; p; p = p->tail) {
      fprintf(out, "%d ", p->head->num);
    }
    fprintf(out, "\n");
  }
  fprintf(out, "\n");
}

static void init_varInfoEnv() { varInfoEnv = TAB_empty(); }

static bool *marked;
static int dfs_N;
static void sort_bg_in_RPO() {
  // sort the nodes in bg in reverse post order
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

  blockInfoEnv[i]->rpo = dfs_N--;
}

static void print_bg_RPO(FILE *out) {
  fprintf(out, "----------------- bg_rpo -----------------\n");
  for (int i = 0; i < num_bg_nodes; ++i) {
    fprintf(out, "%d: (rpo %d)\n", i, blockInfoEnv[i]->rpo);
  }
  fprintf(out, "\n");
}

static void compute_bg_doms() {
  // compute the dominators of each node in bg

  // step 1: sort in RPO
  sort_bg_in_RPO();
#ifdef SSA_DEBUG
  print_bg_RPO(out);
#endif

  // step 2: compute the dominators
  bitmap_set(blockInfoEnv[0]->doms, 0);
  for (int i = 1; i < num_bg_nodes; ++i) {
    bitmap_set_all(blockInfoEnv[i]->doms);
  }

  bool changed = TRUE;
  int num_iters = 0;
  bitmap tmp = Bitmap(num_bg_nodes);
  while (changed) {
    changed = FALSE;
    num_iters++;
    for (int i = 0; i < num_bg_nodes; ++i) {
      int u = blockInfoEnv[i]->rpo;
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
        bitmap_intersection_into(tmp, blockInfoEnv[v]->doms);
      }
      bitmap_set(tmp, u);

      if (!bitmap_equal(tmp, blockInfoEnv[u]->doms)) {
#ifdef DOM_DEBUG
        fprintf(stderr, "changed: %d\n", u);
        fprintf(stderr, "old: ");
        bitmap_print(stderr, bg_doms[u]);
        fprintf(stderr, "new: ");
        bitmap_print(stderr, tmp);
        fprintf(stderr, "\n");
#endif
        changed = TRUE;
        bitmap_copy(blockInfoEnv[u]->doms, tmp);
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
    bitmap_print(out, blockInfoEnv[i]->doms);
  }
  fprintf(out, "\n");
}

static void compute_bg_dom_tree() {
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
      if (v == u || !bitmap_read(blockInfoEnv[u]->doms, v)) {
        continue;
      }
      // idom[u] is the closest dominator of u
      bitmap flag =
          bitmap_difference(blockInfoEnv[u]->doms, blockInfoEnv[v]->doms);
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

    blockInfoEnv[idom]->dom_tree_children = BlockIdlist_Splice(
        blockInfoEnv[idom]->dom_tree_children, BlockIdList(i, NULL));
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
    for (blockIdList p = blockInfoEnv[i]->dom_tree_children; p; p = p->next) {
      fprintf(out, "%d ", p->blockid);
    }
    fprintf(out, "\n");
  }
  fprintf(out, "\n");
}

static void compute_bg_dom_frontiers() {
  // compute the dominator frontiers of each node in bg

  // step 1: compute the dominator tree
  compute_bg_dom_tree(out);

  // step 2: compute the dominator frontiers
  compute_bg_df_recur(0);

#ifdef SSA_DEBUG
  fprintf(out, "----------------- bg_dom_frontiers -----------------\n");
  print_bg_dom_frontiers(out);
#endif
}

static void compute_bg_df_recur(int u) {
  bitmap tmp = Bitmap(num_bg_nodes);

  // compute DF_local[u]
  for (G_nodeList s = G_succ(blockInfoEnv[u]->mynode); s; s = s->tail) {
    int v = s->head->mykey;
    if (blockInfoEnv[v]->idom != u) {
      bitmap_set(tmp, v);
    }
  }

  // compute DF_up[u]
  for (blockIdList p = blockInfoEnv[u]->dom_tree_children; p; p = p->next) {
    int w = p->blockid;
    compute_bg_df_recur(w);
    for (blockIdList q = blockInfoEnv[w]->dom_frontiers; q; q = q->next) {
      int x = q->blockid;
      if (!bitmap_read(blockInfoEnv[x]->doms, u) || x == u) {
        bitmap_set(tmp, x);
      }
    }
  }

  // store DF[u]
  for (int i = 0; i < num_bg_nodes; ++i) {
    if (bitmap_read(tmp, i)) {
      blockInfoEnv[u]->dom_frontiers = BlockIdlist_Splice(
          blockInfoEnv[u]->dom_frontiers, BlockIdList(i, NULL));
    }
  }
}

static void print_bg_dom_frontiers(FILE *out) {
  for (int i = 0; i < num_bg_nodes; ++i) {
    fprintf(out, "%d: ", i);
    for (blockIdList p = blockInfoEnv[i]->dom_frontiers; p; p = p->next) {
      fprintf(out, "%d ", p->blockid);
    }
    fprintf(out, "\n");
  }
  fprintf(out, "\n");
}

static void show_SSA_var_info(Temp_temp key, SSA_var_info value) {
  fprintf(stderr, "var: %d, defsites: ", key->num);
  bitmap_print(stderr, value->defsites);
}

static void compute_phi_functions(G_nodeList lg, G_nodeList bg) {
  // place phi functions in the program

  // step 1: record defsites of each variable
  G_nodeList p = lg;
  for (int i = 0; i < num_bg_nodes; ++i) {
    for (Temp_tempList tl = blockInfoEnv[i]->orig_vars; tl; tl = tl->tail) {
      Temp_temp var = tl->head;
      SSA_var_info info = (SSA_var_info)TAB_look(varInfoEnv, (void *)var);
      if (!info) {
        info = SSA_var_info_init(num_bg_nodes);
        TAB_enter(varInfoEnv, (void *)var, (void *)info);
      }

      bitmap_set(info->defsites, i);
    }
  }

  // step2: place phi func
  bitmap w = Bitmap(num_bg_nodes);
  Temp_temp top = varInfoEnv->top;
  binder b;
  while (top) {
    b = TAB_getBinder(varInfoEnv, (void *)top);
    Temp_temp var = (Temp_temp)b->key;
    SSA_var_info info = (SSA_var_info)b->value;

    // w = defsites(var)
    bitmap_copy(w, info->defsites);

    while (!bitmap_empty(w)) {
      // remove a node u from w
      int u = bitmap_get_first(w);
      bitmap_clear(w, u);

      // for each node v in DF[u]
      for (blockIdList p = blockInfoEnv[u]->dom_frontiers; p; p = p->next) {
        int v = p->blockid;
        // if var not in phi_vars(v), then place phi function for var
        if (!Temp_TempInTempList(var, blockInfoEnv[v]->phi_vars)) {
          blockInfoEnv[v]->phi_vars =
              Temp_TempList(var, blockInfoEnv[v]->phi_vars);
          // need to reconsider v
          if (!Temp_TempInTempList(var, blockInfoEnv[u]->orig_vars)) {
            bitmap_set(w, v);
          }
        }
      }
    }

    top = (Temp_temp)b->prevtop;
  }
#ifdef SSA_DEBUG
  fprintf(out, "----------------- bg_phi_functions -----------------\n");
  print_bg_phi_functions(out);
#endif
}

static void print_var_defsites(FILE *out) {
  TAB_dump(varInfoEnv, (void (*)(void *, void *))show_SSA_var_info);
  fprintf(out, "\n");
}

static void print_bg_phi_functions(FILE *out) {
  for (int i = 0; i < num_bg_nodes; ++i) {
    fprintf(out, "%d: ", i);
    for (Temp_tempList p = blockInfoEnv[i]->phi_vars; p; p = p->tail) {
      fprintf(out, "%d ", p->head->num);
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
  out = stderr;

  SSA_init(lg, bg);

  compute_bg_dom_frontiers();
  compute_phi_functions(lg, bg);

  return bodyil;
}
