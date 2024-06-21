RM       = rm -rf
CLANG    = clang-14
LLVMLINK = llvm-link-14
LLI      = lli-14
ARMCC    = arm-linux-gnueabihf-gcc
QEMU     = qemu-arm
LLC	     = llc-14

BUILD_DIR = "$(CURDIR)/build"
MAIN_EXE  = "$(BUILD_DIR)/tools/main"
FMJ2AST   = "$(CURDIR)/vendor/tools/fmj2ast"
AST2IRP   = "$(CURDIR)/vendor/tools/ast2irp"
IRP2LLVM  = "$(CURDIR)/vendor/tools/irp2llvm"

TEST_DIR  = "$(CURDIR)/test"

MAKEFLAGS = --no-print-directory

.PHONY: build clean veryclean rebuild handin compile run-llvm run-rpi

build:
	@cmake -G Ninja -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Release; \
	cd $(BUILD_DIR) && ninja

clean:
	@find $(TEST_DIR) -type f \( \
		-name "*.ll" -o -name "*.xml" -o -name "*.output" \
		-o -name "*.src" -o -name "*.ast" -o -name "*.irp" \
		-o -name "*.stm" -o -name "*.ins" -o -name "*.ssa" \
		-o -name "*.cfg" -o -name "*.rpi" -o -name "*.arm" \
		-o -name "*.s" \
		-o -name "*.debug" \
		-o -name "*.ig" \
		\) -exec $(RM) {} \;

veryclean: clean
	@$(RM) $(BUILD_DIR)

rebuild: veryclean build

compile: clean
	@cd $(TEST_DIR); \
	for file in $$(ls .); do \
		if [ "$${file##*.}" = "fmj" ]; then \
			echo "[$${file%%.*}]"; \
			$(MAIN_EXE) "$${file%%.*}" -c 0 < "$${file%%.*}".fmj; \
			$(MAIN_EXE) "$${file%%.*}" -c 1 < "$${file%%.*}".fmj; \
			$(LLVMLINK) --opaque-pointers "$${file%%.*}".7.ssa $(BUILD_DIR)/vendor/libsysy/libsysy64.ll -S -o "$${file%%.*}".ll; \
			$(ARMCC) -mcpu=cortex-a72 "$${file%%.*}".10.s $(BUILD_DIR)/vendor/libsysy/libsysy32.s --static -o "$${file%%.*}".s; \
		fi \
	done; \
	cd $(CURDIR)

run-llvm:
	@cd $(TEST_DIR); \
	for file in $$(ls .); do \
		if [ "$${file##*.}" = "fmj" ]; then \
			echo "[$${file%%.*}]"; \
			$(LLI) -opaque-pointers "$${file%%.*}".ll  && \
			echo $$?; \
		fi \
	done; \
	cd $(CURDIR)

run-rpi:
	@cd $(TEST_DIR); \
	for file in $$(ls .); do \
		if [ "$${file##*.}" = "fmj" ]; then \
			echo "[$${file%%.*}]"; \
			$(QEMU) -B 0x1000 "$${file%%.*}".s && \
			echo $$?; \
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
	zip -q -r "docs/$$filename-final.zip" \
	  docs/report.pdf include lib test tools vendor CMakeLists.txt Makefile README.md
