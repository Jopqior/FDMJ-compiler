# Homework Assignment #2

The second homework is a written one, with some coding.

1. Pen & paper assignment:  Do the exercises 2.4(b), 2.5(c), 3.3(b,d), 3.4, 3.12(a, b) in the textbook. (Note that the example given in 3.3(d) in the textbook is wrong. It should be “[([])()[(][])]”.)

2. For exercise 2.4(b), 2.5(c), write the lex programs, respectively, that will accept exactly those strings that are described in them.

3. For 3.3(d), write the yacc program that will accept exactly those described strings.

## Note1

3.3(d) 说明如下。本题的说明是：Balanced parentheses and brackets, where a closing bracket also closes any outstanding open parentheses (up to the previous open bracket). 理解一下：

1.先考虑什么是Balanced parentheses and brackets，应该指的是一个open parenthesis需要有一个close parenthesis对应，而（这个很重要）在这一对parenthesis/brackets之间，所有parentheses和brackets都已经balance掉了。
2.在本题目的场景下，说是一个close bracket可以去close掉“所有open parentheses（可以0或多个），直到它找到对应的open bracket。所以课本给的例子：“[([](()[(][])]”是对的。我们可以这样看一下：第3-4个符号[]不影响任何东西，所以可以先“去掉”，得出：[((()[(][])]，其中第4-5个符号互相balance掉，得出：[(([(][])]，然后这个里面的第6个符号（第一个close）是个bracket，它可以close掉所有open parentheses，所以我们把第5个符号（是一个open parenthesis）“吃掉”并balance out前面一个open bracket，得出[(([])]，之后又有“平常意义下“balance的brackets和parentheses，得出[(]。最后一个close bracket把open parenthesis“吃掉”并与最早那个bracket平衡掉。所以这个例子是对的。
3.课本给的Hint: First, make the language of balanced parentheses and brackets, where extra open parentheses are allowed; then make sure this nonterminal must appear within brackets. 这个hint说的是可以用一个nonterminal产出任何个open parentheses，只要他们被两个平衡的brackets包含住即可。

## Note2

You should scan your written solutions and put into the report.pdf (under your doc directory), and use "make handin" to wrap everything into a zip file to submit on elearning.

Note that 2.4(b), 2.5(c), 3.3(d) need you to write programs with lex and yacc. Do:

* Put all your code under "hw2/tools" directory.
* Add necessary CMakefile under "hw2" and "hw2/tools" directories.
* Change the Makefile under "hw2"

So that you may use "make test" command under hw2 to execute all your programs to test all the corresponding test cases under "hw2/test" directory.

补充：每道题一个文件夹，其中Makefile文件应有 `make build`、`make test`、 `make test`命令，供主文件夹里Makefile使用（注意修改主文件里的Makefile）
