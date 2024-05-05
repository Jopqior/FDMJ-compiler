# Homework Assignment #11

Translation of AS_instrList with LLVM IR mini instructions into SSA form. 

1) You are to implement the ssa.c program under the lib/optimization folder. 

2) he input is supposed to be by the main.c program (under vendor/tools) from xml files for instructions (\*.6.ins, with the more readable form in \*.5.inc), which are generated via the programs under vendor/tools. (You may try to use the instructions from your own "llvmgen.c" code. In this case, lib/util/printer/print_ins.c may be used to print the instructions in the xml form to be read by the above main.c.) 

3) Some test fmj files are under hw11/test. The output are in \*.7.cfg and \*.8.ssa. See Makefile for more details.

4) Your submission will be graded by inspecting the \*.7.cfg and \*.8.ssa files from some test fmj sources. **Some of fmj sources should be directly executable with make test-run. These will be used as part of the grading.**

As usual, do all your work with the class code repository and submit to elearning the materials packaged with "make handin".