MAKEFLAGS = --no-print-directory

SUBDIRS = $(wildcard tools/*)

.PHONY: build test clean handin

build:
	@for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir build; \
	done

test:
	@for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir test; \
	done

clean:
	@for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir clean; \
	done

handin:
	@if [ ! -f docs/report.pdf ]; then \
		echo "请先在docs文件夹下攥写报告, 并转换为'report.pdf'"; \
		exit 1; \
	fi; \
	echo "请输入'学号-姓名' (例如: 12345678910-某个人)"; \
	read filename; \
	zip -q -r "docs/$$filename-hw2.zip" \
	  Makefile test tools docs/report.pdf