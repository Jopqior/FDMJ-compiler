#include <stdio.h>
#include <string.h>
#include "fdmjast.h"
#include "util.h"
#include "temp.h"
#include "assem.h"
#include "assemblock.h"
#include "bg.h"
#include "ig.h"
#include "lxml.h"
#include "xml2ins.h"
#include "print_ins.h"

#define __DEBUG
#undef __DEBUG

A_prog root;

AS_instrList AS_instrList_to_SSA(AS_instrList bodyil, G_nodeList lg, G_nodeList bg); 


static AS_instrList xmlfunc2ins(XMLNode *fn) {
#ifdef __DEBUG
    fprintf(stderr, "==Now reading Function %s\n", fn->attributes.data->value);
#endif
    if (!fn || strcmp(fn->tag, "function")) {
      fprintf(stderr, "Error: Invalid function in the ins XML file\n");
      exit -1;
    }
    AS_instrList il = insxml2func(fn);
}

static void print_to_ssa_file(string file_ssa, AS_instrList il) {
    freopen(file_ssa, "a", stdout);
    AS_printInstrList(stdout, il, Temp_name());
    fclose(stdout);
}

static AS_blockList instrList2BL(AS_instrList il) {
    AS_instrList b = NULL;
    AS_blockList bl = NULL;
    AS_instrList til = il;

    while (til) {
      if (til->head->kind == I_LABEL) {
        if (b) { //if we have a label but the current block is not empty, then we have to stop the block
          Temp_label l = til->head->u.LABEL.label;
          b = AS_splice(b, AS_InstrList(AS_Oper(String("br label `j0"), NULL, NULL, AS_Targets(Temp_LabelList(l, NULL))), NULL)); 
                    //add a jump to the block to be stopped, only for LLVM IR
          bl = AS_BlockSplice(bl, AS_BlockList(AS_Block(b), NULL)); //add the block to the block list
#ifdef __DEBUG
          fprintf(stderr, "1----Start a new Block %s\n", Temp_labelstring(l)); fflush(stderr);
#endif
          b = NULL; //start a new block
        } 
      }

      assert(b||til->head->kind == I_LABEL);  //if not a label to start a block, something's wrong!

#ifdef __DEBUG
      if (!b && til->head->kind == I_LABEL)
          fprintf(stderr, "2----Start a new Block %s\n", Temp_labelstring(til->head->u.LABEL.label)); fflush(stderr);
#endif

      //now add the instruction to the block
      b = AS_splice(b, AS_InstrList(til->head, NULL));

      if (til->head->kind == I_OPER && ((til->head->u.OPER.jumps && til->head->u.OPER.jumps->labels)||(
      !strcmp(til->head->u.OPER.assem,"ret i64 -1")||!strcmp(til->head->u.OPER.assem,"ret double -1.0")))) {
#ifdef __DEBUG
          fprintf(stderr, "----Got a jump, ending the block for label = %s\n", Temp_labelstring(b->head->u.LABEL.label)); fflush(stderr);
#endif
          bl = AS_BlockSplice(bl, AS_BlockList(AS_Block(b), NULL)); //got a jump, stop a block
          b = NULL; //and start a new block
      } 
      til = til->tail;
    }
#ifdef __DEBUG
    fprintf(stderr, "----Processed all instructions\n"); fflush(stderr);
#endif
    return bl;
}

int main(int argc, const char * argv[]) {
  if (argc != 2) {
    fprintf(stdout, "Usage: %s XMLIRPfilename\n", argv[0]);
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
  sprintf(file_liv, "%s.5.ins", file);
  string file_ins = checked_malloc(IR_MAXLEN);
  sprintf(file_ins, "%s.5.ins", file);
  string file_insxml = checked_malloc(IR_MAXLEN);
  sprintf(file_insxml, "%s.6.ins", file);
  string file_cfg= checked_malloc(IR_MAXLEN);
  sprintf(file_cfg, "%s.7.cfg", file);
  string file_ssa= checked_malloc(IR_MAXLEN);
  sprintf(file_ssa, "%s.8.ssa", file);

  XMLDocument doc;

  if (XMLDocument_load(&doc, file_insxml)) {
    if (!doc.root) {
      fprintf(stderr, "Error: Invalid ins XML file\n");
      return -1;
    }
    if (doc.root->children.size == 0) {
      fprintf(stderr, "Error: Nothing in the ins XML file\n");
      return -1;
    }
  }

  XMLNode *insroot = doc.root->children.data[0]; //this is the <root> node
  if (!insroot || insroot->children.size == 0) {
    fprintf(stderr, "Error: No function in the ins XML file\n");
    return -1;
  } 

  AS_instrList inslist_func;

  for (int i = 0; i < insroot->children.size; i++) {
    XMLNode *fn = insroot->children.data[i];
#ifdef __DEBUG
    fprintf(stderr, "==Now reading Function %d: %s\n", i, fn->attributes.data->value);
#endif
    if (!fn || strcmp(fn->tag, "function")) {
      fprintf(stderr, "Error: Invalid function in the ins XML file\n");
      return -1;
    }
    //read from xml to an AS_instrList
    inslist_func = insxml2func(fn);

#ifdef __DEBUG
    fprintf(stderr, "------ now got Function %d, %s------\n", i, fn->attributes.data->value);
    fflush(stderr);
#endif

    // get the prologi and epilogi from the inslist_func
    // remove them from the inslist_func, and form the bodyil = instruction body of the function
    AS_instr prologi = inslist_func->head;
#ifdef __DEBUG
    fprintf(stderr, "prologi->assem = %s\n", prologi->u.OPER.assem);
    fflush(stderr);
#endif
    AS_instrList bodyil = inslist_func->tail;
    inslist_func->tail = NULL; // remove the prologi from the inslist_func
    AS_instrList til = bodyil;
    AS_instr epilogi;
    if (til->tail == NULL) {
#ifdef __DEBUG
        fprintf(stderr, "Empty body");
        fflush(stderr);
#endif
        epilogi = til->head;
        bodyil = NULL;
    } else {
      while (til ->tail->tail != NULL) {
#ifdef __DEBUG
        fprintf(stderr, "til->head->kind = %s\n", til->head->u.OPER.assem);
        fflush(stderr);
#endif
        til = til->tail;
      }
      epilogi = til->tail->head;
#ifdef __DEBUG
    fprintf(stderr, "epilogi->assem = %s\n", epilogi->u.OPER.assem);
    fflush(stderr);
#endif
      til->tail=NULL;
    }

#ifdef __DEBUG
    fprintf(stderr, "------ now we've seperated body into prolog, body, and epilog -----\n");
    fflush(stderr);
#endif

    /* doing the control graph and print to *.8.cfg*/
    // get the control flow and print out the control flow graph to *.8.cfg
    G_graph fg = FG_AssemFlowGraph(bodyil);
    freopen(file_cfg, "a", stdout);
    fprintf(stdout, "------Flow Graph------\n");
    fflush(stdout);
    G_show(stdout, G_nodes(fg), (void*)FG_show);
    fflush(stdout);
    fclose(stdout);

    // data flow analysis
    freopen(file_cfg, "a", stdout);
    G_nodeList lg = Liveness(G_nodes(fg));
    freopen(file_cfg, "a", stdout);
    fprintf(stdout, "/* ------Liveness Graph------*/\n");
    Show_Liveness(stdout, lg);
    fflush(stdout);
    fclose(stdout);
  
    G_nodeList bg = Create_bg(instrList2BL(bodyil)); // create a basic block graph
    freopen(file_cfg, "a", stdout);
    fprintf(stdout, "------Basic Block Graph------\n");
    Show_bg(stdout, bg);
    fprintf(stdout, "\n\n");
    fflush(stdout);
    fclose(stdout);

    AS_instrList bodyil_in_SSA = AS_instrList_to_SSA(bodyil, lg, bg); 

    //print the AS_instrList to the ssa file`
    AS_instrList finalssa = AS_splice(AS_InstrList(prologi, bodyil_in_SSA), AS_InstrList(epilogi, NULL));
    print_to_ssa_file(file_ssa, finalssa);
  }
  // print the runtime functions for the 7.ssa file
  freopen(file_ssa, "a", stdout);
  fprintf(stdout, "declare void @starttime()\n");
  fprintf(stdout, "declare void @stoptime()\n");
  fprintf(stdout, "declare i64* @malloc(i64)\n");
  fprintf(stdout, "declare void @putch(i64)\n");
  fprintf(stdout, "declare void @putint(i64)\n");
  fprintf(stdout, "declare void @putfloat(double)\n");
  fprintf(stdout, "declare i64 @getint()\n");
  fprintf(stdout, "declare i64 @getch(i64)\n");
  fprintf(stdout, "declare float @getfloat()\n");
  fprintf(stdout, "declare i64* @getarray(i64)\n");
  fprintf(stdout, "declare i64* @getfarray(i64)\n");
  fprintf(stdout, "declare void @putarray(i64, i64*)\n");
  fprintf(stdout, "declare void @putfarray(i64, i64*)\n");
  fclose(stdout);

  return 0;
}
