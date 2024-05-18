#ifndef UTIL_H
#define UTIL_H

#include <assert.h>

typedef char *string;
typedef char bool;

#define TRUE 1
#define FALSE 0
#define IR_MAXLEN 200

void *checked_malloc(int);
void *checked_calloc(int, int);
string String(char *);
string Stringf(const char *, ...);

typedef struct U_boolList_ *U_boolList;
struct U_boolList_ {
  bool head;
  U_boolList tail;
};
U_boolList U_BoolList(bool head, U_boolList tail);

#endif