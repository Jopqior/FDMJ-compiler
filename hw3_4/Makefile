RM       = rm -rf
CLANG    = clang-14
LLVMLINK = llvm-link-14
LLI      = lli-14
ARMCC    = arm-linux-gnueabihf-gcc
QEMU     = qemu-arm

BUILD_DIR = $(CURDIR)/build
MAIN_EXE  = $(BUILD_DIR)/tools/main
TEST_DIR  = $(CURDIR)/test

MAKEFLAGS = --no-print-directory

.PHONY: build clean veryclean rebuild test test-extra handin

build:
	@cmake -G Ninja -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Release; \
	cd $(BUILD_DIR) && ninja

clean:
	@find $(TEST_DIR) -type f \( \
		-name "*.src" -o -name "*.ast" -o -name "*.output" \
		\) -exec $(RM) {} \;

veryclean: clean
	@$(RM) $(BUILD_DIR)

rebuild: veryclean build

test: clean
	@cd $(TEST_DIR); \
	for file in $$(ls .); do \
		if [ "$${file##*.}" = "fmj" ]; then \
			echo "[$${file%%.*}]"; \
			$(MAIN_EXE) "$${file%%.*}" < "$${file%%.*}".fmj > "$${file%%.*}".output; \
		fi \
	done; \
	cd $(CURDIR)

test-extra: clean
	@cd $(TEST_DIR)/extra; \
	for file in $$(ls .); do \
		if [ "$${file##*.}" = "fmj" ]; then \
			echo "[$${file%%.*}]"; \
			$(MAIN_EXE) "$${file%%.*}" < "$${file%%.*}".fmj > "$${file%%.*}".output; \
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
	zip -q -r "docs/$$filename-hw3_4.zip" \
	  include lib tools CMakeLists.txt docs/report.pdf

EXTERNAL_TEST_DIR = $(abspath $(CURDIR)/../../FDMJ-tests/hw03_04)
EXTERAL_FMJ2ST = $(EXTERNAL_TEST_DIR)/tools/fmj2ast
clean-external:
	@find $(EXTERNAL_TEST_DIR)/yours -type f \( \
		-name "*.src" -o -name "*.ast" -o -name "*.output" \
		\) -exec $(RM) {} \;
	@find $(EXTERNAL_TEST_DIR)/correct -type f \( \
		-name "*.src" -o -name "*.ast" -o -name "*.output" \
		\) -exec $(RM) {} \;
	@find $(EXTERNAL_TEST_DIR)/test -type f \( \
		-name "*.src" -o -name "*.ast" -o -name "*.output" \
		\) -exec $(RM) {} \;

gen-random:
	@cd $(EXTERNAL_TEST_DIR)/test; \
	for i in $$(seq 1 10); do \
        python3 randomCodeGen_v2.py; \
		mv output.txt random$$i.fmj; \
    done; \
	cd $(CURDIR)

clean-random:
	@cd $(EXTERNAL_TEST_DIR)/test; \
	for i in $$(seq 1 10); do \
		rm -f random$$i.fmj; \
	done; \
	cd $(CURDIR)

test-external: clean-external clean-random gen-random
	@echo "===== generate your src&ast ====="; \
	cd $(EXTERNAL_TEST_DIR)/test && \
	for file in *; do \
		if [ "$${file##*.}" = "fmj" ]; then \
			echo "[$${file%%.*}]"; \
			$(MAIN_EXE) "$${file%%.*}" < "$${file}" > ../yours/"$${file%%.*}".output; \
		fi \
	done; \
	find $(EXTERNAL_TEST_DIR)/test -type f \( \
		-name "*.src" -o -name "*.ast" -o -name "*.output" \
		\) -exec mv {} $(EXTERNAL_TEST_DIR)/yours/ \; && \
	echo "===== generate correct src&ast ====="; \
	for file in *; do \
		if [ "$${file##*.}" = "fmj" ]; then \
			echo "[$${file%%.*}]"; \
			$(EXTERAL_FMJ2ST) "$${file%%.*}" < "$${file}" > ../correct/"$${file%%.*}".output; \
		fi \
	done; \
	find $(EXTERNAL_TEST_DIR)/test -type f \( \
		-name "*.ast" -o -name "*.src" -o -name "*.output" \
		\) -exec mv {} $(EXTERNAL_TEST_DIR)/correct/ \;
	@cd $(EXTERNAL_TEST_DIR); \
	python3 check.py; \
	cd $(CURDIR)