RM       = rm -rf
CLANG    = clang-14

BUILD_DIR = build
TEST_DIR  = tests

MAKEFLAGS = --no-print-directory

.PHONY: build clean veryclean rebuild test handin

build:
	@cmake -G Ninja -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Release; \
	cd $(BUILD_DIR) && ninja;

clean:
	@find $(TEST_DIR) -type f \( \
		-name "*.output" \
		\) -exec $(RM) {} \;

veryclean: clean
	@$(RM) $(BUILD_DIR)

rebuild: veryclean build

test: clean
	@cd $(TEST_DIR); \
	for file in $$(ls .); do \
		if [ "$${file##*.}" = "fmj" ]; then \
			echo "$${file%%.*}"; \
			../$(BUILD_DIR)/tools/main "$${file%%.*}".2.ast > "$${file%%.*}".output; \
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
	zip -q -r "docs/$$filename-hw3_4.zip" \
	  include lib tools CMakeLists.txt Makefile docs/report.pdf
