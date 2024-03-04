MAKEFLAGS = --no-print-directory

.PHONY: build test clean handin

build:
	@cd tools/hw_demo; make build; cd ../..

test:
	@cd tools/hw_demo; make test; cd ../..

clean:
	@cd tools/hw_demo; make clean; cd ../..

handin:
	@if [ ! -f docs/report.pdf ]; then \
		echo "请先在docs文件夹下攥写报告, 并转换为'report.pdf'"; \
		exit 1; \
	fi; \
	echo "请输入'学号-姓名' (例如: 12345678910-某个人)"; \
	read filename; \
	zip -q -r "docs/$$filename-hw2.zip" \
	  Makefile test tools docs/report.pdf