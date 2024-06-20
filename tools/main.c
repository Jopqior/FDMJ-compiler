#include <stdio.h>
#include <string.h>

#include "armgen.h"
#include "assem.h"
#include "assemblock.h"
#include "bg.h"
#include "canon.h"
#include "fdmjast.h"
#include "ig.h"
#include "llvmgen.h"
#include "lxml.h"
#include "print_ast.h"
#include "print_ins.h"
#include "print_irp.h"
#include "print_src.h"
#include "print_stm.h"
#include "regalloc.h"
#include "semant.h"
#include "ssa.h"
#include "temp.h"
#include "tigerirp.h"
#include "util.h"
#include "xml2ast.h"
#include "xml2ins.h"
#include "xml2irp.h"

#define __DEBUG
#undef __DEBUG

A_prog root;

extern int yyparse();

static struct C_block canonicalize(T_funcDecl, string, bool);
static AS_instrList llvmInsSelect(T_funcDecl, struct C_block, string, bool);
static G_nodeList livenessAnalyze(AS_instrList, string);

static void printIrpIns(string file_irp, T_funcDecl func) {
  freopen(file_irp, "a", stdout);
  fprintf(stdout, "------Original IR Tree------\n");
  printIRP_set(IRP_parentheses);
  printIRP_FuncDecl(stdout, func);
  fprintf(stdout, "\n\n");
  fflush(stdout);
  fclose(stdout);
}

static void printSSAIns(string file_ssa, AS_instrList il) {
  freopen(file_ssa, "a", stdout);
  AS_printInstrList(stdout, il, Temp_name());
  fclose(stdout);
}

static void printArmIns(string file_arm, AS_instrList il, string funcname) {
  freopen(file_arm, "a", stdout);
  fprintf(stdout, "\t.text\n");
  fprintf(stdout, "\t.align 1\n");
  fprintf(stdout, "\t.global %s\n", funcname);
  AS_printInstrList(stdout, il, Temp_name());
  fflush(stdout);
  fclose(stdout);
}

static void printRpiIns(string file_rpi, RA_result ra, string funcname) {
  Temp_map coloring = ra->coloring;
  AS_instrList il = ra->il;
  freopen(file_rpi, "a", stdout);
  fprintf(stdout, "\t.text\n");
  fprintf(stdout, "\t.align 2\n");
  fprintf(stdout, "\t.global %s\n", funcname);
  AS_printInstrList(stdout, il, coloring);
  fflush(stdout);
  fclose(stdout);
}

static void print_llvm_runtime_funcs(string filename) {
  freopen(filename, "a", stdout);
  fprintf(stdout, "declare void @starttime()\n");
  fprintf(stdout, "declare void @stoptime()\n");
  fprintf(stdout, "declare i64* @malloc(i64)\n");
  fprintf(stdout, "declare void @putch(i64)\n");
  fprintf(stdout, "declare void @putint(i64)\n");
  fprintf(stdout, "declare void @putfloat(double)\n");
  fprintf(stdout, "declare i64 @getint()\n");
  fprintf(stdout, "declare float @getfloat()\n");
  fprintf(stdout, "declare i64* @getarray(i64)\n");
  fprintf(stdout, "declare i64 @getch(i64)\n");
  fprintf(stdout, "declare i64* @getfarray(i64)\n");
  fprintf(stdout, "declare void @putarray(i64, i64*)\n");
  fprintf(stdout, "declare void @putfarray(i64, i64*)\n");
  fclose(stdout);
}

static void print_arm_runtime_funcs(string filename) {
  freopen(filename, "a", stdout);
  fprintf(stdout, ".global malloc\n");
  fprintf(stdout, ".global getint\n");
  fprintf(stdout, ".global getch\n");
  fprintf(stdout, ".global getfloat\n");
  fprintf(stdout, ".global getarray\n");
  fprintf(stdout, ".global getfarray\n");
  fprintf(stdout, ".global putint\n");
  fprintf(stdout, ".global putch\n");
  fprintf(stdout, ".global putfloat\n");
  fprintf(stdout, ".global putarray\n");
  fprintf(stdout, ".global putfarray\n");
  fprintf(stdout, ".global starttime\n");
  fprintf(stdout, ".global stoptime\n");
  fclose(stdout);
}

static AS_blockList llvmInstrList2BL(AS_instrList il) {
  AS_instrList b = NULL;
  AS_blockList bl = NULL;
  AS_instrList til = il;

  while (til) {
    if (til->head->kind == I_LABEL) {
      if (b) {  // if we have a label but the current block is not empty, then
                // we have to stop the block
        Temp_label l = til->head->u.LABEL.label;
        b = AS_splice(b,
                      AS_InstrList(AS_Oper(String("br label `j0"), NULL, NULL,
                                           AS_Targets(Temp_LabelList(l, NULL))),
                                   NULL));
        // add a jump to the block to be stopped, only for LLVM IR
        bl = AS_BlockSplice(
            bl, AS_BlockList(AS_Block(b),
                             NULL));  // add the block to the block list
#ifdef __DEBUG
        fprintf(stderr, "1----Start a new Block %s\n", Temp_labelstring(l));
        fflush(stderr);
#endif
        b = NULL;  // start a new block
      }
    }

    assert(b ||
           til->head->kind ==
               I_LABEL);  // if not a label to start a block, something's wrong!

#ifdef __DEBUG
    if (!b && til->head->kind == I_LABEL)
      fprintf(stderr, "2----Start a new Block %s\n",
              Temp_labelstring(til->head->u.LABEL.label));
    fflush(stderr);
#endif

    // now add the instruction to the block
    b = AS_splice(b, AS_InstrList(til->head, NULL));

    if (til->head->kind == I_OPER &&
        ((til->head->u.OPER.jumps && til->head->u.OPER.jumps->labels) ||
         (!strcmp(til->head->u.OPER.assem, "ret i64 -1") ||
          !strcmp(til->head->u.OPER.assem, "ret double -1.0")))) {
#ifdef __DEBUG
      fprintf(stderr, "----Got a jump, ending the block for label = %s\n",
              Temp_labelstring(b->head->u.LABEL.label));
      fflush(stderr);
#endif
      bl = AS_BlockSplice(
          bl, AS_BlockList(AS_Block(b), NULL));  // got a jump, stop a block
      b = NULL;                                  // and start a new block
    }
    til = til->tail;
  }
#ifdef __DEBUG
  fprintf(stderr, "----Processed all instructions\n");
  fflush(stderr);
#endif
  return bl;
}

int main(int argc, const char* argv[]) {
  if (argc != 4) {
    fprintf(stdout, "Usage: %s XMLIRPfilename -c [0/1]\n", argv[0]);
    return 1;
  }
  bool isLLVM = (argv[3][0] == '0');

  // output filename
  string file = checked_malloc(IR_MAXLEN);
  sprintf(file, "%s", argv[1]);
  string file_fmj = checked_malloc(IR_MAXLEN);
  sprintf(file_fmj, "%s.fmj", file);
  string file_src = checked_malloc(IR_MAXLEN);
  sprintf(file_src, "%s.1.src", file);
  string file_ast = checked_malloc(IR_MAXLEN);
  sprintf(file_ast, "%s.2.ast", file);
  string file_irp = checked_malloc(IR_MAXLEN);
  sprintf(file_irp, "%s.3.irp", file);
  string file_stm = checked_malloc(IR_MAXLEN);
  sprintf(file_stm, "%s.4.stm", file);
  string file_ins = checked_malloc(IR_MAXLEN);
  sprintf(file_ins, "%s.5.ins", file);
  string file_cfg = checked_malloc(IR_MAXLEN);
  sprintf(file_cfg, "%s.6.cfg", file);
  string file_ssa = checked_malloc(IR_MAXLEN);
  sprintf(file_ssa, "%s.7.ssa", file);
  string file_arm = checked_malloc(IR_MAXLEN);
  sprintf(file_arm, "%s.8.arm", file);
  string file_ig = checked_malloc(IR_MAXLEN);
  sprintf(file_ig, "%s.9.ig", file);
  string file_rpi = checked_malloc(IR_MAXLEN);
  sprintf(file_rpi, "%s.10.s", file);

  // lex & parse
  yyparse();
  if (!root) {
    fprintf(stderr, "Error: No program in the FDMJ file\n");
    return -1;
  }

  if (isLLVM) {
    // ast2src
    freopen(file_src, "w", stdout);
    printA_Prog(stdout, root);
    fflush(stdout);
    fclose(stdout);

    // ast2xml
    freopen(file_ast, "w", stdout);
    printX_Prog(stdout, root);
    fflush(stdout);
    fclose(stdout);

    T_funcDeclList fdl = transA_Prog(stderr, root, 8);

    for (T_funcDeclList f = fdl; f; f = f->tail) {
      printIrpIns(file_irp, f->head);

      struct C_block b = canonicalize(f->head, file_stm, TRUE);

      AS_instrList il = llvmInsSelect(f->head, b, file_ins, FALSE);

      AS_instr prologi = il->head;
#ifdef __DEBUG
      fprintf(stderr, "prologi->assem = %s\n", prologi->u.OPER.assem);
      fflush(stderr);
#endif
      AS_instrList bodyil = il->tail;
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
        while (til->tail->tail != NULL) {
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
        til->tail = NULL;
      }
#ifdef __DEBUG
      fprintf(stderr,
              "------ now we've seperated body into prolog, body, and epilog "
              "-----\n");
      fflush(stderr);
#endif

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

      G_nodeList bg =
          Create_bg(llvmInstrList2BL(bodyil));  // create a basic block graph
      freopen(file_cfg, "a", stdout);
      fprintf(stdout, "------Basic Block Graph------\n");
      Show_bg(stdout, bg);
      fprintf(stdout, "\n\n");
      fflush(stdout);
      fclose(stdout);

      AS_instrList bodyil_in_SSA = AS_instrList_to_SSA(bodyil, lg, bg);

      // print the AS_instrList to the ssa file
      AS_instrList finalssa = AS_splice(AS_InstrList(prologi, bodyil_in_SSA),
                                        AS_InstrList(epilogi, NULL));
      printSSAIns(file_ssa, finalssa);
    }

    print_llvm_runtime_funcs(file_ssa);

    return 0;
  }

  T_funcDeclList fdl = transA_Prog(stderr, root, 4);

  while (fdl) {
    // printIrpIns(file_irp, fdl->head);

    /* Canonicalization */
    struct C_block b = canonicalize(fdl->head, file_stm, FALSE);

    /* LLVM Instruction selection */
    AS_instrList il = llvmInsSelect(fdl->head, b, file_ins, FALSE);

    AS_instr prologi = il->head;
#ifdef __DEBUG
    fprintf(stderr, "prologi->assem = %s\n", prologi->u.OPER.assem);
    fflush(stderr);
#endif
    AS_instrList bodyil = il->tail;
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
      while (til->tail->tail != NULL) {
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
      til->tail = NULL;
    }
#ifdef __DEBUG
    fprintf(stderr,
            "------ now we've seperated body into prolog, body, and epilog "
            "-----\n");
    fflush(stderr);
#endif

    /* doing the control graph and print to *.8.cfg*/
    // get the control flow and print out the control flow graph to *.8.cfg
    G_graph fg = FG_AssemFlowGraph(bodyil);
    // freopen(file_cfg, "a", stdout);
    // fprintf(stdout, "------Flow Graph------\n");
    // fflush(stdout);
    // G_show(stdout, G_nodes(fg), (void*)FG_show);
    // fflush(stdout);
    // fclose(stdout);

    // data flow analysis
    G_nodeList lg = Liveness(G_nodes(fg));
    // freopen(file_cfg, "a", stdout);
    // fprintf(stdout, "/* ------Liveness Graph------*/\n");
    // Show_Liveness(stdout, lg);
    // fflush(stdout);
    // fclose(stdout);

    // create a basic block graph
    G_nodeList bg = Create_bg(llvmInstrList2BL(bodyil));
    // freopen(file_cfg, "a", stdout);
    // fprintf(stdout, "------Basic Block Graph------\n");
    // Show_bg(stdout, bg);
    // fprintf(stdout, "\n\n");
    // fflush(stdout);
    // fclose(stdout);

    /* SSA */
    AS_instrList bodyil_in_SSA = AS_instrList_to_SSA(bodyil, lg, bg);

    // print the AS_instrList to the ssa file`
    AS_instrList finalssa = AS_splice(AS_InstrList(prologi, bodyil_in_SSA),
                                      AS_InstrList(epilogi, NULL));
    // printSSAIns(file_ssa, finalssa);

    /* RPi Instruction selection */
    G_graph ssa_bg = Create_SSA_bg(bg);
    AS_instrList bodyil_wo_SSA = SSA_destruction(bodyil_in_SSA, ssa_bg);

    AS_instrList prologil_arm = armprolog(AS_InstrList(prologi, NULL));
    AS_instrList epilogil_arm = armepilog(AS_InstrList(epilogi, NULL));
    AS_instrList bodyil_arm = armbody(bodyil_wo_SSA);
    AS_instrList finalarm =
        AS_splice(AS_splice(prologil_arm, bodyil_arm), epilogil_arm);
    printArmIns(file_arm, finalarm, fdl->head->name);

    /* Liveness analysis */
    G_nodeList arm_ig = livenessAnalyze(finalarm, file_ig);

    /* Register allocation */
    RA_result ra = RA_regAlloc(finalarm, arm_ig);
    printRpiIns(file_rpi, ra, fdl->head->name);

    fdl = fdl->tail;
  }

  // print the runtime functions for the 9.arm file
  print_arm_runtime_funcs(file_arm);

  // print the runtime functions for the 10.s file
  print_arm_runtime_funcs(file_rpi);

  return 0;
}

static struct C_block canonicalize(T_funcDecl func, string file_stm,
                                   bool isPrint) {
  T_stm s = func->stm;  // get the statement list of the function

  T_stmList sl = C_linearize(s);
  if (isPrint) {
    freopen(file_stm, "a", stdout);
    fprintf(stdout, "------Linearized IR Tree------\n");
    printStm_StmList(stdout, sl, 0);
    fprintf(stdout, "\n\n");
    fflush(stdout);
    fclose(stdout);
  }

  // break the linearized IR tree into basic blocks
  struct C_block b = C_basicBlocks(sl);
  if (isPrint) {
    freopen(file_stm, "a", stdout);
    fprintf(stdout, "------Basic Blocks------\n");
    for (C_stmListList sll = b.stmLists; sll;
         sll = sll->tail) {  // print the basic blocks.
      // Each block is a list of statements starting with a label, ending with a
      // jump
      fprintf(stdout, "For Label=%s\n", S_name(sll->head->head->u.LABEL));
      printStm_StmList(stdout, sll->head,
                       0);  // print the statements in the block
    }
    fprintf(stdout, "\n\n");
    fflush(stdout);
    fclose(stdout);
  }

  sl = C_traceSchedule(b);
  if (isPrint) {
    freopen(file_stm, "a", stdout);
    fprintf(stdout, "------Canonical IR Tree------\n");
    printStm_StmList(stdout, sl, 0);
    fprintf(stdout, "\n\n");
    fflush(stdout);
    fclose(stdout);
  }

  b = C_basicBlocks(sl);  // after trace scheduling, rebuild the basic blocks

  return b;
}

static AS_instrList llvmInsSelect(T_funcDecl func, struct C_block b,
                                  string file_ins, bool isPrint) {
  // First making the head of the function, then the body, then the epilog
  AS_instrList prologil = llvmprolog(func->name, func->args, func->ret_type);
  AS_blockList bodybl = NULL;
  for (C_stmListList sll = b.stmLists; sll; sll = sll->tail) {
    AS_instrList bil = llvmbody(sll->head);
    AS_blockList bbl = AS_BlockList(AS_Block(bil), NULL);
    bodybl = AS_BlockSplice(bodybl, bbl);
  }
  AS_instrList epilogil = llvmepilog(b.label);

  G_nodeList bg = Create_bg(bodybl);  // CFG
  if (isPrint) {
    freopen(file_ins, "a", stdout);
    fprintf(stdout, "\n------For function ----- %s\n\n", func->name);
    fprintf(stdout, "------Basic Block Graph---------\n");
    Show_bg(stdout, bg);
    fflush(stdout);
    fclose(stdout);
  }

  // put all the blocks into one AS list
  AS_instrList il = AS_traceSchedule(bodybl, prologil, epilogil, FALSE);

  if (isPrint) {
    freopen(file_ins, "a", stdout);
    printf("------~Final traced AS instructions ---------\n");
    AS_printInstrList(stdout, il, Temp_name());
    fflush(stdout);
    fclose(stdout);
  }

  return il;
}

static G_nodeList livenessAnalyze(AS_instrList finalarm, string file_ig) {
  // rebuild liveness graph
  G_graph arm_fg = FG_AssemFlowGraph(finalarm);
  freopen(file_ig, "a", stdout);
  fprintf(stdout, "------Flow Graph------\n");
  fflush(stdout);
  G_show(stdout, G_nodes(arm_fg), (void*)FG_show);
  fflush(stdout);
  fclose(stdout);

  G_nodeList arm_lg = Liveness(G_nodes(arm_fg));
  freopen(file_ig, "a", stdout);
  fprintf(stdout, "/* ------Liveness Graph------*/\n");
  Show_Liveness(stdout, arm_lg);
  fflush(stdout);
  fclose(stdout);

  // create interference graph
  G_nodeList arm_ig = Create_ig(arm_lg);
  freopen(file_ig, "a", stdout);
  fprintf(stdout, "------Interference Graph------\n");
  Show_ig(stdout, arm_ig);
  fflush(stdout);
  fclose(stdout);

  return arm_ig;
}
