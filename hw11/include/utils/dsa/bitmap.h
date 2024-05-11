#ifndef BITMAP_H
#define BITMAP_H

#include <stdio.h>

#include "util.h"

#define bitmap_type unsigned int

typedef struct bitmap_ *bitmap;
struct bitmap_ {
  bitmap_type *array;
  int bits;   // number of bits in the array
  int words;  // number of words in the array
};

bitmap Bitmap(int bits);

void bitmap_set(bitmap b, int n);
void bitmap_clear(bitmap b, int n);
int bitmap_read(bitmap b, int n);

void bitmap_set_all(bitmap b);
void bitmap_clear_all(bitmap b);

void bitmap_copy(bitmap dst, bitmap src);
void bitmap_union_into(bitmap dst, bitmap src);
void bitmap_intersection_into(bitmap dst, bitmap src);

bitmap bitmap_union(bitmap b1, bitmap b2);
bitmap bitmap_intersection(bitmap b1, bitmap b2);
bitmap bitmap_difference(bitmap b1, bitmap b2);

bool bitmap_equal(bitmap b1, bitmap b2);
bool bitmap_disjoint(bitmap b1, bitmap b2);

void bitmap_print(FILE *out, bitmap b);

#endif