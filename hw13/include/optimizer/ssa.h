#ifndef SSA_H
#define SSA_H

#include "assem.h"
#include "assemblock.h"
#include "bg.h"
#include "graph.h"
#include "liveness.h"
#include "symbol.h"
#include "util.h"

AS_instrList AS_instrList_to_SSA(AS_instrList bodyil, G_nodeList lg, G_nodeList bg);
G_graph Create_SSA_bg(G_nodeList bg);
AS_instrList SSA_destruction(AS_instrList bodyil, G_graph ssa_bg);

#endif