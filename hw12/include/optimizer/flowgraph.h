/*
 * flowgraph.h - Function prototypes to represent control flow graphs.
 */

#ifndef FLOWGRAPH_H
#define FLOWGRAPH_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "table.h"
#include "symbol.h"
#include "temp.h"
#include "assem.h"
#include "graph.h"

Temp_tempList FG_def(G_node n);
Temp_tempList FG_use(G_node n);
bool FG_isMove(G_node n);
G_graph FG_AssemFlowGraph(AS_instrList il);
void FG_Showinfo(FILE*, AS_instr, Temp_map);
void FG_show(AS_instr ins);

#endif
