#ifndef SEMANT_H
#define SEMANT_H

#include <stdio.h>
#include "env.h"
#include "fdmjast.h"
#include "translate.h"
#include "tigerirp.h"
#include "util.h"

T_funcDeclList transA_Prog(FILE *out, A_prog p, int arch_size);

#endif