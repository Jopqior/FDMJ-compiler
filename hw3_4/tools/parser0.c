#include <stdio.h>
#include <string.h>
#include "fdmjast.h"
#include "printast.h"
#include "printastXML.h"
#include "util.h"
#include "y.tab.h"

int fdmj_or_xml = 0; //if 0 print ast into fdmj, if 1 print ast into xml

A_prog root;

extern int yyparse();

int main(int argc, const char * argv[]) {
  if (argc != 2) {
    fprintf(stdout, "Usage: %s filename\n", argv[0]);
    return 1;
  }

  // output filename
  string file_ll = checked_malloc(IR_MAXLEN);
  sprintf(file_ll, "%s", argv[1]);
  string file_ast = checked_malloc(IR_MAXLEN);
  sprintf(file_ast, "%s.1.ast", file_ll);
  string file_irp = checked_malloc(IR_MAXLEN);
  sprintf(file_irp, "%s.2.irp", file_ll);
  string file_stm = checked_malloc(IR_MAXLEN);
  sprintf(file_stm, "%s.3.stm", file_ll);
  string file_liv = checked_malloc(IR_MAXLEN);
  sprintf(file_liv, "%s.4.liv", file_ll);

  // lex & parse
  yyparse();
  assert(root);
  freopen(file_ast, "a", stdout);
  
  if (fdmj_or_xml == 0) {
    fprintf(stdout, "------Source fmj file------\n");
    printA_Prog(stdout, root);
    fprintf(stdout, "\n\n");
  } else {
    fprintf(stdout, "<?xml version=\"1.0\"?>\n");
    fprintf(stdout, "<Program>\n");
    printX_Prog(stdout, root);
    fprintf(stdout, "</Program>\n");
  }
  fflush(stdout);
  fclose(stdout);
  return 0;
}

