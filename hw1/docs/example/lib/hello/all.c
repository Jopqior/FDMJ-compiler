#include <stdio.h>
#include "all.h"
#include "a.h"
#include "b.h"
#include "c.h"
#include "d.h"
#include "e.h"
#include "f.h"
#include "g.h"

void hello_all() {
  printf("--- hello from ");
  hello_a();
  hello_b();
  hello_c();
  hello_d();
  hello_e();
  hello_f();
  hello_g();
  printf(" ---\n");
}