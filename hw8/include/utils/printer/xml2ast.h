#ifndef XML2AST_H
#define XML2AST_H

#include "lxml.h"
#include "fdmjast.h"
#include "xml2ast.h"

A_prog xml2ast(XMLDocument* doc);
A_prog xmlprog(XMLNode* node);

#endif