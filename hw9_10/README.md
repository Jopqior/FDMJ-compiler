# Homework Assignment #9_10

Translation of TigerIR+ into AS_instrList with LLVM IR mini instructions.

This assignment is for instruction selection, that translates a TigerIR+ tree (i.e., a FuncDeclList) in an XML file into LLVM IR mini code in the form of AS_InstrList **for each basic block**.

The input should be a TigerIR+ XML file, generated from fmj2ast+ast2irp code given under hw9_10/vendor/tools.

1) Some test fmj files are under hw9_10/test. The Makefile uses the given "hw9_10/vendor/tools/fmj2ast" and "hw9_10/vendor/tools/ast2irp" programs to generate TigerIR+ files in XML format (with the file name *.3.irp) to be used for your translation code (see hw9_10/tools/main.c). You may generate more files for testing.

2) The hw9_10/tools/main.c file includes the code for linearization and tracing of the input TigerIR+ tree. You may check the Tiger IR+ tree printout (in file \*.3.irp) and the printout after linearization and tracing (in file \*.4.stm). Also, the file contains the code that prints the result of your instruction selection program (in the file \*.5.ins).

3) You are supposed to supply the code for llvmbody() in the lib/backend/llvm/llvmgen.c file. This function is to translate a **basic block** of TigerIR+ tree into an AS_instrList. Currently, the function returns a correct AS_instrList for testing purposes (you should remove this AS_instrList when you are done).

As usual, do all your work with the class code repository and submit to elearning the materials packaged with "make handin".