#ifndef XML2AST_H
#define XML2AST_H

#include "lxml.h"
#include "fdmjast.h"
#include "xml2ast.h"

A_type xmltype(XMLNode* pos, string s);

A_prog xml2ast(XMLDocument* doc);
A_prog xmlprog(XMLNode* node);

A_mainMethod xmlmainmethod(XMLNode* node);

A_varDecl xmlvardec(XMLNode* node);
A_varDeclList xmlvardecllist(XMLNode* l);

A_classDecl xmlclassdecl(XMLNode* node);
A_classDeclList xmlclassdecllist(XMLNode* node);

A_methodDecl xmlmethoddecl(XMLNode* node);
A_methodDeclList xmlmethoddecllist(XMLNode* l);

A_stmList xmlstmlist(XMLNode* l);
A_stm xmlstm(XMLNode* stm);

A_expList xmlexplist(XMLNode* l);
A_exp xmlexp(XMLNode* exp);
#endif