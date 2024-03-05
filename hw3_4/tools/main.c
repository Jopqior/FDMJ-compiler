#include <stdio.h>
#include <string.h>
#include "fdmjast.h"
#include "print_src.h"
#include "print_ast.h"
#include "util.h"
#include "parser.h"

A_prog root;

extern int yyparse();

int main(int argc, const char * argv[]) {
  if (argc != 2) {
    fprintf(stdout, "Usage: %s filename\n", argv[0]);
    return 1;
  }

  // output filename
  string file = checked_malloc(IR_MAXLEN);
  sprintf(file, "%s", argv[1]);
  string file_src = checked_malloc(IR_MAXLEN);
  sprintf(file_src, "%s.1.src", file);
  string file_ast = checked_malloc(IR_MAXLEN);
  sprintf(file_ast, "%s.2.ast", file);

  // lex & parse
  yyparse();
  assert(root);

  // ast2src
  freopen(file_src, "w", stdout);
  printA_Prog(stdout, root);
  fflush(stdout);
  fclose(stdout);

  // ast2xml
  freopen(file_ast, "w", stdout);
  printX_Prog(stdout, root);
  fflush(stdout);
  fclose(stdout);

  return 0;
}
