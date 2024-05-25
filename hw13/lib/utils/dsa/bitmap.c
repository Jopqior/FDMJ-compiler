#include "bitmap.h"

#include <stdlib.h>

#define bitmap_type unsigned int
#define bitmap_shift 5
#define bitmap_mask 31
#define bitmap_wordlength 32
#define bitmap_fmt "%08x"

#define bitmap_one (bitmap_type)1

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

bitmap Bitmap(int bits) {
  bitmap b = checked_malloc(sizeof *b);
  b->bits = bits;
  b->words = (bits + bitmap_wordlength - 1) / bitmap_wordlength;
  b->array = checked_calloc(b->words, sizeof(bitmap_type));
  return b;
}

void bitmap_set(bitmap b, int n) {
  int word = n >> bitmap_shift;    // n / bitmap_wordlength
  int position = n & bitmap_mask;  // n % bitmap_wordlength
  b->array[word] |= bitmap_one << position;
}

void bitmap_clear(bitmap b, int n) {
  int word = n >> bitmap_shift;    // n / bitmap_wordlength
  int position = n & bitmap_mask;  // n % bitmap_wordlength
  b->array[word] &= ~(bitmap_one << position);
}

int bitmap_read(bitmap b, int n) {
  int word = n >> bitmap_shift;    // n / bitmap_wordlength
  int position = n & bitmap_mask;  // n % bitmap_wordlength
  return (b->array[word] >> position) & 1;
}

int bitmap_get_first(bitmap b) {
  for (int i = 0; i < b->words; i++) {
    if (b->array[i]) {
      for (int j = 0; j < bitmap_wordlength; j++) {
        if ((b->array[i] >> j) & 1) {
          return i * bitmap_wordlength + j;
        }
      }
    }
  }
  return -1;
}

void bitmap_set_all(bitmap b) {
  for (int i = 0; i < b->words; i++) {
    b->array[i] = ~0;
  }
}

void bitmap_clear_all(bitmap b) {
  for (int i = 0; i < b->words; i++) {
    b->array[i] = 0;
  }
}

void bitmap_copy(bitmap dst, bitmap src) {
  if (dst->bits != src->bits) {
    fprintf(stderr, "bitmap_copy: bitmaps have different sizes\n");
    exit(1);
  }

  for (int i = 0; i < dst->words; i++) {
    dst->array[i] = src->array[i];
  }
}

void bitmap_union_into(bitmap dst, bitmap src) {
  if (dst->bits != src->bits) {
    fprintf(stderr, "bitmap_union_into: bitmaps have different sizes\n");
    exit(1);
  }

  for (int i = 0; i < dst->words; i++) {
    dst->array[i] |= src->array[i];
  }
}

void bitmap_intersection_into(bitmap dst, bitmap src) {
  if (dst->bits != src->bits) {
    fprintf(stderr, "bitmap_intersection_into: bitmaps have different sizes\n");
    exit(1);
  }

  for (int i = 0; i < dst->words; i++) {
    dst->array[i] &= src->array[i];
  }
}

bitmap bitmap_union(bitmap b1, bitmap b2) {
  if (b1->bits != b2->bits) {
    fprintf(stderr, "bitmap_union: bitmaps have different sizes\n");
    exit(1);
  }

  bitmap b = Bitmap(b1->bits);
  for (int i = 0; i < b->words; i++) {
    b->array[i] = b1->array[i] | b2->array[i];
  }
  return b;
}

bitmap bitmap_intersection(bitmap b1, bitmap b2) {
  if (b1->bits != b2->bits) {
    fprintf(stderr, "bitmap_intersection: bitmaps have different sizes\n");
    exit(1);
  }

  bitmap b = Bitmap(b1->bits);
  for (int i = 0; i < b->words; i++) {
    b->array[i] = b1->array[i] & b2->array[i];
  }
  return b;
}

bitmap bitmap_difference(bitmap b1, bitmap b2) {
  if (b1->bits != b2->bits) {
    fprintf(stderr, "bitmap_difference: bitmaps have different sizes\n");
    exit(1);
  }

  bitmap b = Bitmap(b1->bits);
  for (int i = 0; i < b->words; i++) {
    b->array[i] = b1->array[i] & ~b2->array[i];
  }
  return b;
}

bool bitmap_equal(bitmap b1, bitmap b2) {
  if (b1->bits != b2->bits) {
    fprintf(stderr, "bitmap_equal: bitmaps have different sizes\n");
    exit(1);
  }

  for (int i = 0; i < b1->words; i++) {
    if (b1->array[i] != b2->array[i]) {
      return FALSE;
    }
  }
  return TRUE;
}

bool bitmap_disjoint(bitmap b1, bitmap b2) {
  if (b1->bits != b2->bits) {
    fprintf(stderr, "bitmap_disjoint: bitmaps have different sizes\n");
    exit(1);
  }

  for (int i = 0; i < b1->words; i++) {
    if (b1->array[i] & b2->array[i]) {
      return FALSE;
    }
  }
  return TRUE;
}

bool bitmap_empty(bitmap b) {
  for (int i = 0; i < b->words; i++) {
    if (b->array[i]) {
      return FALSE;
    }
  }
  return TRUE;
}

void bitmap_print(FILE *out, bitmap b) {
  for (int i = 0; i < b->bits; i++) {
    if (bitmap_read(b, i)) {
      fprintf(out, "%d", i);
      if (i < b->bits - 1) {
        fprintf(out, " ");
      }
    }
  }
  fprintf(out, "\n");
}