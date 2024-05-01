#include "util.h"
#include "temp.h"
#include "llvmgen.h"

#define LLVMGEN_DEBUG
#undef LLVMGEN_DEBUG

static AS_instrList iList = NULL, last = NULL; //These are for collecting the AS instructions into a list (i.e., iList). 
                                               //last is the last instruction in ilist
static void emit(AS_instr inst) {
  if (last) {
    last = last->tail = AS_InstrList(inst, NULL); //add the instruction to the (nonempty) ilist to the end
  } else {
    last = iList = AS_InstrList(inst, NULL); //if this is the first instruction, make it the head of the list
  }
}

static Temp_tempList TL(Temp_temp t, Temp_tempList tl) {
  return Temp_TempList(t, tl);
}

static Temp_tempList TLS(Temp_tempList a, Temp_tempList b) {
  return Temp_TempListSplice(a, b);
}

static Temp_labelList LL(Temp_label l, Temp_labelList ll) {
  return Temp_LabelList(l, ll);
}

/* ********************************************************/
/* YOU ARE TO IMPLEMENT THE FOLLOWING FUNCTION FOR HW9_10 */
/* ********************************************************/

AS_instrList llvmbody(T_stmList stmList) {
  if (!stmList) return NULL;
  iList = last = NULL;


  // **The following is to make up an example of the instruction selection result.  
  //   You are supposed to implement this function. When you are done, remove the following code
  //   Follow the instruction from the class and the book!
  Temp_label l = Temp_newlabel();
  string rd=checked_malloc(IR_MAXLEN); sprintf(rd, "%s:", Temp_labelstring(l));
  emit(AS_Label(rd,l)); 
  emit(AS_Oper("\%d0 = add i64 \%`s0, \%`s1", TL(Temp_newtemp(T_int), NULL), TL(Temp_newtemp(T_int), TL(Temp_newtemp(T_int), NULL)), NULL));
  emit(AS_Oper("br label \%`j0", NULL, NULL, AS_Targets(LL(l, NULL))));
  /***** The above is to be removed! *****/

  return iList;
}
/* ********************************************************/
/* YOU ARE TO IMPLEMENT THE ABOVE FUNCTION FOR HW9_10 */
/* ********************************************************/

/* The following are some auxiliary functions to be used by the main */
// This function is to make the beginning of the function that jumps to the beginning label (lbeg) 
// of a block (in case the lbeg is NOT at the beginning of the block).
AS_instrList llvmbodybeg(Temp_label lbeg) {
  if (!lbeg) return NULL;
  iList = last = NULL;
  Temp_label lstart = Temp_newlabel();
  string ir = (string) checked_malloc(IR_MAXLEN);
  sprintf(ir, "%s:", Temp_labelstring(lstart));
  emit(AS_Label(ir, lstart));
  ir = (string) checked_malloc(IR_MAXLEN);
  sprintf(ir, "br label %%`j0");
  emit(AS_Oper(ir, NULL, NULL, AS_Targets(LL(lbeg, NULL))));
  return iList;
}

// This function is to make the prolog of the function that takes the method name and the arguments
// WE ARE MISSING THE RETURN TYPE in tigherirp.h. YOU NEED TO ADD IT!
AS_instrList llvmprolog(string methodname, Temp_tempList args, T_type rettype) {
#ifdef LLVMGEN_DEBUG
  fprintf(stderr, "llvmprolog: methodname=%s, rettype=%d\n", methodname, rettype);
#endif
  if (!methodname) return NULL;
  iList = last = NULL;
  string ir = (string) checked_malloc(IR_MAXLEN);
  if (rettype == T_int)
    sprintf(ir, "define i64 @%s(", methodname);
  else if (rettype == T_float)
    sprintf(ir, "define double @%s(", methodname);
#ifdef LLVMGEN_DEBUG
    fprintf(stderr, "llvmprolog: ir=%s\n", ir);
#endif
  if (args) {
    int i = 0;
    for (Temp_tempList arg = args; arg; arg = arg->tail, i += 1) {
      if (i!=0) sprintf(ir+strlen(ir), ", "); 
      if (arg->head->type == T_int) {
        sprintf(ir+strlen(ir), "i64 %%`s%d", i);
#ifdef LLVMGEN_DEBUG
        fprintf(stderr, "%d, llvmprolog: ir=%s\n", i, ir);
#endif  
      }
      else if (arg->head->type == T_float) {
#ifdef LLVMGEN_DEBUG
        fprintf(stderr, "%d, llvmprolog: ir=%s\n", i, ir);
#endif
        sprintf(ir+strlen(ir), "double %%`s%d", i);
      }
#ifdef LLVMGEN_DEBUG
      fprintf(stderr, "llvmprolog args: arg=%s\n", ir);
#endif
    }
  }
  sprintf(ir+strlen(ir), ") {");
#ifdef LLVMGEN_DEBUG
  fprintf(stderr, "llvmprolog final: ir=%s\n", ir);
#endif
  emit(AS_Oper(ir, NULL, args, NULL));
  return iList;
}

// This function is to make the epilog of the function that jumps to the end label (lend) of a block
AS_instrList llvmepilog(Temp_label lend) {
  if (!lend) return NULL;
  iList = last = NULL;
  // string ir = (string) checked_malloc(IR_MAXLEN);
  // sprintf(ir, "%s:", Temp_labelstring(lend));
  // emit(AS_Label(ir, lend));
  // emit(AS_Oper("ret i64 -1", NULL, NULL, NULL));
  emit(AS_Oper("}", NULL, NULL, NULL));
  return iList;
}