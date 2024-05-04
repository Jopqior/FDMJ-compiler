#include "util.h"
#include "fdmjast.h"
#include "lxml.h"
#include <stdio.h>
#include <strings.h>
#include <assert.h>
#include "xml2irp.h"

#define __DEBUG
#undef __DEBUG

static T_stm xmlirpstm(XMLNode* stm);
static T_expList xmlirpexplist(XMLNode *l);
static T_exp xmlirpexp(XMLNode* exp);
static T_stm xmlirpstm(XMLNode* stm);
static T_stmList xmlirpstmlist(XMLNode *l);
static T_funcDecl xmlirpfunc(XMLNode *func);
static T_stmList xmlirpstmlist(XMLNode *l);
static Temp_tempList xmlirptemplist(XMLNode *l);

static string onlyid(string s) {
    char *ss = String(s);
    int i = 0, j=0;
    while (s[i] != '\0') {
        if ( (s[i] >= 'a' && s[i] <= 'z') || (s[i] >= 'A' && s[i] <= 'Z') || 
              s[i] == '_' || (s[i]>= '0' && s[i] <= '9') || s[i] == ' ' ||
              s[i] == '[' || s[i] == ']' || s[i] == '.'  || s[i] == '$' ) {
            ss[j] = s[i]; j++;
        }
        i++;
    }
    ss[j] = '\0';
    return String(ss);
}

static XMLNode* xmlgetchildnode(XMLNode* node, string tag) {
    if (!node || node->children.size ==0) return NULL;
#ifdef __DEBUG
    fprintf(stderr, "looking for: xmlgetchildnode: %s\n", tag);
#endif
    XMLNodeList* list = XMLNode_children(node, tag);
    if (!list || list->size == 0) return NULL; //empty list (list not holding any data)
#ifdef __DEBUG
    fprintf(stderr, "Got it: xmlgetchildnode: %s, innert:%s\n", onlyid(list->data[0]->tag), onlyid(list->data[0]->inner_text));
#endif
    return  XMLNodeList_at(list, 0);
}

static T_type xmlirptype(XMLNode *node) {
    string txt = onlyid(node->inner_text);
    if (!strcmp(txt, "T_int")) return T_int;
    else if (!strcmp(txt, "T_float")) return T_float;
    else assert(0);
} 

static char bin_oper[][12] = {
  "T_plus", "T_minus", "T_mul", "T_div", "T_and", "T_or"
};

static char rel_oper[][12] = {
  "T_eq", "T_ne", "T_lt", "T_gt", "T_le", "T_ge"
};

static T_stm xmlirpstm(XMLNode* stm) {
    T_stm ret_s;

    assert(stm);
    string tag = onlyid(stm->tag);
#ifdef __DEBUG
    fprintf(stderr, "DOING THIS STM: xmlstm: tag=%s\n", tag);
#endif
    if (!strcmp(tag, "Seq")) {
        assert(stm && !stmcmp(stm->tag, "Seq"));
        assert(stm->children.size == 2); //must have two stm children
        XMLNode *first = stm->children.data[0];
        XMLNode *right = stm->children.data[1];
        ret_s = T_Seq(xmlirpstm(first), xmlirpstm(right));
    } else if (!strcmp(tag, "Label")) {
        ret_s = T_Label(Temp_namedlabel(onlyid(stm->inner_text)));
    } else if (!strcmp(tag, "Jump")) {
        XMLNode *dst= xmlgetchildnode(stm, "label");
        assert(dst);
        ret_s = T_Jump(Temp_namedlabel(onlyid(dst->inner_text)));
    } else if (!strcmp(tag, "Cjump")) {
        XMLNode *op = xmlgetchildnode(stm, "op");
        assert(op);
        T_relOp o=6;
        string ss = onlyid(op->inner_text);
#ifdef __DEBUG
        fprintf(stderr, "CJump: oper=%s\n", ss);
#endif
        for (int i=0; i<6; i++ ) {
            if (!strcmp(ss, rel_oper[i])) {
                o = (T_relOp)i;
                break;
            }
        }
        if (o == 6) assert(0);
        XMLNode *left = xmlgetchildnode(stm, "left");
        assert(left);
        XMLNode *right = xmlgetchildnode(stm, "right");
        assert(right);
        XMLNode *t = xmlgetchildnode(stm, "true");
        assert(t);
        XMLNode *f = xmlgetchildnode(stm, "false");
        assert(f);
#ifdef __DEBUG
        fprintf(stderr, "CJump: left->%s, right->%s\n", left->children.data[0]->tag, right->children.data[0]->tag);
#endif
        return T_Cjump(o, xmlirpexp(left->children.data[0]), xmlirpexp(right->children.data[0]), 
            Temp_namedlabel(onlyid(t->inner_text)), Temp_namedlabel(onlyid(f->inner_text)));
    } else if (!strcmp(tag, "Move")) {
        XMLNode *dst = xmlgetchildnode(stm, "left");
        XMLNode *src = xmlgetchildnode(stm, "right");
        assert(dst && src);
        ret_s = T_Move(xmlirpexp(dst->children.data[0]), xmlirpexp(src->children.data[0]));
    } else if (!strcmp(tag, "Exp")) {
        XMLNode *e = stm->children.data[0];
        assert(e);
        ret_s = T_Exp(xmlirpexp(e));
    } else if (!strcmp(tag, "Return")) {
        assert(stm->children.size == 1);
        ret_s = T_Return(xmlirpexp(stm->children.data[0]));
    } else {
        assert(0);
    }   
#ifdef __DEBUG
    fprintf(stderr, "====Returning from xmlirpstm tag=%s\n", tag);
#endif
    return ret_s;
}

static T_stmList xmlirpstmlist(XMLNode *l) {
    assert(l || !strcmp(l->tag, "stmList"));
    XMLNode *s = xmlgetchildnode(l, "stm");
    assert(s);
    if (l->children.size == 1) {
        return T_StmList(xmlirpstm(s), NULL);
    } else {
        XMLNode *l1 = xmlgetchildnode(l, "stmList");
        return T_StmList(xmlirpstm(s), xmlirpstmlist(l1));
    }
}

static Temp_temp xmlirptemp(XMLNode *t) {
    assert(t);
    XMLNode *name = xmlgetchildnode(t, "name");
    assert(name);
    XMLNode *type = xmlgetchildnode(t, "type");
    assert(type);
    return Temp_namedtemp(atoi(onlyid(name->inner_text)), xmlirptype(type));
}   

static Temp_tempList xmlirptemplist(XMLNode *l) {
    assert(l || !strcmp(l->tag, "TempList"));
    if (l->children.size == 0)
        return NULL;
    if (l->children.size == 1) 
        return Temp_TempList(xmlirptemp(l->children.data[0]), NULL);
    if (l->children.size == 2 ) {
        XMLNode *l1 = xmlgetchildnode(l, "TempList");
        return Temp_TempList(xmlirptemp(l->children.data[0]), xmlirptemplist(l1));
    }
    fprintf(stderr, "Invalid TempList Element.\n");
    exit -2;
} 

static T_funcDecl xmlirpfunc(XMLNode *func) {
    XMLNode *n = xmlgetchildnode(func, "name");
    assert(n && n->children.size>0);  //must have a name
    XMLNode *ret_t = xmlgetchildnode(func, "ret_type");
    assert(ret_t && ret_t->children.size>0); //must have a return type
    if (func->children.size == 2) { //no TempList and no stm
        return T_FuncDecl(String(onlyid(n->inner_text)), NULL, NULL, xmlirptype(ret_t));
    }
    Temp_tempList tl;
    XMLNode *a = xmlgetchildnode(func, "TempList");
    if (a && a->children.size > 0) {
        tl = xmlirptemplist(a);
    } else {
        tl=NULL;
    }
    T_stm ss;
    if (func->children.size == 3 && (!a || a->children.size == 0)) { //no TempList, but with stm
        XMLNode *s = func->children.data[1]; //this must be a stm
        assert(s && s->children.size>0); 
        ss=xmlirpstm(s);
    } else if (func->children.size == 4 && a->children.size > 0) { //with TempList and stm
        XMLNode *s = func->children.data[2]; //this must be a stm
        assert(s && s->children.size>0); 
        ss=xmlirpstm(s);
    } else ss=NULL;
    return T_FuncDecl(String(onlyid(n->inner_text)), tl, ss, xmlirptype(ret_t));
}

T_funcDeclList xmlirpfunclist(XMLNode *l) {
    assert(l && !strcmp(l->tag, "FuncDeclList"));
#ifdef __DEBUG
    printf("xmlirpfunclist: tag=%s\n", l->tag);
#endif
    XMLNode *f = xmlgetchildnode(l, "FuncDecl");
    assert(f && f->children.size>0);
#ifdef __DEBUG
    fprintf(stderr, "---Endtering function decl: %s\n", onlyid(f->tag));
#endif
    if (l->children.size == 1) {
        return T_FuncDeclList(xmlirpfunc(f), NULL);
    } else {
        XMLNode *l1 = xmlgetchildnode(l, "FuncDeclList");
        return T_FuncDeclList(xmlirpfunc(f), xmlirpfunclist(l1));
    }
}

static T_exp xmlirpexp(XMLNode* exp) {
    T_exp ret_e;

    assert(exp && exp->children.size > 0);

    string tag = onlyid(exp->tag);
#ifdef __DEBUG
    fprintf(stderr, "DOING THIS EXP: xmlexp: tag=%s, inner-text=%s\n", tag, exp->inner_text);
#endif
    if (!strcmp(tag, "Binop")) {
        // code for opExp
        XMLNode *type = xmlgetchildnode(exp, "type"); // type
        assert(type);
        T_type t=xmlirptype(type);
        XMLNode *left = xmlgetchildnode(exp, "left"); // left expression
        assert(left);
        XMLNode *oper = xmlgetchildnode(exp, "op"); // operator
        assert(oper);
        XMLNode *right = xmlgetchildnode(exp, "right"); // right expression
        assert(right);

        T_binOp op;
        string ss = onlyid(oper->inner_text);
        for (int i=0; i<6; i++ ) {
            if (!strcmp(ss, bin_oper[i])) {
                op = (T_binOp)i;
                break;
            }
        }
        if (op>=6) assert(0);
        //the exp nodes are one level under left and right 
        ret_e = T_Binop(op, xmlirpexp(left->children.data[0]), xmlirpexp(right->children.data[0])); 
    } else if (!strcmp(tag, "Mem")) {
        XMLNode *type = xmlgetchildnode(exp, "type"); // first child is a type
        assert(type);
        T_type t=xmlirptype(type);
        assert(exp->children.size == 2);
        T_exp e = xmlirpexp(exp->children.data[1]); // second must be an exp
        assert(e);
        ret_e = T_Mem(e, t);
    } else if (!strcmp(tag, "Name")) {
        ret_e = T_Name(Temp_namedlabel(onlyid(exp->inner_text)));
    } else if (!strcmp(tag, "Temp")) {
        XMLNode *tt= xmlgetchildnode(exp, "temp"); // a child is a temp
        assert(tt);
        XMLNode *name = xmlgetchildnode(tt, "name"); // a child is a name
        assert(name);
        XMLNode *ttt = xmlgetchildnode(tt, "type"); // a child is an id
        assert(ttt);
#ifdef __DEBUG
        fprintf(stderr, "Temp: %s\n", name->inner_text);
#endif
        return T_Temp(Temp_namedtemp(atoi(onlyid(name->inner_text)), xmlirptype(ttt)));
    } else if (!strcmp(tag, "Eseq")) {
        // code for Eseq
        XMLNode *type = xmlgetchildnode(exp, "type"); // a child is a type
        assert(type);
        T_type t=xmlirptype(type);
        XMLNode *stm = xmlgetchildnode(exp, "Stm"); // a child is a stm
        T_stm s;
        if (stm && stm->children.size>0) { //if stm child exists
            s = xmlirpstm(stm->children.data[0]);
        } else 
            s = NULL;
        XMLNode *e = xmlgetchildnode(exp, "Exp"); // a child is an exp
        assert(e); //expression must exist
        ret_e = T_Eseq(s, xmlirpexp(e->children.data[0]));
    } else if (!strcmp(tag, "IntConst")) {
        ret_e = T_IntConst(atoi(onlyid(exp->inner_text)));
    } else if (!strcmp(tag, "FloatConst")) {
#ifdef __DEBUG
        fprintf(stderr, "FloatConst: %lf\n", atof(onlyid(exp->inner_text)));
#endif
        ret_e = T_FloatConst(atof(onlyid(exp->inner_text)));
    } else if (!strcmp(tag, "Call")) {
        XMLNode *type = xmlgetchildnode(exp, "ret_type"); // a child is a return type
        assert(type); //must have
        XMLNode *id = xmlgetchildnode(exp, "id"); // a child is an id
        assert(id); //must have
        XMLNode *obj = xmlgetchildnode(exp, "obj"); // a child is an obj
        assert(obj); //must have
        XMLNode *args = xmlgetchildnode(exp, "args"); // a child is an args
        T_expList a; //args may be empty
        if (args) a = xmlirpexplist(args->children.data[0]);
        else a = NULL;
        ret_e = T_Call(String(onlyid(id->inner_text)), xmlirpexp(obj->children.data[0]), a, xmlirptype(type));
    } else if (!strcmp(tag, "ExtCall")) {
        XMLNode *type = xmlgetchildnode(exp, "ret_type"); // a child is a return type
        assert(type); //must have
        XMLNode *id = xmlgetchildnode(exp, "id"); // a child is an id
        assert(id); //must have
        XMLNode *args = xmlgetchildnode(exp, "args"); // a child is an args
        T_expList a; //args may be empty
        if (args && args->children.size>0) a = xmlirpexplist(args->children.data[0]);
        else a = NULL;
        ret_e = T_ExtCall(String(onlyid(id->inner_text)), a, xmlirptype(type));
    } else if (!strcmp(tag, "Cast")) {
        XMLNode *type = xmlgetchildnode(exp, "type"); // a child is a type
        assert(type); //must have
#ifdef __DEBUG
        fprintf(stderr, "Cast size=%d\n", exp->children.size);
#endif
        if (exp->children.size == 2) {
            ret_e = T_Cast(xmlirpexp(exp->children.data[0]), xmlirptype(type));
        } else {
            fprintf(stderr, "Invalid Cast Element.\n");
            exit(-1);
        }
    } else {
        fprintf(stderr, "Invalid tag=%s\n", tag);
        exit(-1);
    }
#ifdef __DEBUG
    fprintf(stderr, "======Returning from xmlirpexp tag=%s\n", tag);
#endif
    return ret_e;
}

static T_expList xmlirpexplist(XMLNode *l) {
    assert(l || !strcmp(l->tag, "ExpList"));
    if (l->children.size == 1) {
        return T_ExpList(xmlirpexp(l->children.data[0]), NULL);
    } else {
        XMLNode *l1 = xmlgetchildnode(l, "ExpList");
        return T_ExpList(xmlirpexp(l->children.data[0]), xmlirpexplist(l1));
    }   
}
