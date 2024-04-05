#include <stdio.h>
#include <string.h>
#include "fdmjast.h"
#include "print_irp.h"
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
  T_funcDeclList fdl = transA_Prog(stderr, root, 8);
  // fflush(stdout);
  // fclose(stdout);

  while (fdl) {
    T_stm s = fdl->head->stm;
    freopen(file_irp, "a", stdout);
    fprintf(stdout, "------Original IR Tree------\n");
    printIRP_set(IRP_parentheses);
    printIRP_FuncDecl(stdout, fdl->head);
    fprintf(stdout, "\n\n");
    fflush(stdout);
    fclose(stdout);

    fdl = fdl->tail;
  }

  return 0;
}
