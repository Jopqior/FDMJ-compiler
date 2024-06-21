# FDMJ编译器

本项目是一个基于FDMJ语言的编译器，FDMJ是一种miniJava语言，为简化编译器的实现而设计。目前支持将FDMJ语言编译为 LLVM IR (64 bit) 和 RPi (32 bit) 两种目标指令集运行。

## 编译方法

运行 `make build` 编译项目，生成可执行文件用于编译FDMJ语言。

需要编译的源文件放在 `test` 目录下。

运行 `make compile` 会将 `test` 目录下的所有源文件编译为 LLVM IR 和 RPi 两种目标指令集的汇编代码。

运行 `make clean` 清理 `test` 目录下编译生成的文件。

运行 `make veryclean` 清理 `build` 目录 (项目可执行文件) 和 `test` 目录下编译生成的文件。

运行 `make rebuild` 等价于 `make veryclean && make build`。

## 运行方法

### LLVM IR

运行 `make run-llvm` 会使用 `lli` 运行 `test` 目录下的所有 LLVM IR 可执行的文件。

### RPi

运行 `make run-rpi` 会使用 `qemu-arm` 运行 `test` 目录下的所有 RPi 可执行的文件。


## 输出文件

编译生成的文件会放在 `test` 目录下，以源文件名为前缀，具体如下：

1. `*.1.src`：经过词法和语法分析后得到AST，由AST重新构建的源码。
2. `*.2.ast`：xml形式的AST。
3. `*.3.irp`：便于阅读的IR+树。
4. `*.4.stm`：规范化后的IR，包括线性化IR树、基本块和规范化完成的最终IR结果。
5. `*.5.ins`：LLVM指令选择后生成的LLVM指令。
6. `*.6.cfg`：在SSA之前所需要的活跃分析结果，包括CFG、活跃分析结果和block CFG。
7. `*.7.ssa`：转换为SSA形式的LLVM代码，可以链接外部库 `libsysy64` 后运行。
8. `*.8.arm`：RPi指令选择后生成的RPi指令。
9. `*.9.ig`：活跃分析结果，包括CFG、活跃分析结果和冲突图。
10. `*.10.s`：寄存器分配后的最终结果，可以链接外部库 `libsysy32` 后运行。
11. `*.ll`：在链接外部库后，lli可执行形式的LLVM代码。
12. `*.s`：在链接外部库后，可执行形式的RPi程序。

其中从3到7的输出文件中，`ARCH_SIZE` 都为8；从8到10的输出文件中，`ARCH_SIZE` 都为4。

由于64位和32位的程序仅有关于 `ARCH_SIZE` 的几个数字不同，所以没有再为32位专门输出IR+树、规范化后的IR、转换为SSA形式前后的LLVM代码。
