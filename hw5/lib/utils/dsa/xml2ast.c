#include "util.h"
#include "fdmjast.h"
#include "lxml.h"
#include "xml2ast.h"
#include <stdio.h>
#include <strings.h>
#include <assert.h>

void node_out(FILE* file, XMLNode* node, int indent, int times);

string onlyid(string s) {
    char *ss = String(s);
    int i = 0, j=0;
    while (s[i] != '\0') {
        if ( (s[i] >= 'a' && s[i] <= 'z') || (s[i] >= 'A' && s[i] <= 'Z') || 
              s[i] == '_' || (s[i]>= '0' && s[i] <= '9') || s[i] == ' ' ||
              s[i] == '[' || s[i] == ']' || s[i] == '.' ) {
            ss[j] = s[i]; j++;
        }
        i++;
    }
    ss[j] = '\0';
    return String(ss);
}

XMLNode* xmlgetchildnode(XMLNode* node, string tag) {
    if (!node || node->children.size ==0) return NULL;
    //fprintf(stderr, "looking for: xmlgetchildnode: %s\n", tag);
    XMLNodeList* list = XMLNode_children(node, tag);
    if (list->size == 0) return NULL; //empty list (list not holding any data)
    //fprintf(stderr, "Got it: xmlgetchildnode: %s, innert:%s\n", onlyid(list->data[0]->tag), onlyid(list->data[0]->inner_text));
    return  XMLNodeList_at(list, 0);
}

A_pos xmlpos(XMLNode* node) {
    assert(node && node->children.size >0);
    XMLNode *line = xmlgetchildnode(node, "line");
    XMLNode *col= xmlgetchildnode(node, "col");
    assert(line && col);
    //fprintf(stderr, "xmlpos: line=%s col=%s\n", line->inner_text, col->inner_text);
    return A_Pos(atoi(onlyid(line->inner_text)), atoi(onlyid(col->inner_text)));
}

A_type xmltype(XMLNode* pos, string s) {
    A_pos p = xmlpos(pos);

    string t = onlyid(s);

    if (!strncmp(t, "int[]", 5)) 
        return A_Type(p, A_intArrType, NULL);
    else if (!strncmp(t, "int", 3)) {
        return A_Type(p, A_intType, NULL);
    }
    else if (!strncmp(t, "float[]", 7)) 
        return A_Type(p, A_floatArrType, NULL);
    else if (!strncmp(t, "float", 5)) 
        return A_Type(p,  A_floatType, NULL);
    else if (!strncmp(t, "class ", 6))  //a class id type is of the form: "class id"
        return A_Type(p, A_idType, String(t + 6));
    else 
        assert(0); 

    return NULL;
}

A_varDecl xmlvardecl(XMLNode* varDecl) {
    assert(varDecl);
    XMLNode *pos = xmlgetchildnode(varDecl, "pos");
    assert(pos);
    XMLNode *t = xmlgetchildnode(varDecl, "Type");
    XMLNode *i = xmlgetchildnode(varDecl, "id");
    assert(t && i);
    XMLNode *v = xmlgetchildnode(varDecl, "initValue");
    XMLNode *vl = xmlgetchildnode(varDecl, "initValueList");
    if (v) {
        XMLNode *num = xmlgetchildnode(v, "num");
        assert(num);
        A_exp e = xmlexp(num);
        return A_VarDecl(xmlpos(pos), xmltype(pos, onlyid(t->inner_text)), String(onlyid(i->inner_text)), A_ExpList(e, NULL));
    } else if (vl) {
        XMLNode *l = xmlgetchildnode(vl, "expList");
        assert(l);
        A_expList el = xmlexplist(l);
        return A_VarDecl(xmlpos(pos), xmltype(pos, onlyid(t->inner_text)), String(onlyid(i->inner_text)), el);
    } else {
        return A_VarDecl(xmlpos(pos), xmltype(pos, onlyid(t->inner_text)), String(onlyid(i->inner_text)), NULL);
    }
    return NULL;
}

A_varDeclList xmlvardecllist(XMLNode* varDeclList) {
    if ( !varDeclList || varDeclList->children.size == 0) return NULL;
    XMLNode *v = xmlgetchildnode(varDeclList, "varDecl");
    XMLNode *l = xmlgetchildnode(varDeclList, "varDeclList");
    assert(v);
    return A_VarDeclList(xmlvardecl(v), xmlvardecllist(l));
}

A_exp xmlexp(XMLNode* exp) {
    assert(exp && exp->children.size > 0);

    string tag = onlyid(exp->tag);
    //fprintf(stderr, "DOING THIS EXP: xmlexp: tag=%s\n", tag);

    if (!strcmp(tag, "opExp")) {
        // code for opExp
        XMLNode *pos = xmlgetchildnode(exp, "pos"); // position
        assert(pos);
        XMLNode *left = xmlgetchildnode(exp, "left"); // left expression
        assert(left);
        XMLNode *oper = xmlgetchildnode(exp, "op"); // operator
        assert(oper);
        XMLNode *right = xmlgetchildnode(exp, "right"); // right expression
        assert(right);

        A_binop op;
        string ss = onlyid(oper->inner_text);
        if (!strcmp(ss, "AND")) op = A_and;
        else if (!strcmp(ss, "OR")) op = A_or;
        else if (!strcmp(ss, "LESS THAN")) op = A_less;
        else if (!strcmp(ss, "LESS THAN OR EQUAL TO")) op = A_le;
        else if (!strcmp(ss, "GREATER THAN")) op = A_greater;
        else if (!strcmp(ss, "GREATER THAN OR EQUAL TO")) op = A_ge;
        else if (!strcmp(ss, "EQUAL TO")) op = A_eq;
        else if (!strcmp(ss, "NOT EQUAL TO")) op = A_ne;
        else if (!strcmp(ss, "PLUS")) op = A_plus;
        else if (!strcmp(ss, "MINUS")) op = A_minus;
        else if (!strcmp(ss, "TIMES")) op = A_times;
        else if (!strcmp(ss, "DIVIDES")) op = A_div;
        else assert(0);

        //the exp nodes are one level under left and right 
        return A_OpExp(xmlpos(pos), xmlexp(left->children.data[0]), op, xmlexp(right->children.data[0])); 

    } else if (!strcmp(tag, "arrExp")) {
        // code for arrExp *** special case: there are two pos elements....
        XMLNode *pos = xmlgetchildnode(exp, "pos"); // position
        assert(pos);
        XMLNode *a = xmlgetchildnode(exp, "arr"); // array
        assert(a);
        XMLNode *p = xmlgetchildnode(exp, "index"); // array position
        assert(p);
        return A_ArrayExp(xmlpos(pos), xmlexp(a->children.data[0]), xmlexp(p->children.data[0]));
    } else if (!strcmp(tag, "callExp")) {
        // code for callExp
        XMLNode *pos = xmlgetchildnode(exp, "pos"); // position
        assert(pos);
        XMLNode *o = xmlgetchildnode(exp, "obj"); // object
        assert(o);
        XMLNode *f = xmlgetchildnode(exp, "fun"); // function
        assert(f);
        XMLNode *p = xmlgetchildnode(exp, "par"); // arguments
        assert(p);
        return A_CallExp(xmlpos(pos), xmlexp(o->children.data[0]), String(onlyid(f->inner_text)), xmlexplist(p->children.data[0]));
    } else if (!strcmp(tag, "classVarExp")) {
        // code for classVarExp
        XMLNode *pos = xmlgetchildnode(exp, "pos"); // position
        assert(pos);
        XMLNode *o  = xmlgetchildnode(exp, "obj"); // variable
        assert(o && o->children.size>0);
        XMLNode *v = xmlgetchildnode(exp, "var"); // variable
        assert(v);
        return A_ClassVarExp(xmlpos(pos), xmlexp(o->children.data[0]), String(onlyid(v->inner_text)));
    } else if (!strcmp(tag, "true")) {
        // code for trueExp
        XMLNode *pos = xmlgetchildnode(exp, "pos"); // position
        assert(pos);
        return A_BoolConst(xmlpos(pos), TRUE);
    } else if (!strcmp(tag, "false")) {
        // code for falseExp
        XMLNode *pos = xmlgetchildnode(exp, "pos"); // position
        assert(pos);
        return A_BoolConst(xmlpos(pos), FALSE);
    } else if (!strcmp(tag, "num")) {
        // code for numExp
        XMLNode *pos = xmlgetchildnode(exp, "pos"); // position
        assert(pos);
        XMLNode *v = xmlgetchildnode(exp, "value"); // number
        assert(v);
        return A_NumConst(xmlpos(pos), atof(onlyid(v->inner_text)));
    } else if (!strcmp(tag, "lengthOf")) {
        // code for lengthOfExp
        XMLNode *pos = xmlgetchildnode(exp, "pos"); // position
        assert(pos);
        assert(exp->children.size == 2);
        XMLNode *e = exp->children.data[1]; //expression
        return A_LengthExp(xmlpos(pos), xmlexp(e));
    } else if (!strcmp(tag, "id")) {
        // code for idExp
        XMLNode *pos = xmlgetchildnode(exp, "pos"); // position
        assert(pos);
        XMLNode *id = xmlgetchildnode(exp, "string"); // id
        assert(id);
        return A_IdExp(xmlpos(pos), String(onlyid(id->inner_text)));
    } else if (!strcmp(tag, "this")) {
        XMLNode *pos = xmlgetchildnode(exp, "pos"); // position
        assert(pos);
        return A_ThisExp(xmlpos(pos));
    } else if (!strcmp(tag, "newInt")) {
        // code for newIntExp
        XMLNode *pos = xmlgetchildnode(exp, "pos"); // position
        assert(pos);
        assert(exp->children.size == 2);
        XMLNode *e = XMLNode_child(exp, 1); //expression
        return A_NewIntArrExp(xmlpos(pos), xmlexp(e));
    } else if (!strcmp(tag, "newFloat")) {
        // code for newFloatExp
        XMLNode *pos = xmlgetchildnode(exp, "pos"); // position
        assert(pos);
        assert(exp->children.size == 2);
        XMLNode *e = XMLNode_child(exp, 1); //expression
        return A_NewFloatArrExp(xmlpos(pos), xmlexp(e));
    } else if (!strcmp(tag, "newObj")) {
        XMLNode *pos = xmlgetchildnode(exp, "pos"); // position
        assert(pos);
        XMLNode *e = xmlgetchildnode(exp, "id"); // id
        assert(e);
        return A_NewObjExp(xmlpos(pos), String(onlyid(e->inner_text)));
    } else if (!strcmp(tag, "notExp")) {
        // code for notExp
        XMLNode *pos = xmlgetchildnode(exp, "pos"); // position
        assert(pos && exp->children.size == 2);
        XMLNode *e = XMLNode_child(exp, 1); //expression
        return A_NotExp(xmlpos(pos), xmlexp(e));
    } else if (!strcmp(tag, "minusExp")) {
        XMLNode *pos = xmlgetchildnode(exp, "pos"); // position
        assert(pos);
        assert(exp->children.size == 2);
        XMLNode *e = XMLNode_child(exp, 1); //expression
        return A_MinusExp(xmlpos(pos), xmlexp(e));
    } else if (!strcmp(tag, "escExp")) {
        XMLNode *pos = xmlgetchildnode(exp, "pos"); // position
        assert(pos);
        XMLNode *l = xmlgetchildnode(exp, "stmList"); //statement list
        A_stmList sl=NULL;
        if (l) sl = xmlstmlist(l); 
        XMLNode *e;
        if (l) e = XMLNode_child(exp, 2); //expression
        else e = XMLNode_child(exp, 1); //expression
        return A_EscExp(xmlpos(pos), sl, xmlexp(e));
    } else if (!strcmp(tag, "getnum")) {
        XMLNode *pos = xmlgetchildnode(exp, "pos"); // position
        assert(pos);
        return A_Getnum(xmlpos(pos));
    } else if (!strcmp(tag, "getch")) {
        XMLNode *pos = xmlgetchildnode(exp, "pos"); // position
        assert(pos);
        return A_Getch(xmlpos(pos));
    } else if (!strcmp(tag, "getarray")) {
        XMLNode *pos = xmlgetchildnode(exp, "pos"); // position
        assert(pos);
        assert(exp->children.size == 2);
        XMLNode *a = XMLNode_child(exp, 1); //get the expression
        return A_Getarray(xmlpos(pos), xmlexp(a));
    } else {
        assert(0);
    }      
    return NULL;
}

A_expList xmlexplist(XMLNode* expList) {
    if (!expList || expList->children.size == 0) return NULL;
    XMLNode *e = XMLNode_child(expList, 0);
    XMLNode *l = xmlgetchildnode(expList, "expList");
    assert(e);
    return A_ExpList(xmlexp(e), xmlexplist(l));
}

A_stm xmlstm(XMLNode* stm) {
    assert(stm);
    string tag = onlyid(stm->tag);

    //fprintf(stderr, "DOING THIS STM: xmlstm: tag=%s\n", tag);
    XMLNode *pos;

    if (!strcmp(tag, "nestedStm")) {
        XMLNode *pos = XMLNode_child(stm, 0); // position
        assert(pos);
        XMLNode *l = xmlgetchildnode(stm, "stmList");
        return A_NestedStm(xmlpos(pos), xmlstmlist(l));
    } else if (!strcmp(tag, "if")) {
        pos = xmlgetchildnode(stm, "pos"); 
        assert(pos);
        assert(stm->children.size >= 2);
        XMLNode *c = XMLNode_child(stm, 1); //first child must be a condition expression node
        XMLNode *t = xmlgetchildnode(stm, "then"); //find the then part, it must be there
        assert(t);
        XMLNode *e = xmlgetchildnode(stm, "else"); //may not have an else part
        if (e) return A_IfStm(xmlpos(pos), xmlexp(c), xmlstm(t->children.data[0]), xmlstm(e->children.data[0]));
        else return A_IfStm(xmlpos(pos), xmlexp(c), xmlstm(t->children.data[0]), NULL);
    } else if (!strcmp(tag, "while")) {
        XMLNode *pos = XMLNode_child(stm, 0); // position 
        assert(pos);
        XMLNode *c = xmlgetchildnode(stm, "cond"); //get the condition  
        assert(c);
        XMLNode *b;
        if (stm->children.size > 2) {
            b = XMLNode_child(stm, 2); //get the body
            assert(b);
            // condition expression is under "cond" node
            return A_WhileStm(xmlpos(pos), xmlexp(c->children.data[0]), xmlstm(b));
        } else 
            return A_WhileStm(xmlpos(pos), xmlexp(c->children.data[0]), NULL);
    } else if (!strcmp(tag, "assign")) {
        XMLNode *left = xmlgetchildnode(stm, "left");
        XMLNode *right = xmlgetchildnode(stm, "right");
        assert(left && right);
        XMLNode *lefte = left->children.data[0];
        XMLNode *righte = right->children.data[0];
        assert(lefte && righte);
        XMLNode *pos = xmlgetchildnode(lefte, "pos"); //position of assignment is that of the left expression
        assert(pos); 
        return A_AssignStm(xmlpos(pos), xmlexp(lefte), xmlexp(righte));
    } else if (!strcmp(tag, "arrayInit")) {
        XMLNode *pos = XMLNode_child(stm, 0); // position
        assert(pos);
        XMLNode *left = xmlgetchildnode(stm, "left"); //expression for array address
        XMLNode *right = xmlgetchildnode(stm, "right"); //express list for a list of values
        assert(left && right);
        return A_ArrayInit(xmlpos(pos), xmlexp(left), xmlexplist(right));
    } else if (!strcmp(tag, "call")) {
        XMLNode *pos = XMLNode_child(stm, 0); // position
        assert(pos);
        XMLNode *o = xmlgetchildnode(stm, "obj");
        XMLNode *f = xmlgetchildnode(stm, "fun");
        XMLNode *a = xmlgetchildnode(stm, "args");
        assert(o && f );
        if (a) return A_CallStm(xmlpos(pos), xmlexp(o->children.data[0]), 
                            String(onlyid(f->inner_text)), xmlexplist(a->children.data[0]));
        else return A_CallStm(xmlpos(pos), xmlexp(o->children.data[0]), 
                            String(onlyid(f->inner_text)), NULL);
    } else if (!strcmp(tag, "continue")) {
        XMLNode *pos = XMLNode_child(stm, 0); // position
        assert(pos);
        return A_Continue(xmlpos(pos));
    } else if (!strcmp(tag, "break")) {
        XMLNode *pos = XMLNode_child(stm, 0); // position
        assert(pos);
        return A_Break(xmlpos(pos));
    } else if (!strcmp(tag, "return")) {
        XMLNode *pos = XMLNode_child(stm, 0); // position
        assert(pos);
        if (stm->children.size < 2)
            return A_Return(xmlpos(pos), NULL); 
        XMLNode *e = XMLNode_child(stm, 1); //get the return expression
        assert(e);
        return A_Return(xmlpos(pos), xmlexp(e));
    } else if (!strcmp(tag, "putnum")) {
        XMLNode *pos = XMLNode_child(stm, 0); // position
        assert(pos);
        assert(stm->children.size == 2);
        XMLNode *e = XMLNode_child(stm, 1); //get the expression
        return A_Putnum(xmlpos(pos), xmlexp(e));
    } else if (!strcmp(tag, "putarray")) {
        XMLNode *pos = XMLNode_child(stm, 0); // position
        assert(pos);
        XMLNode *a = xmlgetchildnode(stm, "arr");
        assert(a && a->children.size == 1);
        XMLNode *l = xmlgetchildnode(stm, "length");
        assert(l && l->children.size == 1); 
        return A_Putarray(xmlpos(pos), xmlexp(a->children.data[0]), xmlexp(l->children.data[0]));
    } else if (!strcmp(tag, "putch")) {
        XMLNode *pos = xmlgetchildnode(stm, "pos"); // position
        assert(pos);
        assert(stm->children.size == 2);
        XMLNode *c = XMLNode_child(stm, 1);
        return A_Putch(xmlpos(pos), xmlexp(c)); 
    } else if (!strcmp(tag, "starttime")) {
        XMLNode *pos = XMLNode_child(stm, 0); // position
        assert(pos);
        return A_Starttime(xmlpos(pos));
    } else if (!strcmp(tag, "stoptime")) {
        XMLNode *pos = XMLNode_child(stm, 0); // position
        assert(pos);
        return A_Stoptime(xmlpos(pos));
    } else {
        assert(0);
    }   
    return NULL;
}

A_stmList xmlstmlist(XMLNode* stmList) {
    if (!stmList || stmList->children.size == 0) return NULL;
    XMLNode *s = XMLNode_child(stmList, 0);
    XMLNode *l = xmlgetchildnode(stmList, "stmList");
    return A_StmList(xmlstm(s), xmlstmlist(l));   
}


A_formal xmlformal(XMLNode* node) {
    assert(node && !strcmp(node->tag, "formal"));
    XMLNode *pos = xmlgetchildnode(node, "pos");
    XMLNode *t = xmlgetchildnode(node, "type");
    XMLNode *id = xmlgetchildnode(node, "id");
    assert(pos && t && id);
    return A_Formal(xmlpos(pos), xmltype(pos, String(onlyid(t->inner_text))), String(onlyid(id->inner_text)));
}

A_formalList xmlformallist(XMLNode* node) {
    if (!node || node->children.size == 0) return NULL;
    assert(!strcmp(node->tag, "formalList"));
    XMLNode *f = xmlgetchildnode(node, "formal");
    XMLNode *l = xmlgetchildnode(node, "formalList");
    assert(f);
    return A_FormalList(xmlformal(f), xmlformallist(l));
}   

A_methodDecl xmlmethoddecl(XMLNode* node) {
    assert(node);
    XMLNode *pos = xmlgetchildnode(node, "pos");
    XMLNode *t = xmlgetchildnode(node, "type");
    XMLNode *n = xmlgetchildnode(node, "name");
    assert(pos && t && n);

    XMLNode *child;
    XMLNode *f = NULL, *vl = NULL, *sl = NULL;
    for (int i = 3; i < node->children.size; i++) { //look for formal list, var decl list, and statement list
        child = node->children.data[i];
        if (!strcmp(String(onlyid(child->tag)), String("formalList"))) {
            f = child;
        } else if (strcmp(String(onlyid(child->tag)), String("varDeclList")) == 0) {
            vl = child;
        } else if (strcmp(String(onlyid(child->tag)), String("stmList")) == 0) {
            sl = child;
        } else {
            assert(0);
        }
    }
    return A_MethodDecl(xmlpos(pos), xmltype(pos, String(onlyid(t->inner_text))), 
            String(onlyid(n->inner_text)), xmlformallist(f), xmlvardecllist(vl), xmlstmlist(sl));
}

A_methodDeclList xmlmethoddecllist(XMLNode* node) {
    if (!node || node->children.size ==0) return NULL;
    XMLNode *m = xmlgetchildnode(node, "methodDecl");
    XMLNode *l = xmlgetchildnode(node, "methodDeclList");
    assert(m);
    return A_MethodDeclList(xmlmethoddecl(m), xmlmethoddecllist(l));
}

A_mainMethod xmlmainmethod(XMLNode* node) { 
    assert(node);
    XMLNode *pos = xmlgetchildnode(node, "pos");
    XMLNode *v = xmlgetchildnode(node, "varDeclList");
    XMLNode *l = xmlgetchildnode(node, "stmList");
    return A_MainMethod(xmlpos(pos), xmlvardecllist(v), xmlstmlist(l));
}

A_classDecl xmlclassdecl(XMLNode* node) {
    string pname=NULL;
    assert(node);
    XMLNode *pos = xmlgetchildnode(node, "pos");
    assert(pos);
    XMLNode *id = xmlgetchildnode(node, "className");
    assert(id);
    XMLNode *pid = xmlgetchildnode(node, "extends");
    if (pid) pname = String(onlyid(pid->inner_text));
    XMLNode *vdl = xmlgetchildnode(node, "varDeclList");
    XMLNode *mdl = xmlgetchildnode(node, "methodDeclList");
    return A_ClassDecl(xmlpos(pos), String(onlyid(id->inner_text)), pname, xmlvardecllist(vdl), xmlmethoddecllist(mdl));
}

A_classDeclList xmlclassdecllist(XMLNode* node) {
    if (!node || node->children.size == 0) return NULL;
    XMLNode *c = xmlgetchildnode(node, "classDecl");
    XMLNode *l = xmlgetchildnode(node, "classDeclList");
    assert(c);
    return A_ClassDeclList(xmlclassdecl(c), xmlclassdecllist(l));
}

//Build from a program node in the XML
A_prog xmlprog(XMLNode* root) {
    assert(root || !strcmp(root->tag, "Program"));
    XMLNode *pos = xmlgetchildnode(root, "pos");
    assert(pos);
    XMLNode *main = xmlgetchildnode(root, "main");
    assert(main);
    XMLNode *cdl = xmlgetchildnode(root, String("classDeclList"));
    return A_Prog(xmlpos(pos), xmlmainmethod(main), xmlclassdecllist(cdl));
}


