# Homework Assignment #11

Translation of AS_instrList with LLVM IR mini instructions into SSA form. 

1) You are to implement the ssa.c program under the lib/optimization folder. 

2) The input to the main.c program (under vendor/tools) is an xml file for LLVM IR instructions (\*.6.ins, with the more readable form in \*.5.inc), which are generated via the programs under vendor/tools. (The grading will be done on the files from the given tools. However, you are encouraged to use the instructions from your own "llvmgen.c" instruction selection code. In this case, lib/util/printer/print_ins.c may be used to print the LLVM IR instructions in the xml form to be read by the above main.c.) 

3) Some test fmj files are under hw11/test. The outputs are in \*.7.cfg and \*.8.ssa. See Makefile for more details.

4) Your submission will be graded by inspecting the \*.7.cfg and \*.8.ssa files from some test fmj sources. Also, "make test-run" will be used to run the LLVM IR programs (they should all be executable now that they are in SSA).

As usual, do all your work with the class code repository and submit to elearning the materials packaged with "make handin".