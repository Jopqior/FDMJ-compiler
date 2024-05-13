# Homework Assignment #12

Translation of LLVM IR mini instructions into RPi Mini instructions.

1.  Continue with HW11. That is, given the instructions (AS\_\*) in LLVM IR mini (given as \*.6.ins files), translate into ssa form (this was HW11), and then translate to instructions (still in AS\_\*) in RPi Mini.

    * Special care is needed for the function declaration and the return and phi-function instructions. 
	*  Other instructions are mostly straightforward, but there is a need to parse the "assem" string in the AS_* structure.

2. For each input source (\*.6.ins), your program should generate *.9.rpi file in the same style as in HW11.

3. Copy files from hw11 to hw12 (keep the same code structure!). Use the same test fmj files as for hw11.

4. Your submission will be graded by inspecting the \*.9.rpi (mostly by comparing the \*.8.ssa and \*.9.rpi files).

As usual, do all your work with the class code repository and submit to elearning the materials packaged with "make handin".