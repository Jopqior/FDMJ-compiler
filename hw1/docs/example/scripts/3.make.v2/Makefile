# Makefile还能简化build的逻辑
#   之间有复杂的依赖关系，很难顺序写出
#   无需重新编译已编译过的文件

.PHONY: dir build run test clean rebuild

build: dir build/main.out

dir:
	@mkdir -p build

run:
	@build/main.out

test:
	@build/main.out < test/test.txt

clean:
	@rm -rf build

rebuild: clean build

build/main.out: build/a.o build/b.o build/c.o \
				        build/d.o build/e.o build/f.o build/g.o build/all.o \
					      build/ast.o build/lexer.o build/parser.o build/main.o
	cc -o $@ $^

build/a.o: lib/hello/a.c include/hello/a.h
	cc -o $@ -c -Iinclude/hello $<

build/b.o: lib/hello/b.c include/hello/b.h
	cc -o $@ -c -Iinclude/hello $<

build/c.o: lib/hello/c.c include/hello/c.h
	cc -o $@ -c -Iinclude/hello $<

build/d.o: lib/hello/d.c include/hello/d.h
	cc -o $@ -c -Iinclude/hello $<

build/e.o: lib/hello/e.c include/hello/e.h
	cc -o $@ -c -Iinclude/hello $<

build/f.o: lib/hello/f.c include/hello/f.h
	cc -o $@ -c -Iinclude/hello $<

build/g.o: lib/hello/g.c include/hello/g.h
	cc -o $@ -c -Iinclude/hello $<

build/all.o: lib/hello/all.c include/hello/all.h
	cc -o $@ -c -Iinclude/hello $<

build/ast.o: lib/frontend/ast.c include/frontend/ast.h
	cc -o $@ -c -Iinclude/frontend $<

build/lexer.o: build/lexer.c build/parser.h
	cc -o $@ -c -Iinclude/build $<

build/lexer.c: lib/frontend/lexer.lex build/parser.h
	lex -o $@ $<

build/parser.h: build/parser.c

build/parser.c: lib/frontend/parser.yacc # 加了-d，同时生成.h和.c；-v用来debug
	yacc -o $@ -d -v $<

build/parser.o: build/parser.c build/parser.h
	cc -o $@ -c -Ibuild $<

build/main.o: tools/main.c
	cc -o $@ -c -Iinclude/frontend -Iinclude/hello -Ibuild $<
