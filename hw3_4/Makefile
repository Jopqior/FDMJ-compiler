RM       = rm -rf

MAKEFLAGS = --no-print-directory

TESTS_DIR = test

.PHONY: build clean veryclean rebuild test

build:
	@cmake -G Ninja -B build \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_INSTALL_PREFIX=build; \
	cd build; \
	ninja; \
	ninja install; 

clean: 
	@cd $(TESTS_DIR); \
	$(RM) *.output *.ll *.s *.out *.ast *.irp *.stm *.liv; \
	cd ..

veryclean: clean
	@$(RM) build

rebuild: veryclean build

test: clean
	@cd $(TESTS_DIR); \
	for file in $$(ls .); do \
		if [ "$${file##*.}" = "fmj" ]; then \
			echo "COMPILE $${file%%.*}"; \
			../build/bin/Parser0 "$${file%%.*}".s < "$${file%%.*}".fmj; \
		fi \
	done; \
	cd ..

handin:
	@if [ ! -f docs/report.pdf ]; then \
		echo "请先在docs文件夹下攥写报告, 并转换为'report.pdf'"; \
		exit 1; \
	fi; \
	echo "请输入'学号-姓名' (例如: 12345678910-某个人)"; \
	read filename; \
	zip -q -r "docs/$$filename-hw1.zip" \
	  include lib tools CMakeLists.txt Makefile docs/report.pdf