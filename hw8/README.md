# Homework Assignment #8

Translate AST to TigerIR+ (part II)

This assignment translates FDMJ AST ( **WITH**  arrays and classes) to TigerIR+. Files are under repo/hw8.

1) Some test fmj files are under hw8/test. The Makefile uses the given "vendor/tools/fmj2ast" program to generate ast files in XML format to be used for your translation code (see tools/main.c).

2) You are supposed to supply the code for transA_Prog() in the lib/frontend/semant.c file, which is currently made to return a correct IRP tree for testing purposes (you should remove this IR tree when you are done).

2) The tools/main.c code includes the linearization and tracing parts on your final IR tree. You may check the IR-tree printout (in file *.3.irp) and the printout after linearization and tracing (in file *.4.stm).

As usual, do all your work with the class code repository and submit to elearning the materials packaged with "make handin".
