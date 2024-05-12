# Homework Assignment #12

Translation of LLVM IR mini instructions into RPi Mini instructions.

1. Continue from hw9_10 (and hw11). That is, given the instructions (AS\_\*) in LLVM IR mini, translate to instructions (still in AS\_\*) in RPi Mini.
    * Special care is needed for the function declaration and the return and phi-function instructions. 
	*  Other instructions are mostly straightforward, but there is a need to parse the "assem" string in the AS_* structure.
2. For each input source, your program should generate *.9.rpi printout in the same style as in hw9_10 and hw11.

3. Use the test fmj files under hw11/test.

4. Your submission will be graded by inspecting the \*.9.rpi.

As usual, do all your work with the class code repository and submit to elearning the materials packaged with "make handin".