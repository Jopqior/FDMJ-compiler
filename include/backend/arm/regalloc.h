#ifndef REGALLOC_H
#define REGALLOC_H

#include <stdio.h>
#include "assem.h"
#include "flowgraph.h"
#include "ig.h"
#include "liveness.h"
#include "util.h"

typedef struct RA_result_ *RA_result;
struct RA_result_ {
  Temp_map coloring;
  AS_instrList il;
};

RA_result RA_regAlloc(AS_instrList il, G_nodeList ig);

#endif