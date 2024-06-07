#include "liveness.h"

//structure for in and out temps to attach to the flowgraph nodes
struct inOut_ {
  Temp_tempList in;
  Temp_tempList out;
};
typedef struct inOut_ *inOut;

//This is the (global) table for storing the in and out temps
static G_table InOutTable = NULL;

//initialize the table
static void init_INOUT() {
  if (InOutTable == NULL) {
    InOutTable = G_empty();
  }
}

//Attach the inOut info to the table
static void INOUT_enter(G_node n, inOut info) {
  G_enter(InOutTable, n, info);
}

//Lookup the inOut info
static inOut INOUT_lookup(G_node n) {
  return G_look(InOutTable, n);
}

//inOut Constructor
static inOut InOut(Temp_tempList in, Temp_tempList out) {
  inOut info = (inOut)checked_malloc(sizeof(inOut));
  info->in = in;
  info->out = out;
  return info;
}

//for printing purpose
void PrintTemp(FILE *out, Temp_temp t) {
  fprintf(out, " %s ", Temp_look(Temp_name(), t));
}

//Printing
void PrintTemps(FILE *out, Temp_tempList tl) {
  if (tl == NULL) return;
  Temp_temp h = tl->head;
  fprintf(out, "%s, ", Temp_look(Temp_name(), h));
  PrintTemps(out, tl->tail);
}

Temp_tempList FG_In(G_node n) {
  inOut io;
  io = INOUT_lookup(n);
  if (io != NULL) return io->in;
  else return NULL;
}

Temp_tempList FG_Out(G_node n) {
  inOut io;
  io = INOUT_lookup(n);
  if (io != NULL) return io->out;
  else return NULL;
}

//initialize the INOUT info for a graph
static void init_INOUT_graph(G_nodeList l) {
  while ( l != NULL ) {
    if (INOUT_lookup(l->head) == NULL) //If there is no io info yet, initialize one
      INOUT_enter(l->head, InOut(NULL, NULL));
    l = l->tail;
  }
}

static int gi = 0;

static bool LivenessIteration(G_nodeList gl) {
  bool changed = FALSE;
  gi++;
  while ( gl != NULL ) {
    G_node n = gl->head;

    //do in[n] = use[n] union (out[n] - def[n])
    Temp_tempList in = Temp_TempListUnion(FG_use(n), Temp_TempListDiff(FG_Out(n), FG_def(n)));

    //Now do out[n]=union_s in succ[n] (in[s])
    G_nodeList s = G_succ(n);
    Temp_tempList out = NULL; //out is an accumulator
    for (; s != NULL; s = s->tail) {
      out = Temp_TempListUnion(out, FG_In(s->head));
    }
    //See if any in/out changed
    if (!(Temp_TempListEq(FG_In(n), in) && Temp_TempListEq(FG_Out(n), out)))
      changed = TRUE;
    //enter the new info
    G_enter(InOutTable, gl->head, InOut(in, out));
    gl = gl->tail;
  }
  return changed;
}

void Show_Liveness(FILE* out, G_nodeList l) {
  fprintf(out, "Number of iterations=%d\n", gi);
  while ( l != NULL ) {
    G_node n = l->head;
    fprintf(out, "----------------------\n");
    G_show(out, G_NodeList(n, NULL), NULL);
    fprintf(out, "def=");
    PrintTemps(out, FG_def(n));
    fprintf(out, "\n");
    fprintf(out, "use=");
    PrintTemps(out, FG_use(n));
    fprintf(out, "\n");
    fprintf(out, "In=");
    PrintTemps(out, FG_In(n));
    fprintf(out, "\n");
    fprintf(out, "Out=");
    PrintTemps(out, FG_Out(n));
    fprintf(out, "\n");
    l = l->tail;
  }
}

G_nodeList Liveness(G_nodeList l) {
  init_INOUT(); //Initialize InOut table
  bool changed = TRUE;
  while (changed) changed = LivenessIteration(l);
  return l;
}
