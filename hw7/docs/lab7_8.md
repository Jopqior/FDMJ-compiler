# lab7_8: Translate AST2IRP

# Tiger IR+ (IRP) 的设计

> tiger以exp为主体，exp里可以有stm
>
> FDMJ是以stm为主体，里边可以有exp
>
> 我们在Tiger IR的基础上加了点东西变成Tiger IR+使之适配FDMJ

T_funcDecl、T_funcDeclList：程序以function为单位，程序用一系列function表示（a list of functions），第一个是main（约定）；每个function有函数名、参数（Temp_tempList的形式，之后到机器语言里会变成一堆栈）、语句（T_stm的形式；变量没有声明，都是Temp_temp的形式，temp可以认为是reg，有value/pointer和location，在IR里有无限个）。在FDMJ中，method和function是一个概念，在翻译过程中会消除class的概念，提取method，重命名表示不同类的method

T_stm：seq用来拼接T_stm，使之称为一串T_stm（等价于stmList，但建议用seq，更简洁）；label、jump、cjump和跳转有关（label是jump的地方，类似于goto标签的作用）；move用来赋值；exp用来忽略T_exp的返回值让exp变成stm；return为函数返回

T_exp：temp和const分别转换id和num；binop用于二元运算；call和extcall分别用来调用自定义函数与libsysy函数；eseq用来给T_stm加上返回值让stm变成exp；mem用于操控内存，比如修改数组某位置的值；cast用于T_int和T_float的类型转换。每个T_exp都有类型，可能是T_int类型（int、array、class，后两者都转换为地址所以也是int），也可能是T_float类型（float）。

# Translate的方法

## 策略

一边做类型检查（Type Checking）一边翻译（Translate）

- 类型检查在transA_XXX的函数体，即为hw5&hw6的实现（semant.h|c）
- 翻译在transA_XXX的return部分，通过调用专用的翻译函数Tr_XXX实现（translate.h|c）

## Tr_exp: semant和translate的接口

在确定semant和translate之间的接口时，一种较好的规则是：semant模块不应当包含任何对T_stm和T_exp的直接引用，任何对IR的操作都应当由Translate来实现。

按照虎书上的描述，设计Tr_exp，用以将semant和translate模块解耦合（translate）

- Ex代表有返回值的exp，包装了T_exp
- Nx代表无返回值的stm，包装了T_stm
- Cx代表条件语句，包装了T_stm，以及真假label的两个patchList

用Tr_Ex, Tr_Nx, Tr_Cx将T_exp, T_stm, Cx包装为Tr_exp（translate）

用unEx, unNx, unCx将Tr_exp转换为T_exp, T_stm, Cx，即实现3类Tr_exp的相互转换（translate）

- unEx
  - Tr_ex直接返回其中的T_exp
  - Tr_nx用T_Escq返回0，这是自定义的返回值，没有实际意义
  - Tr_cx会填充patchList，并利用T_Escq根据真假label返回1或0
- unNx
  - Ex用T_Exp包装其中的T_exp返回
  - Tr_nx直接返回其中的T_stm
  - Tr_cx直接返回cx中的stm
- unCx
  - Tr_ex用T_Cjump根据其中的T_exp是否等于0占位真假patchList返回Cx
  - Tr_nx无法包装为Cx，会报错
  - Tr_cx直接返回其中的Cx

## hw7: 不带class

关键在于运算、赋值、条件、数组四部分，pathList细节请见虎书

运算

- 算术运算（+, -, ...）：包装成T_Binop，返回Tr_Ex
- 比较运算（>, <, ...）：包装成T_Cjump，其中真假label的patchList待填充，返回Tr_Cx
  - or：填充第一个Exp的false label为next，将两个Exp的true label给join起来待填充，第二个Exp的false label也待填充，返回Tr_Cx
  - and：填充第一个Exp的true label为next，将两个Exp的false label给join起来待填充，第二个Exp的true label也待填充，返回Tr_Cx
  - 取反运算：使用unCx获得一个Cx，交换其真假label的patchList，返回Tr_Cx

赋值

- 使用unEx获得一个T_exp然后赋值

条件

- if：
  - 使用unCx获得一个Cx，然后对patchList进行填充，据是否有else返回不同的语句
- while：
  - 使用unCx获得一个Cx，然后对patchList进行填充，据是否有loop返回不同的语句
  - 在semant模块中会维护whiletests和whileends两个标签栈，以检查continue和break是否在while中

数组

- 初始化：int[] a={ConstList}
  - 调用malloc分配空间，在-1位置放上长度，然后通过计算偏移并取Mem为某位置赋值
- 赋值：a[]={expList}，a=new int[exp]
  - 同上
- 存储：exp[exp]
  - 通过计算偏移并取Mem获取某位置
- 长度：length(exp)
  - 取-1位置获得长度

## hw8: 考虑class

### method列表

- 在构建method列表时，main method为首，class method跟随其后
- 命名
  - main method名为main
  - 假设class名为c，c中有一个method名为m，则该class method名为c$m
- 参数列表
  - main method的参数列表为空
  - 所有class method的参数列表中，第一个参数统一为t99，表示this，剩下的为定义的参数
- 在遍历所有method之前，都会对temp的计数进行重置（所有S_endScope(venv)之后），因此所有method的temp都是从t100开始的（除了class method的第一个参数this为t99）。之所以这么做，是因为method之间互不影响，所以之前temp是可以复用的，阅读起来更好看。

### Unified Object Record

- 起初，偏移表（S_table）varoff和methoff为空，globaloff为0
- 预处理时（transA_xxx_basic函数）
  - 每遇到一个var，如果varoff中没有此id，则记录映射id->globaloff到varoff，然后globaloff递增
  - 每遇到一个meth，如果methoff中没有此id，则记录映射id->globaloff到methoff，然后globaloff递增
- 最终，globaloff为所有class的所有var和method的数量（var同名只算一次，meth同名只算一次），varoff和methoff记录相应var和meth的偏移量（所有偏移的单位都是arch_size*8，比如arch_size=8时表示64位机器）

### class主要有3种操作

new类

- 先新建一个temp，为其分配globaloff大小的空间，然后根据id遍历其vtbl和mtbl，进行必要的初始化
- 由于在vtbl和mtbl中记录了A_varDecl，因此容易对变量进行初始化
- 由于在mtbl中记录了method的类来源from，因此容易对方法m进行初始化为from$m以实现多态（多态在FDMJ里便从运行时提到了编译时）

类变量

- 通过varoff找到var的位置

类方法

- 通过methoff找到meth的位置，然后进行函数调用，传参数时需要额外传递obj的temp

# tips

- T_Name(label)和T_Label(label)都是包装label，但前者是exp后者是stm；T_Temp(temp)返回exp，包装temp
- T_Mem(exp)返回exp，会在IR转换为机器语言时暗示生成存取指令
- T_ExtCall是调用libsysy的方法，T_Call是调用自定义的类方法（其第一个参数是m本身有点冗余了，因为第二个参数是c#m已经实现了多态）











