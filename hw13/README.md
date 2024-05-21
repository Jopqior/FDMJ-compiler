# Homework Assignment #13

Do register allocation on RPi Mini instructions.

1.  Continue with HW12. That is, given the instructions (AS\_\*) in RPi mini, allocate register for the all the temps.

    * Special care is needed for maintaining the call conventions of RPi regarding their use of registers (including caller/callee saved registers convention and the parameter passing convention). 

2. For each input source (\*.6.ins), your program should generate *.10.rpi file in the same style as in HW12, but all temps are now regiters.

3. Copy files from hw11 to hw13 (keep the same code structure!). Use the same test fmj files as for hw11.

4. Your submission will be graded by inspecting the \*.10.rpi and executed with qemu for ARM (RPi).
  
As usual, do all your work with the class code repository and submit to elearning the materials packaged with "make handin".