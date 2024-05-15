# lab12: arm tiling

### 1. What to do

(1) Eliminate  phi function; Since arm has no phi function, if we want to convert our assem into arm form, we must eliminate all the phifunction. (2) According to the assem, judge the assem type and convert them into arm form. (3) Due to the differences in function calling conventions between ARM and LLVM, we need to consider how to pass arguments. Additionally, we need to consider caller-saved registers and callee-saved registers in this section.



### 2. Remove phifunction

When we encounter the phi function, we need to eliminate the phi function while ensuring the correctness of the function itself (a reference idea is to add an assignment at the end of all the predecessors of the node, so as to ensure the overall accuracy) There are many ways to eliminate the phi function, you can use whatever you want to eliminate phifunction



### 3. Tiling

We need to convert the instructions from llvm form to arm form. You can refer to the given code to judge and assist everyone in your work.



### 4. Register

+ When passing parameters, if the integer is less than four, we can directly transfer it in the register; if the floating point number is less than 16, we can also directly transfer it in the register(which are caller saved register). If this number is more than that, we need to store them on the stack
+ Relevant knowledge can be viewed from the following link: [Trape frame](https://zhuanlan.zhihu.com/p/651171419) .And we need to do caller saved and callee saved work.