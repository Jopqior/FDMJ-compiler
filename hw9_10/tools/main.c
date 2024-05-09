#include <stdio.h>
#include <string.h>
#include "fdmjast.h"
#include "xml2irp.h"
#include "canon.h"
#include "util.h"
#include "print_stm.h"
#include "print_irp.h"
#include "temp.h"
#include "tigerirp.h"
#include "assem.h"
#include "assemblock.h"
#include "bg.h"
#include "llvmgen.h"

A_prog root;

T_funcDeclList fdl=NULL;

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
  string file_llvm = checked_malloc(IR_MAXLEN);
  sprintf(file_llvm, "%s.6.llvm", file);

  // parse xml to irp
  XMLDocument doc;
  if (XMLDocument_load(&doc, file_irp)) {
    if (!doc.root) {
      fprintf(stderr, "Error: Invalid TigerIR+ XML file\n");
      return -1;
    }
    if (doc.root->children.size == 0) {
      fprintf(stderr, "Error: Nothing in the TigerIR+ XML file\n");
      return -1;
    }
    fdl = xmlirpfunclist(XMLNode_child(doc.root, 0));
  }

  //fprintf(stdout, "IRP file loaded successfully. %s\n", fdl->head->name);
  if (!fdl) {
    fprintf(stderr, "Error: No function decl list in the TigerIR+ XML file\n");
    return -1;
  }

  //printIRP_FuncDeclList(stdout, fdl);
  //now fdl is the function declaration list. We now linearize and canonicalize each function.
  
  while (fdl) {
    T_stm s = fdl->head->stm; //get the statement list of the function
    freopen(file_stm, "a", stdout);
    fprintf(stdout, "------Original IR Tree------\n");
    printIRP_set(IRP_parentheses);
    printIRP_FuncDecl(stdout, fdl->head);
    fprintf(stdout, "\n\n");
    fflush(stdout);
    fclose(stdout);
    
    T_stmList sl = C_linearize(s);
    freopen(file_stm, "a", stdout);
    fprintf(stdout, "------Linearized IR Tree------\n");
    printStm_StmList(stdout, sl, 0);
    fprintf(stdout, "\n\n");
    fflush(stdout);
    fclose(stdout);

    struct C_block b = C_basicBlocks(sl); //break the linearized IR tree into basic blocks
    freopen(file_stm, "a", stdout);
    fprintf(stdout, "------Basic Blocks------\n");
    for (C_stmListList sll = b.stmLists; sll; sll = sll->tail) { //print the basic blocks. 
                      // Each block is a list of statements starting with a label, ending with a jump
      fprintf(stdout, "For Label=%s\n", S_name(sll->head->head->u.LABEL));
      printStm_StmList(stdout, sll->head, 0); //print the statements in the block
    }
    fprintf(stdout, "\n\n");
    fflush(stdout);
    fclose(stdout);
    
    /* No need to trace schedule for now, as we are not using it
    sl = C_traceSchedule(b);
    freopen(file_stm, "a", stdout);
    fprintf(stdout, "------Canonical IR Tree------\n");
    printStm_StmList(stdout, sl, 0);
    fprintf(stdout, "\n\n");
    fflush(stdout);
    fclose(stdout);

    //b = C_basicBlocks(sl); 
    */

    // llvm instruction selection. First making the head of the function, then the body, then the epilog

    //Making LLVM IR function header. See llvmgen.h for the function prototype, and llvmgen.c for the implementation
    //fprintf(stderr, "Instruction selection for function: %s\n", fdl->head->name);
    AS_instrList prologil = llvmprolog(fdl->head->name, fdl->head->args, fdl->head->ret_type); //add the prolog of the function
    AS_blockList bodybl = NULL; //making an empty body
    for (C_stmListList sll = b.stmLists; sll; sll = sll->tail) { // for each basic block we do the instruction selection

/* ****************************************************************************************************/
//
//      The followsing is the instruction selection function. It takes a list of statements and returns a list of AS instructions.
//      YOU ARE SUPPOSED TO IMPLEMENT THIS FUNCTION. THE FUNCTION IS IN llvmgen.c. THE FUNCTION IS CALLED llvmbody.

      AS_instrList bil = llvmbody(sll->head); //This is the instruction selection function. See llvmgen.h for the function prototype.


      //AS_printInstrList(stderr, bil, Temp_name()); //you may debug your instruction selection result of a block here
/* ***************************************************************************************************  */      

      AS_blockList bbl = AS_BlockList(AS_Block(bil), NULL); //making the list of AS instructions into a block of AS instructions
      bodybl = AS_BlockSplice(bodybl, bbl); //putting the block into the list of blocks
    }
    /* not necessary for now
    if (bodybl && bodybl->head && bodybl->head->label) { //if the block has a label, we need to make sure the first instruction jumps to it
      AS_instrList bil = llvmbodybeg(bodybl->head->label);  //add the necessary jump instruction
      AS_blockList bbl = AS_BlockList(AS_Block(bil), NULL); //make it into the block
      bodybl = AS_BlockSplice(bbl, bodybl); //merge the block
    }
    */
    AS_instrList epilogil = llvmepilog(b.label); //add the epilog of the function. See llvmgen.h for the function prototype.

//Now make a control flow graph (CFG) of the function. See assemblock.h for the function prototype
    G_nodeList bg = Create_bg(bodybl); // CFG

    freopen(file_ins, "a", stdout);
    fprintf(stdout, "\n------For function ----- %s\n\n", fdl->head->name); 
    fprintf(stdout, "------Basic Block Graph---------\n");
    Show_bg(stdout, bg);
    //put all the blocks into one AS list
    AS_instrList il = AS_traceSchedule(bodybl, prologil, epilogil, FALSE);

    printf("------~Final traced AS instructions ---------\n");
    AS_printInstrList(stdout, il, Temp_name());
    fflush(stdout);
    fclose(stdout);

    freopen(file_llvm, "a", stdout);
    AS_printInstrList(stdout, il, Temp_name());
    fflush(stdout);
    fclose(stdout);

    fdl = fdl->tail;
  }

  freopen(file_llvm, "a", stdout);
  fprintf(stdout, "declare void @starttime()\n");
  fprintf(stdout, "declare void @stoptime()\n");
  fprintf(stdout, "declare i64* @malloc(i64)\n");
  fprintf(stdout, "declare void @putch(i64)\n");
  fprintf(stdout, "declare void @putint(i64)\n");
  fprintf(stdout, "declare void @putfloat(double)\n");
  fprintf(stdout, "declare i64 @getint()\n");
  fprintf(stdout, "declare float @getfloat()\n");
  fprintf(stdout, "declare i64* @getarray(i64)\n");
  fprintf(stdout, "declare i64* @getfarray(i64)\n");
  fprintf(stdout, "declare void @putarray(i64, i64*)\n");
  fprintf(stdout, "declare void @putfarray(i64, i64*)\n");
  fflush(stdout);
  fclose(stdout);

  return 0;
}