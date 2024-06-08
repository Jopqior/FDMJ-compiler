#ifndef LLVMGEN_H
#define LLVMGEN_H

#include "assem.h"
#include "tigerirp.h"
#include "util.h"

AS_instrList llvmbody(T_stmList stmList);
AS_instrList llvmbodybeg(Temp_label lbeg);
AS_instrList llvmprolog(string methodname, Temp_tempList args, T_type rettype);
AS_instrList llvmepilog(Temp_label lend);

#endif