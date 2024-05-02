# lab9_10: llvm tiling

### 1. What to do

(1) For each of the LLVM IR mini instructions, generate a set of tiles that may be useful for IR Tree. And (2) then use your tiles to “tile” the instructions



### 2. LLVM IR

#### Overview

The purpose of this part is to direct you to the subset of LLVM IR instructions that's going to be used in our class. The full LLVM IR documentation can be found in the

[LLVM Language Reference](https://releases.llvm.org/3.5.0/docs/LangRef.html)

In this set of instructions, the data types used are integer type `i32`, `double`, `i1`(used for comparison instructions)

#### Notation 

+ `T` stands for `i64` , `double` or `ptr` (ptr is “opaque pointer”, and when needed, use `T*`). 
+ `BOP` stands for one of the binary operations: `add`, `sub`, `mul`, and `sdiv`. (For float, using `fadd`,`fsub`,`fmul`,`fdiv`)
+ `CND` stands for one of the comparison conditions: `eq`, `ne`,``sgt` (`s` stands for "signed"), `sge`, `slt`, and `sle`. 
+  `OP` can be a constant or a local, or when it’s a `ptr` type, some pointe manipulation of LLVM IR can be done. See below. 
+ ` %L` stands for any local.

#### Instructions 

| Instruction                                      |                          Meaning                          |
| ------------------------------------------------ | :-------------------------------------------------------: |
| `%L = BOP T OP1, OP2`                            |   perform `BOP` on `OP1` and `OP2` and store into `%L`    |
| `%L = alloca T, align 4`                         |   allocate stack space, and returns the address to `%L`   |
| `%L = load T, ptr OP`                            |  load the value at the location OP, with return type `T`  |
| `store T OP1, ptr OP2`                           |         store value `OP1` into the location `OP2`         |
| `%L = icmp(or fcmp) CND i64 OP1, OP2`            | compare two numbers and return the one-bit result to `%L` |
| `br i1 <cond>, label <iftrue> , label <iffalse>` |   Conditional branch, where \<cond> is a one-bit value    |
| `br label <dest> `                               |                   Unconditional branch                    |
| `%L = call T1 OP1(T2 OP2, ..., Tn OPN)`          |           call the function and return a value            |
| `call void OP1(T2 OP2, ... ,TN OPN) `            |                     call the function                     |
| `%L = ptrtoint i64 OP to ptr`                    |     convert an integer OP to a ptr and return to `%L`     |
| `%L = inttoptr ptr OP to i64`                    |     convert an integer OP to a ptr and return to `%L`     |
| `%L = sitofp i64 OP to double`                   |                 convert OP(int) to double                 |
| `%L = fptosi double OP to i64`                   |                 convert OP(double) to int                 |

#### Other instructions and structures 

In our class, we will use other instructions and structures as necessary, including for example, function declarations, external functions and so on. In fact, as long as your llvm program can run, you can use any instruction you want.



### 3. Tips

+ In llvm(and arm), we can only perform calculations on two numbers of the same type (which means we can’t add i64 and double). Therefore, it is recommended to add a `T_Cast` operation to the previous `binop` , `cjump` operations to help everyone determine which operand to use.

+ The assistant's code generation results are relatively simple and are provided for reference only. It is hoped that students can, according to what was discussed in class, find better tiling as much as possible, which can generate more efficient code. This will also partially affect the grading.