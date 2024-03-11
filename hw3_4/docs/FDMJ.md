# FDMJ

# Syntax

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
           | if '(' Exp ')' Stm else Stm | if '(' Exp ')' Stm 
           | while '(' Exp ')' Stm | while '(' Exp ')' ';'
           | Exp '=' Exp ';' 
           | Exp '[' ']' '=' '{' ExpList '}' ';' 
           | Exp '.' id '(' ExpList ')' ';' 
           | continue ';' | break ';' 
           | return Exp ';' 
           | putnum '(' Exp ')' ';' | putch '(' Exp ')' ';'
           | putarray '(' Exp ',' Exp ')' ';'
           | starttime '(' ')' ';' | stoptime '(' ')' ';'
      Exp -> NUM | true | false | length '(' Exp ')'
           | getnum '(' ')' | getch '(' ')'
           | getarray '(' Exp ')'
           | id | this
           | new int '[' Exp ']' | new float '[' Exp ']' | new id '(' ')'
           | Exp op Exp | '!' Exp | '-' Exp // op stands for "+, - , * , / , || , && , < , <= , > , >= , == , != "
           | '(' Exp ')' | '(' '{' StmList '}' Exp ')'
           | Exp '.' id
           | Exp '.' id '(' ExpList ')'
           | Exp '[' Exp ']'
      ExpList -> ε | Exp ExpRest
        ExpRest -> ε | ',' Exp ExpRest
  ClassDeclList -> ε | ClassDecl ClassDeclList
    ClassDecl -> public class id '{' VarDeclList MethodDeclList '}' 
               | public class id extends id '{' VarDeclList MethodDeclList '}'
      MethodDeclList -> ε | MethodDecl MethodDeclList
      MethodDecl -> public Type id '(' FormalList ')' '{' VarDeclList StmList '}'
        Type -> class id | int | int '[' ']' | float | float '[' ']'
          FormalList -> ε | Type id FormalRest
            FormalRest -> ε | ',' Type id FormalRest
```

- FDMJ2024新类型：float（FDMJ2023只有int类型）
  - 复制所有int相关的语法规则将int改为float（public int main除外）
  - 将putint与getint改为putnum与getnum，这样方便编写FDMJ程序，无需考虑输入输出的是什么类型，等到之后类型检查时再决定用libsysy里的putint还是putfloat等等
  - 无需修改Const区分int还是float，因为在数组的初始化时，可能会遇到整型和浮点型混合的情况，此时需要隐式类型转换，得等到之后类型检查时处理，所以前期没法分得太清楚，只能都存到float里边去之后处理

## Overall Structure

```java
public int main() {
  // vars
  // stms
}

public class A {
  // vars
  // methods
}

// classes
```
- Variable Declarations
  - local variable: defined in a method (including the main method)
  - class variable: defined in a class, which only “exists” with a class instance (object). A class variable `b` of class `A` can only be accessed using `this.b` inside class, using `A.b` outside class.
  - There are **no** “global” variables.
  - All arrays are in the heap memory.
- Class Inheritance
  - “extends” means subclass. 
  - A subclass inherits the class variables and methods from all its ancestor classes. 
  - However, if a variable in the subclass have the same name as its parent/ancestor, then the variable is “redefined” in the subclass.

## Comments

- `//...` in 1 line
- `/*...*/` in n line(s)
- Comments are not treated as part of the program
