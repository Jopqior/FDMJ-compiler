RM       = rm -rf
CLANG    = clang-14
LLVMLINK = llvm-link-14
LLI      = lli-14
ARMCC    = arm-linux-gnueabihf-gcc
QEMU     = qemu-arm

BUILD_DIR = "$(CURDIR)/build"
MAIN_EXE  = "$(BUILD_DIR)/tools/main"
FMJ2AST   = "$(CURDIR)/../tools/fmj2ast"
ASTCHECK  = "$(CURDIR)/../tools/astcheck"
TEST_DIR  = "$(CURDIR)/test"

MAKEFLAGS = --no-print-directory

.PHONY: build clean veryclean rebuild test test-extra handin

build:
	@cmake -G Ninja -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Release; \
	cd $(BUILD_DIR) && ninja

clean:
	@find $(TEST_DIR) -type f \( \
		-name "*.ll" -o -name "*.xml" -o -name "*.output" \
		-o -name "*.src" -o -name "*.ast" -o -name "*.irp" \
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

.PHONY: astcheck
astcheck: clean
	@cd $(TEST_DIR); \
	for file in $$(ls .); do \
		if [ "$${file##*.}" = "fmj" ]; then \
			echo "[$${file%%.*}]"; \
			$(FMJ2AST) "$${file%%.*}" && \
			$(ASTCHECK) "$${file%%.*}"; \
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
	zip -q -r "docs/$$filename-hw6.zip" \
	  docs/report.pdf include lib

EXTERNAL_TEST_DIR = $(abspath $(CURDIR)/../../FDMJ-tests/hw06)
.PHONY: test-external clean-external gen-random clean-random

clean-external: clean-random
	@find $(EXTERNAL_TEST_DIR)/test -type f \( \
		-name "*.ll" -o -name "*.xml" -o -name "*.output" \
		-o -name "*.src" -o -name "*.ast" -o -name "*.irp" \
		\) -exec $(RM) {} \;
	@$(RM) $(EXTERNAL_TEST_DIR)/yours/*.debug
	@$(RM) $(EXTERNAL_TEST_DIR)/correct/*.debug

gen-random:
	@cd $(EXTERNAL_TEST_DIR); \
	python3 randomCodeGen.py; \
	cd $(CURDIR)

clean-random:
	@find $(EXTERNAL_TEST_DIR)/test -type f \( \
		-name "random*.fmj" \
		\) -exec $(RM) {} \;

test-external: clean-external gen-random
	@cd $(EXTERNAL_TEST_DIR)/test; \
	for file in $$(ls .); do \
		if [ "$${file##*.}" = "fmj" ]; then \
			echo "[$${file%%.*}]"; \
			$(FMJ2AST) "$${file%%.*}" && \
			$(MAIN_EXE) "$${file%%.*}" 2> ../yours/"$${file%%.*}".debug; \
		fi \
	done; \
	for file in $$(ls .); do \
		if [ "$${file##*.}" = "fmj" ]; then \
			echo "[$${file%%.*}]"; \
			$(FMJ2AST) "$${file%%.*}" && \
			$(ASTCHECK) "$${file%%.*}" 2> ../correct/"$${file%%.*}".debug; \
		fi \
	done; \
	cd $(EXTERNAL_TEST_DIR); \
	python3 check.py; \
	cd $(CURDIR)
