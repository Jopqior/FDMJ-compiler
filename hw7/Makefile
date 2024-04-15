RM       = rm -rf
CLANG    = clang-14
LLVMLINK = llvm-link-14
LLI      = lli-14
ARMCC    = arm-linux-gnueabihf-gcc
QEMU     = qemu-arm

BUILD_DIR = "$(CURDIR)/build"
MAIN_EXE  = "$(BUILD_DIR)/tools/main"
FMJ2AST   = "$(CURDIR)/../tools/fmj2ast"
AST2IRP   = "$(CURDIR)/../tools/ast2irp"
TEST_DIR  = "$(CURDIR)/test"

MAKEFLAGS = --no-print-directory

.PHONY: build clean veryclean rebuild test test-extra handin

build:
	@cmake -G Ninja -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Release; \
	cd $(BUILD_DIR) && ninja

clean:
	@find $(TEST_DIR) -type f \( \
		-name "*.ll" -o -name "*.xml" -o -name "*.output" \
		-o -name "*.src" -o -name "*.ast" -o -name "*.irp" -o -name "*.debug" \
		\) -exec $(RM) {} \;

veryclean: clean
	@$(RM) $(BUILD_DIR)

rebuild: veryclean build

test: clean
	@cd $(TEST_DIR); \
	for file in $$(ls .); do \
		if [ "$${file##*.}" = "fmj" ]; then \
			echo "[$${file%%.*}]"; \
			$(FMJ2AST) "$${file%%.*}" && \
			$(MAIN_EXE) "$${file%%.*}"; \
		fi \
	done; \
	cd $(CURDIR)

test-extra: clean
	@cd $(TEST_DIR)/extra; \
	for file in $$(ls .); do \
		if [ "$${file##*.}" = "fmj" ]; then \
			echo "[$${file%%.*}]"; \
			$(FMJ2AST) "$${file%%.*}" && \
			$(MAIN_EXE) "$${file%%.*}"; \
		fi \
	done; \
	cd $(CURDIR)

.PHONY: ast2irp
ast2irp: clean
	@cd $(TEST_DIR); \
	for file in $$(ls .); do \
		if [ "$${file##*.}" = "fmj" ]; then \
			echo "[$${file%%.*}]"; \
			$(FMJ2AST) "$${file%%.*}" && \
			$(AST2IRP) "$${file%%.*}"; \
		fi \
	done; \
	cd $(CURDIR)

handin:
	@if [ ! -f docs/report.pdf ]; then \
		echo "请先在docs文件夹下攥写报告, 并转换为'report.pdf'"; \
		exit 1; \
	fi; \
	echo "请输入'学号-姓名' (例如: 12345678910-某个人)"; \
	read filename; \
	zip -q -r "docs/$$filename-hw7.zip" \
	  docs/report.pdf include lib

TEST_EXTERNAL_DIR=../../FDMJ-tests
TEXT_EXTERNAL_HW=hw07
TESTFILE_EXTERNAL_DIR=$(TEST_EXTERNAL_DIR)/$(TEXT_EXTERNAL_HW)/test
TEST_EXTERNAL_BIN=$(TEST_EXTERNAL_DIR)/$(TEXT_EXTERNAL_HW)/bin

RUN_RANDOM_CODE_GEN := 0
CHECK_PY_SCRIPT := check.py
PRINT_INFO := 0
CLEAN_RANDOM_TEST := 0

# 如果在命令行中传递了 RANDOM=1，则启动随机代码生成
ifdef RANDOM
	RUN_RANDOM_CODE_GEN := $(RANDOM)
endif

ifeq ($(RUN_RANDOM_CODE_GEN), 1)
    RANDOM_CODE_GEN := python3 ../randomCodeGen_v2.py;
endif

# 如果在命令行中传递了 CHECK，则设置使用的检查脚本
ifdef CHECK
    CHECK_PY_SCRIPT := $(CHECK)
endif

# 如果在命令行中传递了 INFO，则设置是否打印额外信息
ifdef INFO
	PRINT_INFO := $(INFO)
endif

ifdef CLEAN_RANDOM
	CLEAN_RANDOM_TEST := $(CLEAN_RANDOM)
endif

clean_external: 
	@$(RM) $(TESTFILE_EXTERNAL_DIR)/bin
	@$(RM) $(TEST_EXTERNAL_DIR)/$(TEXT_EXTERNAL_HW)/yours
	@$(RM) $(TEST_EXTERNAL_DIR)/$(TEXT_EXTERNAL_HW)/correct
	@$(RM) $(TEST_EXTERNAL_DIR)/$(TEXT_EXTERNAL_HW)/tools
	
	@if [ "$(CLEAN_RANDOM_TEST)" = "1" ]; then \
		find $(TESTFILE_EXTERNAL_DIR) -type f \( \
			-name "*.txt" -o -name "*.ast" -o -name "*.src" -o -name "*.xml" \
			-o -name "*.output" -o -name "gen_program_*" -o -name "myRandomTest*.fmj" \
			-o -name "*.irp" \
		\) -exec $(RM) {} \; ;\
	else \
		find $(TESTFILE_EXTERNAL_DIR) -type f \( \
			-name "*.txt" -o -name "*.ast" -o -name "*.src" -o -name "*.xml" \
			-o -name "*.output" -o -name "gen_program_*" \
			-o -name "*.irp" \
		\) -exec $(RM) {} \; ;\
	fi
	
SHELL:=/bin/bash

test_external: clean build clean_external
	@mkdir -p $(TEST_EXTERNAL_DIR)/$(TEXT_EXTERNAL_HW)/yours
	@mkdir -p $(TEST_EXTERNAL_DIR)/$(TEXT_EXTERNAL_HW)/correct
	@cd $(TESTFILE_EXTERNAL_DIR); \
	$(RANDOM_CODE_GEN) \
	for file in $$(ls .); do \
		if [ "$${file##*.}" = "fmj" ]; then \
			if [ $(PRINT_INFO) -eq 1 ]; then \
				echo "[$${file%%.*}]"; \
				echo "running fmj to ast code..."; \
			fi; \
			$(FMJ2AST) "$${file%%.*}"; \
			if [ $(PRINT_INFO) -eq 1 ]; then \
				echo "running your code..."; \
			fi; \
			$(MAIN_EXE) "$${file%%.*}"; \
			mv "$${file%%.*}.3.irp" "../yours/$${file%%.*}.irp"; \
			if [ $(PRINT_INFO) -eq 1 ]; then \
				echo "running standard code..."; \
			fi; \
			$(AST2IRP) "$${file%%.*}"; \
			mv "$${file%%.*}.3.irp" "../correct/$${file%%.*}.irp"; \
		fi; \
	done; \
	python3 ../${CHECK_PY_SCRIPT}; \
	cd $(CURDIR)
