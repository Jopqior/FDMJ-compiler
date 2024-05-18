#ifndef ARMGEN_H
#define ARMGEN_H

#include "assem.h"
#include "util.h"

AS_instrList armprolog(AS_instrList il);
AS_instrList armbody(AS_instrList il);
AS_instrList armepilog(AS_instrList il);

#endif
