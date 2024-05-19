#include "ssa.h"

#include <string.h>

#include "assem.h"
#include "bitmap.h"
#include "graph.h"

#define SSA_DEBUG
#undef SSA_DEBUG

#define SSA_DEC_DEBUG
#undef SSA_DEC_DEBUG

#define LT_DEBUG
#undef LT_DEBUG

#define DOM_DEBUG
#undef DOM_DEBUG

#define DOM_TREE_DEBUG
#undef DOM_TREE_DEBUG

#define DOM_FRONTIER_DEBUG
#undef DOM_FRONTIER_DEBUG

#define RENAME_DEBUG
#undef RENAME_DEBUG

bool Lengauer_Tarjan_Enable = TRUE;
static FILE *out;

typedef struct blockIdList_ *blockIdList;
struct blockIdList_ {
  int blockid;
  blockIdList tail;
};
static blockIdList BlockIdList(int data, blockIdList next) {
  blockIdList p = (blockIdList)checked_malloc(sizeof *p);
  p->blockid = data;
  p->tail = next;
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
  while (p->tail) {
    p = p->tail;
  }
  p->tail = b;
  return a;
}

typedef struct var_stack_ *var_stack;
struct var_stack_ {
  Temp_temp head;
  var_stack tail;
};
static void print_var_stack(FILE *out, var_stack stack) {
  for (var_stack p = stack; p; p = p->tail) {
    fprintf(out, "%d ", p->head->num);
  }
  fprintf(out, "\n");
}

typedef struct instrInfoList_ *instrInfoList;
struct instrInfoList_ {
  AS_instr instr;
  Temp_tempList origin_def;
  instrInfoList tail;
};
static instrInfoList InstrInfoList(AS_instr instr, Temp_tempList tl,
                                   instrInfoList tail) {
  instrInfoList il = checked_malloc(sizeof *il);
  il->instr = instr;
  il->origin_def = tl;
  il->tail = tail;
  return il;
}
static instrInfoList InstrInfoList_Splice(instrInfoList a, instrInfoList b) {
  if (!a) {
    return b;
  }
  if (!b) {
    return a;
  }
  instrInfoList p = a;
  while (p->tail) {
    p = p->tail;
  }
  p->tail = b;
  return a;
}
static instrInfoList Insert_phi_func(instrInfoList a, AS_instr instr,
                                     Temp_tempList tl) {
  instrInfoList firstInstr = a->tail;
  instrInfoList newInstrs = InstrInfoList(instr, tl, firstInstr);
  a->tail = newInstrs;
  return a;
}

typedef struct SSA_block_info_ *SSA_block_info;
struct SSA_block_info_ {
  G_node mynode;
  bitmap doms;
  int idom;
  blockIdList dom_tree_children;
  blockIdList dom_frontiers;
  Temp_tempList orig_vars;
  Temp_tempList phi_vars;
  instrInfoList instrInfos;
  Temp_tempList blockIn;
  Temp_tempList blockOut;
};
static SSA_block_info SSA_block_info_init(G_node node, int num_bg_nodes) {
  SSA_block_info info = (SSA_block_info)checked_malloc(sizeof *info);
  info->mynode = node;
  info->doms = Bitmap(num_bg_nodes);
  info->idom = -1;
  info->dom_tree_children = NULL;
  info->dom_frontiers = NULL;
  info->orig_vars = NULL;
  info->phi_vars = NULL;
  info->instrInfos = NULL;
  info->blockIn = NULL;
  info->blockOut = NULL;
  return info;
}

typedef struct SSA_var_info_ *SSA_var_info;
struct SSA_var_info_ {
  bitmap defsites;
  var_stack stack;
};
static SSA_var_info SSA_var_info_init(int num_bg_nodes) {
  SSA_var_info info = (SSA_var_info)checked_malloc(sizeof *info);
  info->defsites = Bitmap(num_bg_nodes);
  info->stack = NULL;
  return info;
}
static void Var_Stack_push(SSA_var_info info, Temp_temp t) {
  if (!info->stack) {
    info->stack = (var_stack)checked_malloc(sizeof *info->stack);
    info->stack->head = Temp_namedtemp(t->num, t->type);
    info->stack->tail = NULL;
    return;
  }
  var_stack p = (var_stack)checked_malloc(sizeof *p);
  p->head = Temp_namedtemp(t->num, t->type);
  p->tail = info->stack;
  info->stack = p;
}
static void Var_Stack_pop(SSA_var_info info) {
  if (!info->stack) {
    fprintf(stderr, "Error: stack is empty\n");
    exit(1);
  }
  var_stack p = info->stack;
  info->stack = p->tail;
  p = NULL;
}
static Temp_temp Var_Stack_top(SSA_var_info info) {
  if (!info->stack) {
    fprintf(stderr, "Error: stack is empty\n");
    exit(1);
  }
  return info->stack->head;
}

static int num_bg_nodes;
static int *bg_RPO;
static SSA_block_info *blockInfoEnv;
static TAB_table varInfoEnv;

static int compute_doms_iter = 0;

static void SSA_init(G_nodeList lg, G_nodeList bg);
static void init_bg_RPO();
static void init_blockInfoEnv(G_nodeList bg);
static void init_blockInfoAbountLg(G_nodeList lg);
static void print_blockOrigVars(FILE *out);
static void print_blockInstrInfos(FILE *out);
static void init_varInfoEnv(G_nodeList lg);

static void sort_bg_in_RPO();
static void dfs_bg(int i);
static void print_bg_RPO(FILE *out);

static void compute_bg_doms();
static void print_bg_doms(FILE *out, int num_iters);

static void compute_bg_idoms();
static void construct_bg_dom_tree();
static void print_bg_idoms(FILE *out);
static void print_bg_dom_tree(FILE *out);

static void compute_bg_dom_frontiers();
static void compute_bg_df_recur(int u);
static void print_bg_dom_frontiers(FILE *out);

static void compute_phi_functions(G_nodeList lg);
static void place_phi_func(Temp_temp var, int v);
static void print_var_defsites(FILE *out);
static void print_bg_phi_functions(FILE *out);

static void rename_vars();
static void rename_vars_recur(int u);

static AS_instrList get_final_result();

static void SSA_init(G_nodeList lg, G_nodeList bg) {
  num_bg_nodes = bg->head->mygraph->nodecount;

  init_bg_RPO();
  init_blockInfoEnv(bg);
  init_blockInfoAbountLg(lg);
  init_varInfoEnv(lg);
}

static void init_bg_RPO() {
  bg_RPO = (int *)checked_malloc(num_bg_nodes * sizeof *bg_RPO);
  for (int i = 0; i < num_bg_nodes; ++i) {
    bg_RPO[i] = -1;
  }
}

static void init_blockInfoEnv(G_nodeList bg) {
  blockInfoEnv =
      (SSA_block_info *)checked_malloc(num_bg_nodes * sizeof *blockInfoEnv);
  for (G_nodeList p = bg; p; p = p->tail) {
    blockInfoEnv[p->head->mykey] = SSA_block_info_init(p->head, num_bg_nodes);
  }
}

static void init_blockInfoAbountLg(G_nodeList lg) {
  G_nodeList p = lg;
  for (int i = 0; i < num_bg_nodes; ++i) {
    blockInfoEnv[i]->blockIn = FG_In(p->head);
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

      // record the block LiveOut
      if (q->tail) {
        blockInfoEnv[i]->blockOut = FG_Out(p->head);
      }

      Temp_tempList tl = FG_def(p->head);
      // record the instrs
      blockInfoEnv[i]->instrInfos = InstrInfoList_Splice(
          blockInfoEnv[i]->instrInfos, InstrInfoList(q->head, tl, NULL));

      if (!tl) {
        continue;
      }
      // record the origs
      if (!blockInfoEnv[i]->orig_vars) {
        blockInfoEnv[i]->orig_vars = tl;
      } else {
        blockInfoEnv[i]->orig_vars =
            Temp_TempListUnion(blockInfoEnv[i]->orig_vars, tl);
      }
    }
  }
#ifdef SSA_DEBUG
  // fprintf(out, "----------------- blockInstrInfos -----------------\n");
  // print_blockInstrInfos(out);
#endif
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

static void print_blockInstrInfos(FILE *out) {
  for (int i = 0; i < num_bg_nodes; ++i) {
    fprintf(out, "%d: \n", i);
    for (instrInfoList p = blockInfoEnv[i]->instrInfos; p; p = p->tail) {
      switch (p->instr->kind) {
        case I_OPER: {
          fprintf(out, "[%s] ", p->instr->u.OPER.assem);
          break;
        }
        case I_LABEL: {
          fprintf(out, "[%s] ", p->instr->u.LABEL.assem);
          break;
        }
        case I_MOVE: {
          fprintf(out, "[%s] ", p->instr->u.MOVE.assem);
          break;
        }
        default:
          break;
      }
      for (Temp_tempList q = p->origin_def; q; q = q->tail) {
        fprintf(out, "%d ", q->head->num);
      }
      fprintf(out, "\n");
    }
    fprintf(out, "\n");
  }
}

static void init_varInfoEnv(G_nodeList lg) {
  varInfoEnv = TAB_empty();
  for (G_nodeList p = lg; p; p = p->tail) {
    for (Temp_tempList q = FG_def(p->head); q; q = q->tail) {
      if (TAB_look(varInfoEnv, (void *)q->head)) {
        continue;
      }
      SSA_var_info info = SSA_var_info_init(num_bg_nodes);
      TAB_enter(varInfoEnv, (void *)q->head, (void *)info);
    }
    for (Temp_tempList q = FG_use(p->head); q; q = q->tail) {
      if (TAB_look(varInfoEnv, (void *)q->head)) {
        continue;
      }
      SSA_var_info info = SSA_var_info_init(num_bg_nodes);
      TAB_enter(varInfoEnv, (void *)q->head, (void *)info);
    }
  }
}

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

  bg_RPO[dfs_N--] = i;
}

static void print_bg_RPO(FILE *out) {
  fprintf(out, "----------------- bg_rpo -----------------\n");
  for (int i = 0; i < num_bg_nodes; ++i) {
    fprintf(out, "(rpo %d): %d\n", i, bg_RPO[i]);
  }
  fprintf(out, "\n");
}

static void compute_bg_doms() {
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
      int u = bg_RPO[i];
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

  compute_doms_iter = num_iters;
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

static void compute_bg_idoms() {
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
}

typedef struct LT_info_ *LT_info;
struct LT_info_ {
  int parent;       // parent in the DFS tree
  int semi_dfsnum;  // semi-dfsnum
  int ancestor;     // ancestor in the DFS tree
  bitmap bucket;    // n in bucket[s] means sdom(n) = s
  int label;        // node that has min dfsnum from path (ancestor[cur], cur]
};
static LT_info LT_Info_init() {
  LT_info info = (LT_info)checked_malloc(sizeof *info);
  info->parent = -1;
  info->semi_dfsnum = -1;
  info->ancestor = -1;
  info->bucket = Bitmap(num_bg_nodes);
  info->label = -1;
  return info;
}
static LT_info *lt_infos;
static int *vertex;  // vertex[i]=u means u is the ith vertex in the DFS tree
static int dfscnt;

static void dfs(int v) {
  lt_infos[v]->semi_dfsnum = dfscnt;
  vertex[dfscnt] = v;
  lt_infos[v]->label = v;
  dfscnt++;
  for (G_nodeList p = G_succ(blockInfoEnv[v]->mynode); p; p = p->tail) {
    int w = p->head->mykey;
    if (lt_infos[w]->semi_dfsnum == -1) {
      lt_infos[w]->parent = v;
      dfs(w);
    }
  }
}

static void link(int v, int w) {
  // make w a child of v
  lt_infos[w]->ancestor = v;
}

static void compress(int v) {
  if (lt_infos[v]->ancestor == -1) {
    fprintf(stderr, "compress: ancestor is -1\n");
    exit(1);
  }

  if (lt_infos[lt_infos[v]->ancestor]->ancestor != -1) {
    compress(lt_infos[v]->ancestor);
    if (lt_infos[lt_infos[lt_infos[v]->ancestor]->label]->semi_dfsnum <
        lt_infos[lt_infos[v]->label]->semi_dfsnum) {
      lt_infos[v]->label = lt_infos[lt_infos[v]->ancestor]->label;
    }
    lt_infos[v]->ancestor = lt_infos[lt_infos[v]->ancestor]->ancestor;
  }
}

/* eval(v) returns the vertex with the minimum semi_dfsnum on the path from v to
 * the root of the DFS tree, but not including the root */
static int eval(int v) {
  if (lt_infos[v]->ancestor == -1) {
    return v;
  }

  compress(v);
  return lt_infos[v]->label;
}

static void lengauer_tarjan_idoms() {
  // initialize the LT_info
  lt_infos = (LT_info *)checked_malloc(num_bg_nodes * sizeof *lt_infos);
  for (int i = 0; i < num_bg_nodes; ++i) {
    lt_infos[i] = LT_Info_init();
  }

  // DFS and initialize the vertex, semi_dfsnum, label
  vertex = (int *)checked_malloc(num_bg_nodes * sizeof *vertex);
  dfscnt = 0;
  dfs(0);
#ifdef LT_DEBUG
  for (int i = 0; i < num_bg_nodes; ++i) {
    fprintf(stderr, "vertex[%d]=%d\n", i, vertex[i]);
  }
  fprintf(stderr, "DFS done\n");
#endif

  for (int i = num_bg_nodes - 1; i > 0; --i) {
    // compute semi(w)
    int w = vertex[i];
    for (G_nodeList s = G_pred(blockInfoEnv[w]->mynode); s; s = s->tail) {
      int v = s->head->mykey;
      int u = eval(v);
      if (lt_infos[u]->semi_dfsnum < lt_infos[w]->semi_dfsnum) {
        lt_infos[w]->semi_dfsnum = lt_infos[u]->semi_dfsnum;
      }
    }
    // add w to the bucket of vertex[semi_dfsnum[w]]
    bitmap_set(lt_infos[vertex[lt_infos[w]->semi_dfsnum]]->bucket, w);
    // link w to its parent
    link(lt_infos[w]->parent, w);

    // compute idom
    for (int v = 0; v < num_bg_nodes; ++v) {
      if (bitmap_read(lt_infos[lt_infos[w]->parent]->bucket, v)) {
        int u = eval(v);
        if (lt_infos[u]->semi_dfsnum < lt_infos[v]->semi_dfsnum) {
          // idom[v] = idom[u], need to reconsider v later
          blockInfoEnv[v]->idom = u;
        } else {
          // idom[v] = semi[v] = parent[w]
          blockInfoEnv[v]->idom = lt_infos[w]->parent;
        }
      }
    }
  }
#ifdef LT_DEBUG
  fprintf(stderr, "sdom done\n");
#endif

  // compute the rest of idoms
  blockInfoEnv[0]->idom = -1;
  for (int i = 1; i < num_bg_nodes; ++i) {
    int w = vertex[i];
    if (blockInfoEnv[w]->idom != vertex[lt_infos[w]->semi_dfsnum]) {
      blockInfoEnv[w]->idom = blockInfoEnv[blockInfoEnv[w]->idom]->idom;
    }
  }
}

static void lt_dfs_dom_tree(int u) {
  for (blockIdList p = blockInfoEnv[u]->dom_tree_children; p; p = p->tail) {
    int v = p->blockid;
    bitmap_set(blockInfoEnv[v]->doms, v);
    bitmap_union_into(blockInfoEnv[v]->doms, blockInfoEnv[u]->doms);
    lt_dfs_dom_tree(v);
  }
}

static void lengauer_tarjan_doms() {
  // after constructing the dominator tree, compute the dominators
  bitmap_set(blockInfoEnv[0]->doms, 0);
  lt_dfs_dom_tree(0);
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
    for (blockIdList p = blockInfoEnv[i]->dom_tree_children; p; p = p->tail) {
      fprintf(out, "%d ", p->blockid);
    }
    fprintf(out, "\n");
  }
  fprintf(out, "\n");
}

static void compute_bg_dom_frontiers() { compute_bg_df_recur(0); }

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
  for (blockIdList p = blockInfoEnv[u]->dom_tree_children; p; p = p->tail) {
    int w = p->blockid;
    compute_bg_df_recur(w);
    for (blockIdList q = blockInfoEnv[w]->dom_frontiers; q; q = q->tail) {
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
    for (blockIdList p = blockInfoEnv[i]->dom_frontiers; p; p = p->tail) {
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

static void compute_phi_functions(G_nodeList lg) {
  // place phi functions in the program

  // step 1: record defsites of each variable
  G_nodeList p = lg;
  for (int i = 0; i < num_bg_nodes; ++i) {
    for (Temp_tempList tl = blockInfoEnv[i]->orig_vars; tl; tl = tl->tail) {
      Temp_temp var = tl->head;
      SSA_var_info info = (SSA_var_info)TAB_look(varInfoEnv, (void *)var);
      if (!info) {
        fprintf(stderr, "Error: var not found in varInfoEnv\n");
        exit(1);
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
      for (blockIdList p = blockInfoEnv[u]->dom_frontiers; p; p = p->tail) {
        int v = p->blockid;
        // if var not in phi_vars(v), then place phi function for var
        if (!Temp_TempInTempList(var, blockInfoEnv[v]->phi_vars) &&
            Temp_TempInTempList(var, blockInfoEnv[v]->blockIn)) {
          place_phi_func(var, v);
          blockInfoEnv[v]->phi_vars =
              Temp_TempList(var, blockInfoEnv[v]->phi_vars);
          // need to reconsider v
          if (!Temp_TempInTempList(var, blockInfoEnv[v]->orig_vars)) {
            bitmap_set(w, v);
          }
        }
      }
    }

    top = (Temp_temp)b->prevtop;
  }
}

static void place_phi_func(Temp_temp var, int v) {
  if (!G_pred(blockInfoEnv[v]->mynode)) {
    return;
  }

  // place phi function for var in block v
  Temp_tempList dsts = Temp_TempList(Temp_namedtemp(var->num, var->type), NULL);
  Temp_tempList srcs = NULL;
  Temp_labelList labels = NULL;
  string instr_str =
      Stringf("%%`d0 = phi %s ", var->type == T_int ? "i64" : "double");
  int cnt = 0;
  for (G_nodeList p = G_pred(blockInfoEnv[v]->mynode); p; p = p->tail, ++cnt) {
    srcs = Temp_TempList(Temp_namedtemp(var->num, var->type), srcs);
    AS_block block = G_nodeInfo(p->head);
    labels = Temp_LabelListSplice(labels, Temp_LabelList(block->label, NULL));
    strcat(instr_str, Stringf("[%%`s%d, %%`j%d]", cnt, cnt));
    if (p->tail) {
      strcat(instr_str, ", ");
    }
  }

  // insert the phi function
  blockInfoEnv[v]->instrInfos =
      Insert_phi_func(blockInfoEnv[v]->instrInfos,
                      AS_Oper(instr_str, dsts, srcs, AS_Targets(labels)),
                      Temp_TempList(var, NULL));
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

static void rename_vars() {
#ifdef RENAME_DEBUG
  fprintf(out, "Enter rename_vars\n");
#endif
  // rename variables in the program

  // step 1: initialize the stack
  Temp_temp top = varInfoEnv->top;
  binder b;
  while (top) {
    b = TAB_getBinder(varInfoEnv, (void *)top);
    Temp_temp var = (Temp_temp)b->key;
    SSA_var_info info = (SSA_var_info)b->value;
    Var_Stack_push(info, var);
    top = (Temp_temp)b->prevtop;
  }

  // step 2: rename variables
  rename_vars_recur(0);
}

static int get_pred_num(G_nodeList pred, int u) {
  int cnt = 0;
  for (G_nodeList p = pred; p; p = p->tail, ++cnt) {
    if (p->head->mykey == u) {
      return cnt;
    }
  }
  return -1;
}

static bool is_phi_func(AS_instr instr) {
  return instr->kind == I_OPER && instr->u.OPER.assem[0] == '%' &&
         instr->u.OPER.assem[7] == 'p' && instr->u.OPER.assem[8] == 'h' &&
         instr->u.OPER.assem[9] == 'i';
}

static void rename_vars_recur(int u) {
#ifdef RENAME_DEBUG
  fprintf(out, "rename_vars_recur: %d\n", u);
#endif
  // rename variables in block u
  for (instrInfoList p = blockInfoEnv[u]->instrInfos; p; p = p->tail) {
    AS_instr S = p->instr;
    if (!is_phi_func(S)) {
      Temp_tempList use_x = NULL;
      switch (S->kind) {
        case I_OPER: {
          use_x = S->u.OPER.src;
          break;
        }
        case I_MOVE: {
          use_x = S->u.MOVE.src;
          break;
        }
        default:
          break;
      }
      while (use_x) {
#ifdef RENAME_DEBUG
        fprintf(out, "rename1: %d -> ", use_x->head->num);
#endif
        use_x->head = Var_Stack_top((SSA_var_info)TAB_look(
            varInfoEnv, Temp_namedtemp(use_x->head->num, use_x->head->type)));
#ifdef RENAME_DEBUG
        fprintf(out, "%d\n", use_x->head->num);
#endif
        use_x = use_x->tail;
      }
    }

    Temp_tempList def_x = NULL;
    switch (S->kind) {
      case I_OPER: {
        def_x = S->u.OPER.dst;
        break;
      }
      case I_MOVE: {
        def_x = S->u.MOVE.dst;
        break;
      }
      default:
        break;
    }

    // record the origin def
    p->origin_def = NULL;
    for (Temp_tempList q = def_x; q; q = q->tail) {
      p->origin_def =
          Temp_TempListSplice(p->origin_def, Temp_TempList(q->head, NULL));
    }

    while (def_x) {
      Temp_temp new_x = Temp_newtemp(def_x->head->type);
#ifdef RENAME_DEBUG
      fprintf(out, "rename2: %d -> %d\n", def_x->head->num, new_x->num);
      fprintf(out, "old stack: ");
      print_var_stack(out,
                      ((SSA_var_info)TAB_look(varInfoEnv, def_x->head))->stack);
#endif
      Var_Stack_push((SSA_var_info)TAB_look(varInfoEnv, def_x->head), new_x);
#ifdef RENAME_DEBUG
      fprintf(out, "new stack: %d", TAB_look(varInfoEnv, def_x->head) == NULL);
      print_var_stack(out,
                      ((SSA_var_info)TAB_look(varInfoEnv, def_x->head))->stack);
#endif
      def_x->head = new_x;
      def_x = def_x->tail;
    }
  }

  for (G_nodeList Y = G_succ(blockInfoEnv[u]->mynode); Y; Y = Y->tail) {
    int cnt = get_pred_num(G_pred(Y->head), u);
    for (instrInfoList p = blockInfoEnv[Y->head->mykey]->instrInfos; p;
         p = p->tail) {
      if (!is_phi_func(p->instr)) {
        continue;
      }

      int op_cnt = 0;
      for (Temp_tempList q = p->instr->u.OPER.src; q; q = q->tail, ++op_cnt) {
        if (op_cnt == cnt) {
#ifdef RENAME_DEBUG
          fprintf(out, "rename3: %d -> ", q->head->num);
#endif
          q->head = Var_Stack_top((SSA_var_info)TAB_look(varInfoEnv, q->head));
#ifdef RENAME_DEBUG
          fprintf(out, "%d\n", q->head->num);
#endif
        }
      }
    }
  }
#ifdef RENAME_DEBUG
  fprintf(out, "rename_vars_recur: %d done\n", u);
#endif

  // rename variables in the children of u
  for (blockIdList p = blockInfoEnv[u]->dom_tree_children; p; p = p->tail) {
    rename_vars_recur(p->blockid);
  }

  // pop the stack
  for (instrInfoList p = blockInfoEnv[u]->instrInfos; p; p = p->tail) {
    Temp_tempList def_x = p->origin_def;
    while (def_x) {
#ifdef RENAME_DEBUG
      fprintf(out, "rename4: %d\n", def_x->head->num);
      fprintf(out, "old stack: ");
      print_var_stack(out,
                      ((SSA_var_info)TAB_look(varInfoEnv, def_x->head))->stack);
#endif
      Var_Stack_pop((SSA_var_info)TAB_look(varInfoEnv, def_x->head));
#ifdef RENAME_DEBUG
      fprintf(out, "new stack: ");
      print_var_stack(out,
                      ((SSA_var_info)TAB_look(varInfoEnv, def_x->head))->stack);
#endif
      def_x = def_x->tail;
    }
  }
}

static AS_instrList get_final_result() {
  AS_instrList result = NULL;
  for (int i = 0; i < num_bg_nodes; ++i) {
    for (instrInfoList p = blockInfoEnv[i]->instrInfos; p; p = p->tail) {
      result = AS_splice(result, AS_InstrList(p->instr, NULL));
    }
  }
  return result;
}

static bool isSSA = TRUE;

AS_instrList AS_instrList_to_SSA(AS_instrList bodyil, G_nodeList lg,
                                 G_nodeList bg) {
  /* here is your implementation of translating to ssa */

  if (!lg || !bg) {
    isSSA = FALSE;
    return bodyil;
  }
  out = stderr;

  SSA_init(lg, bg);

  /* compute DF */

  if (!Lengauer_Tarjan_Enable) {  // step 1: sort in RPO
    sort_bg_in_RPO();
#ifdef SSA_DEBUG
    print_bg_RPO(out);
#endif

    // step 2: compute the dominators
    compute_bg_doms(out);
#ifdef SSA_DEBUG
    print_bg_doms(out, compute_doms_iter);
#endif

    // step 3: compute the dominator tree of bg
    compute_bg_idoms();
#ifdef SSA_DEBUG
    fprintf(out, "----------------- bg_idoms -----------------\n");
    print_bg_idoms(out);
#endif

    construct_bg_dom_tree();
#ifdef SSA_DEBUG
    fprintf(out, "----------------- bg_dom_tree -----------------\n");
    print_bg_dom_tree(out);
#endif
  } else {
    lengauer_tarjan_idoms();
#ifdef SSA_DEBUG
    fprintf(out, "----------------- bg_idoms -----------------\n");
    print_bg_idoms(out);
#endif

    construct_bg_dom_tree();
#ifdef SSA_DEBUG
    fprintf(out, "----------------- bg_dom_tree -----------------\n");
    print_bg_dom_tree(out);
#endif

    lengauer_tarjan_doms();
#ifdef SSA_DEBUG
    print_bg_doms(out, 0);
#endif
  }

  compute_bg_dom_frontiers();
#ifdef SSA_DEBUG
  fprintf(out, "----------------- bg_dom_frontiers -----------------\n");
  print_bg_dom_frontiers(out);
#endif

  /* place phi functions */

  compute_phi_functions(lg);
#ifdef SSA_DEBUG
  fprintf(out, "----------------- bg_phi_functions -----------------\n");
  print_bg_phi_functions(out);
#endif

  /* rename variables */

  rename_vars();

  return get_final_result();
}

G_graph Create_SSA_bg(G_nodeList bg) {
  if (!isSSA) {
    return bg ? bg->head->mygraph : NULL;
  }
  for (G_nodeList p = bg; p; p = p->tail) {
    AS_instrList instrs = NULL;
    for (instrInfoList q = blockInfoEnv[p->head->mykey]->instrInfos; q;
         q = q->tail) {
      instrs = AS_splice(instrs, AS_InstrList(q->instr, NULL));
    }
    p->head->info = AS_Block(instrs);
  }
  return bg->head->mygraph;
}

static bool isBlockContainPhi(AS_block b) {
  for (AS_instrList p = b->instrs; p; p = p->tail) {
    if (is_phi_func(p->head)) {
      return TRUE;
    }
  }
  return FALSE;
}

static bool isNodeHasMultiplePred(G_node n) {
  G_nodeList p = G_pred(n);
  return p && p->tail;
}

static bool isNodeHasMultipleSucc(G_node n) {
  G_nodeList p = G_succ(n);
  return p && p->tail;
}

static bool isInstrCmpOrJump(AS_instr instr) {
  return instr->kind == I_OPER && (strstr(instr->u.OPER.assem, "cmp") ||
                                   strstr(instr->u.OPER.assem, "br") ||
                                   strstr(instr->u.OPER.assem, "ret"));
}

static G_node splitNewBlock(G_graph bg, G_node u, G_node v) {
  Temp_label vLabel = ((AS_block)G_nodeInfo(v))->label;
  // create a new block
  Temp_label new_label = Temp_newlabel_prefix('S');
  AS_instr labelIns =
      AS_Label(Stringf("%s:", Temp_labelstring(new_label)), new_label);
  AS_instr jmpIns = AS_Oper("br label \%`j0", NULL, NULL,
                            AS_Targets(Temp_LabelList(vLabel, NULL)));
  AS_instrList instrs = AS_InstrList(labelIns, AS_InstrList(jmpIns, NULL));
  AS_block new_block = AS_Block(instrs);

  // change u's last instr to a jump to the new block
  AS_block u_block = G_nodeInfo(u);
  AS_instrList last = u_block->instrs;
  while (last->tail) {
    last = last->tail;
  }
  if (!last->head->kind == I_OPER) {
    fprintf(stderr, "Error: last instr is not I_OPER\n");
    exit(1);
  }
  for (Temp_labelList p = last->head->u.OPER.jumps->labels; p; p = p->tail) {
    if (p->head == vLabel) {
      p->head = new_label;
    }
  }

  // add the new block to the graph
  G_node new_node = G_Node(bg, new_block);
#ifdef SSA_DEC_DEBUG
  fprintf(out, "last1: %p\n", bg->mylast);
  G_nodeList last2 = G_nodes(bg);
  while (last2->tail) {
    last2 = last2->tail;
  }
  fprintf(out, "last2: %p\n", last2);
#endif
  G_addEdge(u, new_node);
  G_addEdge(new_node, v);

  return new_node;
}

AS_instrList SSA_deconstruct(AS_instrList bodyil, G_graph ssa_bg) {
  if (!isSSA) {
    return bodyil;
  }

  int originNodeCnt = ssa_bg->nodecount;
  for (G_nodeList p = G_nodes(ssa_bg); p && originNodeCnt;
       p = p->tail, --originNodeCnt) {
    G_node v = p->head;
    AS_block b = G_nodeInfo(v);
    if (!isBlockContainPhi(b)) {
      continue;
    }
#ifdef SSA_DEC_DEBUG
    fprintf(out, "SSA_dec: %s\n", Temp_labelstring(b->label));
#endif
    // critical edge splitting
    S_table parallelCopyTab = S_empty();

    G_nodeList q = G_pred(v);
    while (q) {
      G_node u = q->head;
      q = q->tail;

      AS_block pred = G_nodeInfo(u);
      if (!isNodeHasMultiplePred(v) || !isNodeHasMultipleSucc(u)) {
        S_enter(parallelCopyTab, S_Symbol(Temp_labelstring(pred->label)),
                (void *)u);
        continue;
      } else {
#ifdef SSA_DEC_DEBUG
        fprintf(out, "split edge: %s -> %s\n", Temp_labelstring(pred->label),
                Temp_labelstring(b->label));
#endif
        // split the edge: pred -> pred', pred' -> n
        G_rmEdge(u, v);
        G_node newNode = splitNewBlock(ssa_bg, u, v);
#ifdef SSA_DEC_DEBUG
        AS_block newBlock = G_nodeInfo(newNode);
        fprintf(out, "new block: %s\n", Temp_labelstring(newBlock->label));
#endif
        S_enter(parallelCopyTab, S_Symbol(Temp_labelstring(pred->label)),
                (void *)newNode);
      }
    }
#ifdef SSA_DEC_DEBUG
    fprintf(out, "== new bg ==\n");
    Show_bg(out, G_nodes(ssa_bg));
    fprintf(out, "edge split done\n");
#endif

    // deconstruct phi functions
    for (AS_instrList pre = b->instrs, cur = pre->tail; cur; cur = cur->tail) {
      if (!is_phi_func(cur->head)) {
        pre = cur;
        continue;
      }

      Temp_temp dst = cur->head->u.OPER.dst->head;
      Temp_tempList srcs = cur->head->u.OPER.src;
      Temp_labelList labels = cur->head->u.OPER.jumps->labels;

      for (; srcs; srcs = srcs->tail, labels = labels->tail) {
        Temp_temp src = srcs->head;
        Temp_label label = labels->head;

        // find the block that contains the label
        G_node u =
            (G_node)S_look(parallelCopyTab, S_Symbol(Temp_labelstring(label)));
        AS_block pred = G_nodeInfo(u);
#ifdef SSA_DEC_DEBUG
        fprintf(out, "pred: %s\n", Temp_labelstring(pred->label));
#endif

        // insert a move instruction
        AS_instrList prevLastIns = pred->instrs;
        AS_instrList lastIns = prevLastIns->tail;
        while (!isInstrCmpOrJump(lastIns->head)) {
          prevLastIns = lastIns;
          lastIns = lastIns->tail;
        }
        AS_instr moveIns = NULL;
        switch (src->type) {
          case T_int: {
            moveIns =
                AS_Move("\%`d0 = add i64 \%`s0, 0", Temp_TempList(dst, NULL),
                        Temp_TempList(src, NULL));
            break;
          }
          case T_float: {
            moveIns =
                AS_Move("\%`d0 = fadd double \%`s0, 0.0",
                        Temp_TempList(dst, NULL), Temp_TempList(src, NULL));
            break;
          }
          default:
            break;
        }
        prevLastIns->tail = AS_InstrList(moveIns, lastIns);
      }

      // remove the phi function
      pre->tail = cur->tail;
    }
  }

  AS_instrList result = NULL;
  for (G_nodeList p = G_nodes(ssa_bg); p; p = p->tail) {
    AS_block b = G_nodeInfo(p->head);
    for (AS_instrList q = b->instrs; q; q = q->tail) {
      result = AS_splice(result, AS_InstrList(q->head, NULL));
    }
  }

  return result;
}