# Makefile将build委托给CMake，进一步简化

MAKEFLAGS = --no-print-directory

.PHONY: build run test clean rebuild

build:
	@cmake -G Ninja -B build; \
	cd build && ninja

run:
	@build/tools/main.out

test:
	@build/tools/main.out < test/test.txt

clean:
	@rm -rf build

rebuild: clean build