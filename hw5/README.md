# Homework Assignment #5

You need to write a semantic checker for FDMJ2024 programs. The semantic rules are discussed in class.  **In this assignment hw5,  you only need to check FDMJ programs WITHOUT classes.**

Modify the semant.c code under the lib/frontend to implement the semantic checker program. Your semant.c code is given the AST (with the root A_prog). The AST is read from an XML file (printed using print_ast.c under lib/utils/printer).

The test AST files are under tests (with the corresponding fmj file). If you want to generate more test cases, you may: (1) hand-write your own AST in the XML form, (2) use your own parser (from hw4) and the print_ast.c code, or (3) use vendor/tools/fmj2ast to use TA's program to generate AST in XML (see Makefile for usage)

As usual, hand in your code using "make handin" and submit the package using eLearning.
