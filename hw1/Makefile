RM       = rm -rf
CLANG    = clang-14
LLVMLINK = llvm-link-14
LLI      = lli-14
ARMCC    = arm-linux-gnueabihf-gcc
QEMU     = qemu-arm

BUILD_DIR       = $(CURDIR)/build
MAIN_EXE        = $(BUILD_DIR)/tools/main
TEST_DIR        = $(CURDIR)/test
TEST_EXTRA_DIR  = $(TEST_DIR)/extra

MAKEFLAGS = --no-print-directory

.PHONY: build clean veryclean rebuild test handin

build:
	@cmake -G Ninja -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Release; \
	cd $(BUILD_DIR) && ninja && cd ..; \
	mkdir -p $(BUILD_DIR)/vendor; \
	$(CLANG) -S -emit-llvm $(CURDIR)/vendor/libsysy64.c -o $(BUILD_DIR)/vendor/libsysy64.ll

clean:
	@find $(TEST_DIR) -type f \( \
		-name "*.ll" -o -name "*.xml" -o -name "*.output" \
		\) -exec $(RM) {} \;

clean-extra:
	@find $(TEST_EXTRA_DIR) -type f \( \
		-name "*.ll" -o -name "*.xml" -o -name "*.output" \
		\) -exec $(RM) {} \;

veryclean: clean clean-extra
	@$(RM) $(BUILD_DIR)

rebuild: veryclean build

test: clean
	@cd $(TEST_DIR); \
	for file in $$(ls .); do \
		if [ "$${file##*.}" = "fdmj" ]; then \
			echo "[$${file%%.*}]"; \
			$(MAIN_EXE) "$${file%%.*}" < "$${file%%.*}".fdmj && \
			$(LLVMLINK) --opaque-pointers "$${file%%.*}".ll $(BUILD_DIR)/vendor/libsysy64.ll -S -o "$${file%%.*}".ll.ll && \
			$(LLI) -opaque-pointers "$${file%%.*}".ll.ll > "$${file%%.*}".output && \
			echo $$?; \
		fi \
	done; \
	cd ../..

test-extra: clean-extra
	@cd $(TEST_EXTRA_DIR); \
	for file in $$(ls .); do \
		if [ "$${file##*.}" = "fdmj" ]; then \
			echo "[$${file%%.*}]"; \
			$(MAIN_EXE) "$${file%%.*}" < "$${file%%.*}".fdmj && \
			$(LLVMLINK) --opaque-pointers "$${file%%.*}".ll $(BUILD_DIR)/vendor/libsysy64.ll -S -o "$${file%%.*}".ll.ll && \
			$(LLI) -opaque-pointers "$${file%%.*}".ll.ll > "$${file%%.*}".output && \
			echo $$?; \
		fi \
	done; \
	cd ../..

handin:
	@if [ ! -f docs/report.pdf ]; then \
		echo "请先在docs文件夹下攥写报告, 并转换为'report.pdf'"; \
		exit 1; \
	fi; \
	echo "请输入'学号-姓名' (例如: 12345678910-某个人)"; \
	read filename; \
	zip -q -r "docs/$$filename-hw1.zip" \
	  include lib tools vendor CMakeLists.txt docs/report.pdf
