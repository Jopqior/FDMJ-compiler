#include <stdio.h>
#include <string.h>
#include "fdmjslpast.h"
#include "slpast_fdmjprinter.h"
#include "slpast_xmlprinter.h"
#include "llvmgen.h"
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
  string file_ll = checked_malloc(IR_MAXLEN);
  sprintf(file_ll, "%s.ll", argv[1]);
  string file_ast = checked_malloc(IR_MAXLEN);
  sprintf(file_ast, "%s.xml", argv[1]);

  // lex & parse
  
  yyparse();
  assert(root);

  // print ast
  freopen(file_ast, "w", stdout);
  fprintf(stdout, "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>");
  printX_Prog(stdout, root);
  fprintf(stdout, "\n\n");
  fflush(stdout);
  fclose(stdout);

  // go through the ast to generate .ll file
  
  freopen(file_ll, "w", stdout);
  fprintf(stdout, "define i64 @main {\n");
  AS_generateLLVMcode(stdout, root);
  fclose(stdout);

  //add the external library
  freopen(file_ll, "a", stdout);
  fprintf(stdout, "}\n");
  fprintf(stdout, "declare ptr @malloc(i64)\n");
  fprintf(stdout, "declare void @putint(i64)\n");
  fprintf(stdout, "declare void @putch(i64)\n");
  fclose(stdout);

  return 0;
}
