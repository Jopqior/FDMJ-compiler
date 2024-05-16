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
IRP2LLVM  = "$(CURDIR)/vendor/tools/irp2llvm"

TEST_DIR  = "$(CURDIR)/test"

MAKEFLAGS = --no-print-directory

.PHONY: build clean veryclean rebuild test test-extra handin runone

build:
	@cmake -G Ninja -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Release; \
	cd $(BUILD_DIR) && ninja

clean:
	@find $(TEST_DIR) -type f \( \
		-name "*.ll" -o -name "*.xml" -o -name "*.output" \
		-o -name "*.src" -o -name "*.ast" -o -name "*.irp" \
		-o -name "*.stm" -o -name "*.ins" -o -name "*.ssa" \
		-o -name "*.cfg" \
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
			$(IRP2LLVM) "$${file%%.*}"; \
			$(MAIN_EXE) "$${file%%.*}" 2> "$${file%%.*}".debug; \
		fi \
	done; \
	cd $(CURDIR)

test-run: clean
	@cd $(TEST_DIR); \
	for file in $$(ls .); do \
		if [ "$${file##*.}" = "fmj" ]; then \
			echo "[$${file%%.*}]"; \
			$(FMJ2AST) "$${file%%.*}" && \
			$(AST2IRP) -f xml "$${file%%.*}" && \
			$(IRP2LLVM) "$${file%%.*}" && \
			$(MAIN_EXE)  "$${file%%.*}" 2> "$${file%%.*}".debug && \
			$(LLVMLINK) --opaque-pointers "$${file%%.*}".8.ssa $(BUILD_DIR)/vendor/libsysy/libsysy64.ll -S -o "$${file%%.*}".ll && \
			$(LLI) -opaque-pointers "$${file%%.*}".ll  && \
			echo $$?; \
		fi \
	done; \
	cd $(CURDIR)

test-extra: clean
	@cd $(TEST_DIR)/extra; \
	for file in $$(ls .); do \
		if [ "$${file##*.}" = "fmj" ]; then \
			echo "[$${file%%.*}]"; \
			$(FMJ2AST) "$${file%%.*}"; \
			$(AST2IRP) -f xml "$${file%%.*}"; \
			$(IRP2LLVM) "$${file%%.*}"; \
		fi \
	done; \
	cd $(CURDIR)

irp2llvm:clean
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
	zip -q -r "docs/$$filename-hw11.zip" \
	  docs/report.pdf include lib

runone: clean
	@cd $(TEST_DIR); \
	echo "[${FILE}]"; \
	$(FMJ2AST) "${FILE}" && \
	$(AST2IRP) -f xml "${FILE}" && \
	$(IRP2LLVM) "${FILE}" && \
	$(MAIN_EXE)  "${FILE}" 2> "${FILE}".debug && \
	$(LLVMLINK) --opaque-pointers "${FILE}".8.ssa $(BUILD_DIR)/vendor/libsysy/libsysy64.ll -S -o "${FILE}".ll && \
	$(LLI) -opaque-pointers "${FILE}".ll > "${FILE}".output && \
	echo $$?; \
	cd $(CURDIR)
