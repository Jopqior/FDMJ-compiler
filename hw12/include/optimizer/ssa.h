#ifndef SSA_H
#define SSA_H

#include "assem.h"
#include "assemblock.h"
#include "bg.h"
#include "graph.h"
#include "liveness.h"
#include "symbol.h"
#include "util.h"

AS_instrList AS_instrList_to_SSA(AS_instrList bodyil, G_nodeList fg, G_nodeList bg);

#endif