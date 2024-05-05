#ifndef SSA_H
#define SSA_H

#include "assem.h"
#include "graph.h"

AS_instrList AS_instrList_to_SSA(AS_instrList bodyil, G_graph fg, G_nodeList bg);

#endif