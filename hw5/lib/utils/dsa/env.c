#include "env.h"

E_enventry E_VarEntry(A_varDecl vd, Ty_ty ty) {
  E_enventry entry = checked_malloc(sizeof(*entry));
  entry->kind = E_varEntry;
  entry->u.var.vd = vd;
  entry->u.var.ty = ty;
  return entry;
}

E_enventry E_ClassEntry(A_classDecl cd, S_symbol fa, E_status status, S_table vtbl, S_table mtbl) {
  E_enventry entry = checked_malloc(sizeof(*entry));
  entry->kind = E_classEntry;
  entry->u.cls.cd = cd;
  entry->u.cls.fa = fa;
  entry->u.cls.status = status;
  entry->u.cls.vtbl = vtbl;
  entry->u.cls.mtbl = mtbl;
  return entry;
}

E_enventry E_MethodEntry(A_methodDecl md, S_symbol from, Ty_ty ret, Ty_fieldList fl) {
  E_enventry entry = checked_malloc(sizeof(*entry));
  entry->kind = E_methodEntry;
  entry->u.meth.md = md;
  entry->u.meth.from = from;
  entry->u.meth.ret = ret;
  entry->u.meth.fl = fl;
  return entry;
}
