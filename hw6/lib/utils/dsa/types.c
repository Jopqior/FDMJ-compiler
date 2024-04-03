/*
 * types.c -
 *
 * All types and functions declared in this header file begin with "Ty_"
 * Linked list types end with "..list"
 */

#include <stdio.h>
#include "util.h"
#include "symbol.h"
#include "types.h"

static struct Ty_ty_ tyint = {Ty_int};
Ty_ty Ty_Int(void) {
  return &tyint;
}

static struct Ty_ty_ tystring = {Ty_float};
Ty_ty Ty_Float(void) {
  return &tystring;
}

Ty_ty Ty_Array(Ty_ty ty) {
  Ty_ty p = checked_malloc(sizeof(*p));
  p->kind = Ty_array;
  p->u.array = ty;
  return p;
}

Ty_ty Ty_Name(S_symbol sym) {
  Ty_ty p = checked_malloc(sizeof(*p));
  p->kind = Ty_name;
  p->u.name = sym;
  return p;
}

Ty_field Ty_Field(S_symbol name, Ty_ty ty) {
  Ty_field p = checked_malloc(sizeof(*p));
  p->name = name;
  p->ty = ty;
  return p;
}

Ty_fieldList Ty_FieldList(Ty_field head, Ty_fieldList tail) {
  Ty_fieldList p = checked_malloc(sizeof(*p));
  p->head = head;
  p->tail = tail;
  return p;
}

/* printing functions - used for debugging */
static char str_ty[][12] = {
  "ty_int", "ty_float",
  "ty_array", "ty_name"
};

/* This will infinite loop on mutually recursive types */
void Ty_print(Ty_ty t) {
  if (t == NULL) printf("null");
  else {
    fprintf(stderr, "%s", str_ty[t->kind]);
    if (t->kind == Ty_name) {
      fprintf(stderr, ", %s", S_name(t->u.name));
    }
    fprintf(stderr, "\n");
  }
}
