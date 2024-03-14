CURRENT_DIR = tools/2.4
BUILD_DIR = $(CURRENT_DIR)/build
TEST_DIR = test/2.4

.PHONY: build run test clean rebuild

build:
	@mkdir -p build; \
	lex -o build/lexer.c lexer.lex; \
	cc -o build/lexer build/lexer.c -ll

run:
	@build/lexer

test:
	@cd ../../$(TEST_DIR); \
	for file in $$(ls .); do \
		if [ "$${file##*.}" = "txt" ]; then \
			echo test "$${file%%.*}".txt; \
			../../$(BUILD_DIR)/lexer < "$${file%%.*}".txt; \
		fi \
	done; \
	cd ../../$(CURRENT_DIR)

clean:
	@rm -rf build

rebuild: clean build
