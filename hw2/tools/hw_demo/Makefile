# Makefile相当于bash脚本的多合一

.PHONY: build run test clean rebuild

build:
	@mkdir -p build; \
	cc -o build/main.o -c main.c; \
	cc -o build/main.out build/main.o

run:
	@build/main.out

test:
	@build/main.out < ../../test/hw_demo/test.txt

clean:
	@rm -rf build

rebuild: clean build
