# Homework Assignment #2

The second homework is a written one (see elearning), with some coding. You should scan your written solutions and put into the report.pdf (under your doc directory), and use "make handin" to wrap everything into a zip file to submit on elearning.

Note that 2.4(b), 2.5(c), 3.3(d) need you to write programs with lex and yacc. Do:

* Put all your code under "hw2/tools" directory.
* Add necessary CMakefile under "hw2" and "hw2/tools" directories.
* Change the Makefile under "hw2"

So that you may use "make test" command under hw2 to execute all your programs to test all the corresponding test cases under "hw2/test" directory.

补充：每道题一个文件夹，其中Makefile文件应有 `make build`、`make test`、 `make test`命令，供主文件夹里Makefile使用（注意修改主文件里的Makefile）
