#include <stdio.h>
#include <string.h>
#include "fdmjast.h"
#include "xml2ast.h"
#include "semant.h"
#include "util.h"

A_prog root;

int main(int argc, const char * argv[]) {
  if (argc != 2) {
    fprintf(stdout, "Usage: %s ASTXMLFilename\n", argv[0]);
    return 1;
  }

  // output filename
  string file = checked_malloc(IR_MAXLEN);
  sprintf(file, "%s", argv[1]);
  string file_fmj = checked_malloc(IR_MAXLEN);
  sprintf(file_fmj, "%s.fmj", file);
  string file_out = checked_malloc(IR_MAXLEN);
  sprintf(file_out, "%s.out", file);
  string file_src = checked_malloc(IR_MAXLEN);
  sprintf(file_src, "%s.1.src", file);
  string file_ast = checked_malloc(IR_MAXLEN);
  sprintf(file_ast, "%s.2.ast", file);
  string file_irp = checked_malloc(IR_MAXLEN);
  sprintf(file_irp, "%s.3.irp", file);
  string file_stm = checked_malloc(IR_MAXLEN);
  sprintf(file_stm, "%s.4.stm", file);
  string file_liv = checked_malloc(IR_MAXLEN);
  sprintf(file_liv, "%s.5.liv", file);

  // parse xml to ast
  XMLDocument doc;
  if (XMLDocument_load(&doc, file_ast))
    root = xmlprog(XMLNode_child(doc.root, 0));
  assert(root);

  // type checking & translate
  // freopen(file_out, "w", stdout);
  transA_Prog(stderr, root);
  // fflush(stdout);
  // fclose(stdout);

  return 0;
}
