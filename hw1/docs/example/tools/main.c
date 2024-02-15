#include "ast.h"
#include "all.h"
#include "parser.h"

int main() {
  hello_all();
  fake_ast();
  return(yyparse());
}
