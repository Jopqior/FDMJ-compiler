#ifndef REGALLOC_H
#define REGALLOC_H

#include <stdio.h>
#include "assem.h"
#include "flowgraph.h"
#include "ig.h"
#include "liveness.h"
#include "util.h"

AS_instrList regalloc(AS_instrList il, G_nodeList ig);

#endif