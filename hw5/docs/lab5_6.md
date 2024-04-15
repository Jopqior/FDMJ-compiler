# Type Checking

## Types

### Data Type (3)

> FDMJ is static typed

- basic type (integer|float)
- array type (of integers|floats)
- class type (also called an object type, with a class name)
  - class variables
    - Class-wide variables
      - Using static keyword (e.g. class c {static int a;…}) in Java
      - **FDMJ don’t have these**
    - Object (instance) variables
      - Act more like a record (e.g. class c {int a;}) in Java
      - **FDMJ only have public object variables**
  - methods 
    - Java methods also have class level or object level (static method; Non-static method)
    - **FDMJ only have non-static public methods**
  - Inheritance
    - Java
      - A sub-class inherits the variables and the methods of the (super) class. This inheritance relationship is transitive.
      - Can override (public) variables and methods
        - variables: overridden by same name. 
        - methods: overridden by the same name and signature (polymorphism)
    - **FDMJ**
      - 子类继承父类的变量，但不能重复声明同名变量
      - 子类继承父类的方法，可以重写方法的实现，但方法的签名(返回值和参数列表)不能改变
      - 只能单继承，不允许multiple；不允许成环

### Value Type (4)

> In FDMJ “everything represents a value”, useful in type checking 

- basic value (abbreviated as “value”)
  - 只有int或float的变量或常量才有value
  - 变量如：int a; float b;
  - 常量如：1; 2.3;
- pointer value (abbreviated as “pointer”)
  - 只有数组或类的对象或匿名对象才有pointer
  - 对象如：int[] a;（数组对象），class C;（类对象）
  - 匿名对象如：{1, 2, 3}（匿名数组对象），new C()（匿名类对象）
- location value (of all data types, abbreviated as “location”)
  - For a memory location (either in the stack or in heap space). 
  - Each location is dedicated for a specific data type and holds 32 bits (for this class).
  - 只有左值（变量或对象）有location，右值（常量或匿名对象）没有location
  - 但注意匿名对象中的变量是左值（如new C().obj = 2;，虽然这样做没有意义，但是是合法的）
- method value
  - Representing function entrance location (instruction space)
  - Note we don’t have goto statements in FDMJ.

### Name Type (3)

>  FDMJ uses name equivalence 

1. variable name: 
   - Associated with 1 of the three data types
   - Carries 2 values: a location value and an basic/pointer value (if defined)
   - A variable name stands for either: (e.g. variable name is a)
     - a basic type has location value and integer value (e.g. int a;)
     - an array type has location value and pointer value (e.g. int[] a;)
     - a class type has location value and pointer value (e.g. class A a;)
   - A variable is only in the scope where it is declared
     - In a method, or （包括传入的，和声明的；FDMJ传值(integer/pointer)，不传引用(location)）
     - In a class
2. class name
   - Is an alias for class type
   - Is not associated with any value
   - A class name is used for defining either: (e.g. class name is A)
     - a variable of a class type (e.g. “class A o;”)
     - an anonymous variable which only has pointer value (e.g. “new A()”)
3. method name:
   - Is a pointer value, which is the “address” of a “function” (or a method). No location value is associated. 
   - The exact “address” of a method 
     - depends on the object on which it is called (also based on the inheritance hierarchy). 
     - Or it’s determined at link time (if it’s an external function).

### Casting

> FDMJ allow implicit basic type casting and class type upcasting during assignment

- implicit basic type casting
  - 隐式类型转换，比如int a = 1.1 + 2;最终a赋值为3
- class type upcasting (inheritance)!
  - upcasting相当于丢掉子类的一些性质变成父类
  - 比如，允许类赋值操作：Father o = new Son();
  - o的location和value都是Father（比如float f = i; 整型放到了float的location和value里了）
  - 因此，此后o只能调用父类中存在的变量和方法；但如果调用了子类重写的方法，则调用子类的实现
  - 在typechecking里，我们不管它调用什么函数，只检查右边是否是左边的类型或左边的孩子
  - 那我们怎么o.m()知道调用是父类还是子类的方法实现呢？在赋值时，entry会记录method到底指向父类还是子类的method实现。这是runtime做的事情（dynamic method）

## Rules

```java
Prog -> MainMethod ClassDeclList
  MainMethod -> public int main '(' ')' '{' VarDeclList StmList '}'
    VarDeclList -> ε | VarDecl VarDeclList
      VarDecl -> class id id ';' // id <=> [a-z_A-Z][a-z_A-Z0-9]*
               | int id ';' | int id '=' Const ';' 
               | int '[' ']' id ';' | int '[' ']' id '=' '{' ConstList '}' ';'
               | float id ';' | float id '=' Const ';' 
               | float '[' ']' id ';' | float '[' ']' id '=' '{' ConstList '}' ';'
        Const -> NUM | '-' NUM // NUM <=> [1-9][0-9]*|0|[1-9][0-9]*\.[0-9]*|0\.[0-9]*|[1-9][0-9]*\.|0\.|\.[0-9]*
        ConstList -> ε | Const ConstRest
          ConstRest -> ε | ',' Const ConstRest
    StmList -> ε | Stm StmList
      Stm -> '{' StmList '}' 
           | if '(' Exp ')' Stm else Stm | if '(' Exp ')' Stm // 检查Exp为int或float 
           | while '(' Exp ')' Stm | while '(' Exp ')' ';' // 检查Exp为int或float 
           | Exp '=' Exp ';' // 检查类型是否兼容，允许upcast，允许int和float之间的隐式类型转换 
           | Exp '[' ']' '=' '{' ExpList '}' ';' // 检查Exp为array，ExpList为int或float
           | Exp '.' id '(' ExpList ')' ';' // 检查Exp为class，Exp有id方法，ExpList匹配方法参数
           | continue ';' | break ';' // 检查在while里
           | return Exp ';' 
           | putnum '(' Exp ')' ';' | putch '(' Exp ')' ';' // 检查Exp为int或float
           | putarray '(' Exp ',' Exp ')' ';' // 检查第一个Exp为int或float，第二个Exp为array
           | starttime '(' ')' ';' | stoptime '(' ')' ';'
      Exp -> NUM // 检查是int还是float
           | true | false // 返回int
           | length '(' Exp ')' // 返回int，检查Exp是array
           | getnum '(' ')' // 返回float
           | getch '(' ')' // 返回int
           | getarray '(' Exp ')' // 返回int，检查Exp是array
           | id // 返回变量类型
           | this // 返回class
           | new int '[' Exp ']' | new float '[' Exp ']' // 返回array，检查Exp是int或float
           | new id '(' ')' // 返回class
           | Exp op Exp | '!' Exp | '-' Exp // 检查Exp是int或float，op当都为int时返回int否则返回float，!返回int，-返回Exp类型
           | '(' Exp ')' | '(' '{' StmList '}' Exp ')' // 返回Exp类型
           | Exp '.' id // 检查Exp为class，检查Exp是否有id，返回id类型
           | Exp '.' id '(' ExpList ')' // 检查Exp为class，检查Exp是否有id，检查ExpList是否匹配参数列表，返回方法类型
           | Exp '[' Exp ']' // 检查第一个Exp为array，第二个Exp为int或float
      ExpList -> ε | Exp ExpRest
        ExpRest -> ε | ',' Exp ExpRest
  ClassDeclList -> ε | ClassDecl ClassDeclList
    ClassDecl -> public class id '{' VarDeclList MethodDeclList '}' 
               | public class id extends id '{' VarDeclList MethodDeclList '}' // 检查继承关系
      MethodDeclList -> ε | MethodDecl MethodDeclList
      MethodDecl -> public Type id '(' FormalList ')' '{' VarDeclList StmList '}'
        Type -> class id | int | int '[' ']' | float | float '[' ']'
          FormalList -> ε | Type id FormalRest
            FormalRest -> ε | ',' Type id FormalRest
```

## Checking

### 数据结构
- Ty_ty会用到以下4种（type.h）
  - Ty_int：变量类型是整型，Ty_Int()
  - Ty_float：变量类型是浮点型，Ty_Float()
  - Ty_array：变量类型是整型数组或浮点型数组，Ty_Array(Ty_Int()) | Ty_Array(Ty_Float())
  - Ty_name：变量类型是名为class_id的类，Ty_Name(class_id)
- S_table中存储S_symbol到E_enventry的映射，E_enventry有以下3种（env.h）
  - E_varEntry：变量或常量声明
    - vd：A_varDecl类型，用于继承时如果发生重定义错误时输出pos
    - ty：Ty_ty类型，用于标识其类型，可能是Ty_int或Ty_float或Ty_array或Ty_name
  - E_classEntry：类声明
    - cd：A_classDecl类型，用于检测到循环时输出pos
    - fa：S_symbol类型，用于记录父类名
    - status：E_status类型，用于环检测
    - vtbl：S_table类型，用于存储类的变量声明
    - mtbl：S_table类型，用于存储类的方法声明
  - E_methodEntry：方法声明
    - md：A_methodDecl类型，用于继承时如果发生重定义错误时输出pos
    - from：S_symbol类型，用于记录方法的来源类名
    - ret：Ty_ty类型，用于标识返回值类型，可能是Ty_int或Ty_array或Ty_name
    - fl：Ty_fieldList类型，用于存储方法参数列表类型，由Ty_fieldList(Ty_field(var_id, var_ty), Ty_fieldList(…))构成
- 每个S_table中存储的只能是E_enventry的3种类型之一
   1. cenv：类环境（class environment）
      - class_id →  E_classEntry
      - E_classEntry中包括
        - vtbl：类变量环境（variable environment）
          - var_id →  E_varEntry
        - mtbl：类方法环境（method environment）
          - meth_id →  E_methodEntry
   2. venv：变量环境（variable environment）
      - var_id →  E_varEntry

### 流程
1. 初始阶段：初始化静态全局变量（静态是为了仅在本文件中使用，全局是为了函数少传些参数）
   1. cenv：类环境
   2. venv：变量环境
   3. MAIN_CLASS：一个dummy class的名字，是所有没有extend的类的父类
   4. cur_class_id：目前正在检查的类名，在类型检查阶段使用
   5. cur_method_id：目前正在检查的方法名，在类型检查阶段使用
2. 预处理阶段：检查并建立cenv
   1. 遍历类：
      - 记录extend关系
      - 检查类变量与方法是否重定义
      - 记录类变量与方法到cenv
   2. 遍历类：查看是否有父类，如果有，先递归做父类，然后将父类变量与方法复制到子类
      - 能够检查extend关系是否有循环
      - 如果变量与父类中的同名，报错重定义
      - 如果方法与父类中的同名，检查签名是否相同(返回值和参数列表类型)，若不同则报错
3. 类型检查阶段：检查类和主方法
   1. 遍历类：
      - 检查类变量
      - 检查类方法
   2. 检查主方法
   3. 检查方法时会遇到变量声明、stm和exp，需要特别注意
      - 每次进入一个方法S_beginScope(venv)，检查退出后S_endScope(venv)
      - 类变量、方法变量（包括方法的参数），都不能重定义
      - 检查类型为class的变量时，需要查询cenv看这个class是否存在
      - 赋值给类型为class的变量时，需要检查是否为其本身或其子类

注：类型检查到错的地方请用如下输出
```cpp
void transError(FILE *out, A_pos pos, string msg) {
  fprintf(out, "(line:%d col:%d) %s\n", pos->line, pos->pos, msg);
  fflush(out);
  exit(1);
}
```
