# Homework Assignment #2

You need to write the lex/yacc files for a parser (called parser0) for the following grammar (which is a simply part of FDMJ 2024). Your parser is to generate an AST structure (using the ast given), and print out the XML representation of the AST structure (or the FDMJ source file back).

The second homework also includes a written part (see elearning). You should scan your written solutions and put into the report.pdf (under your doc directory), and use "make handin" to wrap everything into a zip file to submit on elearning.

Grammar:

```
Prog -> MainMethod ClassDeclList
  MainMethod -> public int main '(' ')' '{' StmList '}'
  StmList -> ε | Stm StmList
  Stm -> '{' StmList '}' 
  ClassDeclList -> ε | ClassDecl ClassDeclList
  ClassDecl -> public class id '{' MethodDeclList '}' 
             | public class id extends id '{' MethodDeclList '}'
  MethodDeclList -> ε | MethodDecl MethodDeclList
  MethodDecl -> public int id '(' ')' '{' StmList '}'
```
Note that in the above, id is a string of characters (as defined in the FDMJ2024-SLP in HW1) and `ε` is the empty string. Every other terminal is a "key word" for the language.

The AST definition is given in the "include/utils/dsa/fdmjast.h". You simply use the needed node types (for MainMethod, StmList, Stm, ClassDeclList, ClassDecl, MethodDeclList, and MethodDecl) from the file and ignore the rest for now (which will be used in the next homework assignment). 

