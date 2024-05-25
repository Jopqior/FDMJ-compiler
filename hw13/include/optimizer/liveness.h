#ifndef LIVENESS_H
#define LIVENESS_H

#include <stdio.h>
#include "util.h"
#include "table.h"
#include "graph.h"
#include "symbol.h"
#include "temp.h"
#include "assem.h"
#include "flowgraph.h"

G_nodeList Liveness(G_nodeList);
void Show_Liveness(FILE*, G_nodeList);
Temp_tempList FG_Out(G_node);
Temp_tempList FG_In(G_node);

#endif
