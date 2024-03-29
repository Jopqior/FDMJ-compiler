#ifndef ENV_H
#define ENV_H

#include "fdmjast.h"
#include "symbol.h"
#include "types.h"
#include "util.h"

typedef struct E_enventry_ *E_enventry;

typedef enum {
  E_transInit,
  E_transFind,
  E_transFill
} E_status;

typedef enum {
  E_varEntry, 
  E_classEntry, 
  E_methodEntry
} E_enventryKind;

struct E_enventry_ {
  E_enventryKind kind;
  union {
    struct {
      A_varDecl vd;
      Ty_ty ty;
    } var;
    struct {
      A_classDecl cd;
      S_symbol fa;
      E_status status;
      S_table vtbl;
      S_table mtbl;
    } cls;
    struct {
      A_methodDecl md;
      S_symbol from;
      Ty_ty ret;
      Ty_fieldList fl;
    } meth;
  } u;
};

E_enventry E_VarEntry(A_varDecl vd, Ty_ty ty);
E_enventry E_ClassEntry(A_classDecl cd, S_symbol fa, E_status status, S_table vtbl, S_table mtbl);
E_enventry E_MethodEntry(A_methodDecl md, S_symbol from, Ty_ty ret, Ty_fieldList fl);

#endif
