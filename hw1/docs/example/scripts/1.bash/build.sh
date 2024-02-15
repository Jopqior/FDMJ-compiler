mkdir -p build
cc -o build/a.o -c -Iinclude/hello lib/hello/a.c
cc -o build/b.o -c -Iinclude/hello lib/hello/b.c
cc -o build/c.o -c -Iinclude/hello lib/hello/c.c
cc -o build/d.o -c -Iinclude/hello lib/hello/d.c
cc -o build/e.o -c -Iinclude/hello lib/hello/e.c
cc -o build/f.o -c -Iinclude/hello lib/hello/f.c
cc -o build/g.o -c -Iinclude/hello lib/hello/g.c
cc -o build/all.o -c -Iinclude/hello lib/hello/all.c
cc -o build/ast.o -c -Iinclude/frontend lib/frontend/ast.c
yacc -o build/parser.c -d -v lib/frontend/parser.yacc
lex -o build/lexer.c lib/frontend/lexer.lex
cc -o build/lexer.o -c -Iinclude/build build/lexer.c
cc -o build/parser.o -c -Ibuild build/parser.c
cc -o build/main.o -c -Iinclude/frontend -Iinclude/hello -Ibuild tools/main.c
cc -o build/main.out \
 build/a.o build/b.o build/c.o build/d.o \
 build/e.o build/f.o build/g.o build/all.o \
 build/ast.o build/lexer.o build/parser.o build/main.o