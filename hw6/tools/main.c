#include "util.h"
#include "fdmjast.h"
#include "print_ast.h"
#include "print_src.h"
#include "lxml.h"
#include "xml2ast.h"
#include "semant.h"

int main(int argc, char *argv[])
{
  if (argc<2) {
    fprintf(stderr, "Usage: %s ASTxmlfilename\n", argv[0]);
    return 0;
  }

  XMLDocument doc;
  A_prog p;

  if (XMLDocument_load(&doc, argv[1])) {
	  printf("----The fmj source----\n");
    printA_Prog(stdout, p=xmlprog(XMLNode_child(doc.root, 0)));
	  // printf("----the AST ----\n");
    // printX_Prog(stdout, xmlprog(XMLNode_child(doc.root, 0)));
    transA_Prog(stderr, p);
  }

  return 0;
}
