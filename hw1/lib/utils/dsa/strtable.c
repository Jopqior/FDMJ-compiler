#include <stdio.h>
#include <string.h>
#include "util.h"
#include "strtable.h"

static strbinder StrBinder(char *key, int value, strbinder next, char *prevtop) {
  strbinder b = checked_malloc(sizeof(*b));
  b->key = key;
  b->value = value;
  b->next = next;
  b->prevtop = prevtop;
  return b;
}

STRTAB_table STRTAB_empty(void) {
  STRTAB_table t = checked_malloc(sizeof(*t));
  int i;
  t->top = NULL;
  for (i = 0; i < STRTABSIZE; i++)
    t->table[i] = NULL;
  return t;
}

unsigned long hash(char *s) {
    int len = strlen(s);
    char *p = s;

    unsigned long h = 5381;
    while (len--) {
        h = (((h << 5) + h) + *(p++)) % STRTABSIZE;
    }
    return h;
}

void STRTAB_enter(STRTAB_table t, char *key, int value) {
  int index;
  assert(t && key);
  index = hash(key);
  t->table[index] = StrBinder(key, value, t->table[index], t->top);
  t->top = key;
}

int STRTAB_look(STRTAB_table t, char *key) {
  int index;
  strbinder b;
  assert(t && key);
  index = hash(key);
  for (b = t->table[index]; b; b = b->next)
    if (strcmp(b->key, key) == 0) return b->value;
  return -1;
}

char *STRTAB_pop(STRTAB_table t) {
  char *k;
  strbinder b;
  int index;
  assert(t);
  k = t->top;
  assert(k);
  index = hash(k);
  b = t->table[index];
  assert(b);
  t->table[index] = b->next;
  t->top = b->prevtop;
  return b->key;
}

void STRTAB_dump(STRTAB_table t, void (*show)(char *key, int value)) {
  char *k = t->top;
  int index = hash(k);
  strbinder b = t->table[index];
  if (b == NULL) return;
  t->table[index] = b->next;
  t->top = b->prevtop;
  show(b->key, b->value);
  STRTAB_dump(t, show);
  assert(t->top == b->prevtop && t->table[index] == b->next);
  t->top = k;
  t->table[index] = b;
}

strbinder STRTAB_getBinder(STRTAB_table t, char *key) {
  int index;
  strbinder b;
  assert(t && key);
  index = hash(key);
  for (b = t->table[index]; b; b = b->next)
    if (strcmp(b->key, key) == 0) return b;
  return NULL;
}