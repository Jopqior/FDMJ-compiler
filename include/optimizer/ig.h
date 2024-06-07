#ifndef IG_H
#define IG_H

#include <stdio.h>
#include <stdlib.h>
#include "util.h"
#include "table.h"
#include "graph.h"
#include "symbol.h"
#include "temp.h"
#include "assem.h"
#include "flowgraph.h"
#include "liveness.h"

void Enter_ig(Temp_temp, Temp_temp);
G_nodeList Create_ig(G_nodeList);
void Show_ig(FILE *, G_nodeList);

#endif
