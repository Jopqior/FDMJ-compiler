RM       = rm -rf
CLANG    = clang-14
LLVMLINK = llvm-link-14
LLI      = lli-14
ARMCC    = arm-linux-gnueabihf-gcc
QEMU     = qemu-arm

BUILD_DIR = "$(CURDIR)/build"
MAIN_EXE  = "$(BUILD_DIR)/tools/main"
FMJ2AST   = "$(CURDIR)/vendor/tools/fmj2ast"
AST2IRP   = "$(CURDIR)/vendor/tools/ast2irp"
IRP2LLVM = "$(CURDIR)/vendor/tools/irp2llvm"
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
		-o -name "*.stm" -o -name "*.ins" \
		-o -name "*.debug" \
		\) -exec $(RM) {} \;

veryclean: clean
	@$(RM) $(BUILD_DIR)

rebuild: veryclean build

test: clean
	@cd $(TEST_DIR); \
	for file in $$(ls .); do \
		if [ "$${file##*.}" = "fmj" ]; then \
			echo "[$${file%%.*}]"; \
			$(FMJ2AST) "$${file%%.*}"; \
			$(AST2IRP) -f xml "$${file%%.*}"; \
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

.PHONY: irp2llvm
irp2llvm: clean
	@cd $(TEST_DIR); \
	for file in $$(ls .); do \
		if [ "$${file##*.}" = "fmj" ]; then \
			echo "[$${file%%.*}]"; \
			$(FMJ2AST) "$${file%%.*}" && \
			$(AST2IRP) -f xml "$${file%%.*}" && \
			$(IRP2LLVM) "$${file%%.*}"; \
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
	zip -q -r "docs/$$filename-hw9_10.zip" \
	  docs/report.pdf include lib

TEST_EXTERNAL_DIR = "$(CURDIR)/../../FDMJ-tests"
TEST_EXTERNAL_HW = hw09_10
PRINT_INFO := 0

.PHONY: test_external clean_external test-run
clean_external:
	@$(RM) $(TEST_EXTERNAL_DIR)/$(TEST_EXTERNAL_HW)/yours; \
	$(RM) $(TEST_EXTERNAL_DIR)/$(TEST_EXTERNAL_HW)/correct; \
	find $(TEST_DIR) -type f \( \
		-name "*.ll" -o -name "*.xml" -o -name "*.output" \
		-o -name "*.src" -o -name "*.ast" -o -name "*.irp" \
		-o -name "*.stm" -o -name "*.ins" \
		-o -name "*.debug" \
		\) -exec $(RM) {} \;

test_external: clean build clean_external
	@mkdir -p $(TEST_EXTERNAL_DIR)/$(TEST_EXTERNAL_HW)/yours; \
	mkdir -p $(TEST_EXTERNAL_DIR)/$(TEST_EXTERNAL_HW)/correct; \
	cd $(TEST_EXTERNAL_DIR)/$(TEST_EXTERNAL_HW)/test; \
	for file in $$(ls .); do \
		if [ "$${file##*.}" = "fmj" ]; then \
			echo "[$${file%%.*}]"; \
			if [ $(PRINT_INFO) -eq 1 ]; then \
				echo "running fmj to ast code..."; \
			fi; \
			$(FMJ2AST) "$${file%%.*}" && \
			if [ $(PRINT_INFO) -eq 1 ]; then \
				echo "running ast to irp code..."; \
			fi; \
			$(AST2IRP) -f xml "$${file%%.*}" && \
			if [ $(PRINT_INFO) -eq 1 ]; then \
				echo "running your code..."; \
			fi; \
			$(MAIN_EXE) "$${file%%.*}"; \
			mv "$${file%%.*}.5.ins" "../yours/$${file%%.*}.5.ins"; \
			if [ $(PRINT_INFO) -eq 1 ]; then \
				echo "running correct code..."; \
			fi; \
			$(IRP2LLVM) "$${file%%.*}"; \
			mv "$${file%%.*}.5.ins" "../correct/$${file%%.*}.5.ins"; \
		fi \
	done; \
	cd $(CURDIR)
