#ifndef STRTABLE_H
#define STRTABLE_H

/*
 * table.h - generic hash table
 *
 * No algorithm should use these functions directly, because
 *  programming with void* is too error-prone.  Instead,
 *  each module should make "wrapper" functions that take
 *  well-typed arguments and call the TAB_ functions.
 */

#define STRTABSIZE 127

typedef struct STRTAB_table_ *STRTAB_table;
typedef struct strbinder_ *strbinder;

struct strbinder_ {
  char *key;
  int value;
  strbinder next;
  char *prevtop;
};

struct STRTAB_table_ {
  strbinder table[STRTABSIZE];
  char *top;
};

/* Make a new table mapping "keys" to "values". */
STRTAB_table STRTAB_empty(void);

/* Enter the mapping "key"->"value" into table "t",
 *  shadowing but not destroying any previous binding for "key". */
void STRTAB_enter(STRTAB_table t, char *key, int value);

/* Look up the most recent binding for "key" in table "t" */
int STRTAB_look(STRTAB_table t, char *key);

/* Pop the most recent binding and return its key.
 * This may expose another binding for the same key, if there was one. */
char *STRTAB_pop(STRTAB_table t);


/* Call "show" on every "key"->"value" pair in the table,
 *  including shadowed bindings, in order from the most
 *  recent binding of any key to the oldest binding in the table */
void STRTAB_dump(STRTAB_table t, void (*show)(char *key, int value));

strbinder STRTAB_getBinder(STRTAB_table t, char *key);

#endif
