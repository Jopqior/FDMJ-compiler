#ifndef BG_H
#define BG_H

#include <stdio.h>
#include <stdlib.h>
#include "util.h"
#include "assemblock.h"
#include "table.h"
#include "graph.h"
#include "symbol.h"
#include "temp.h"
#include "assem.h"

/* Block graph: graph on AS_ basic blocks. 
   This is useful to find dominance relations, etc. */

/* input AS_blockList after instruction selection for each block
   in the C_Block, generate a basic blocks graph called bg */
G_nodeList Create_bg(AS_blockList);

/* get bg */
G_graph Bg_graph();
S_table Bg_env();

/* print bg */
void Show_bg(FILE *, G_nodeList);

#endif