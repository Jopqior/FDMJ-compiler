# Assignment #1

See eLearning for more details and about submission.

1. **You need to add** the code to the main directory so to implement the `AS_generateLLVMcode(stdout, root)` function (used in the `main.c`), which should walk down the AST tree from `root` to generate the LLVM code. The code will be run (with the given test sources as well as other test cases) when your work is graded.

1. **You need to change** the grammar so the source code can allow "`putint(e1, e2)`" in additon to "`putint(e)`". ***BUT: you are not allowed to change the AST tree! Think how to parse to the same AST tree.*** That is, you are only to change lexer.lex and parser.yacc to accomplish this change to the source file.

1. The `main` program parses an input FDMJ SLP program, and generates an AST tree (see main.c). To use it: `${main_dir}/main output_filename < source_file`

1. There are two "printers" you may use: one is to print back the fdmj source file back, or the XML representation of the AST tree (so we can browse/visualize the AST tree). You are free to change the printer to be used.

1. The LLVM IR instructions to use is as follows:
```
%x = op i64 X, Y
call void @putint(i64 X)
call void @putch(i64 X)
```
where (1) x is an positive integer number that can only be used once on the left of the instructions (you may use them as many times as you wish on the right); (2) X and Y are either an integer number, or a %y where y has appeared on the left hand side of an earlier instruction, (3)