#ifndef SSA_H
#define SSA_H

#include "assem.h"
#include "assemblock.h"
#include "bg.h"
#include "graph.h"
#include "liveness.h"
#include "symbol.h"
#include "util.h"

AS_instrList SSA_construction(AS_instrList bodyil, G_nodeList lg, G_nodeList bg);
AS_instrList SSA_destruction(AS_instrList bodyil, G_nodeList bg);

#endif