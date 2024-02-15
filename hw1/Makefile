RM       = rm -rf
CLANG    = clang-14
LLVMLINK = llvm-link-14
LLI      = lli-14
ARMCC    = arm-linux-gnueabihf-gcc
QEMU     = qemu-arm

BUILD_DIR = build
TEST_DIR  = test

MAKEFLAGS = --no-print-directory

.PHONY: build clean veryclean rebuild test

build:
	@cmake -G Ninja -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Release; \
	cd $(BUILD_DIR) && ninja

clean:
	@find $(TEST_DIR) -type f \( \
		-name "*.ll" -o -name "*.xml" -o -name "*.output" \
		\) -exec $(RM) {} \;

veryclean: clean
	@$(RM) $(BUILD_DIR)

rebuild: veryclean build

test: clean
	@cd $(TEST_DIR); \
	for file in $$(ls .); do \
		if [ "$${file##*.}" = "fdmj" ]; then \
			echo "$${file%%.*}"; \
			../$(BUILD_DIR)/tools/main "$${file%%.*}" < "$${file%%.*}".fdmj > "$${file%%.*}".output; \
		fi \
	done; \
	cd ../..
