clean:
	@find .  -type f \( \
		-name "*.ll" -o -name "*.xml" -o -name "*.output" \
		-o -name "*.src" -o -name "*.ast" -o -name "*.irp" \
		-o -name "*.stm" \
		\) -exec $(RM) {} \;

test: clean
	clang-14 -S -emit-llvm libsysy64.c -o libsysy.ll; \
	for file in $$(ls .); do \
		if [ "$${file##*.}" = "lli" ]; then \
			echo "$${file%%.*}"; \
 llvm-link-14 -opaque-pointers "$${file}" libsysy.ll -o "$${file%%.*}".ll;\
 		lli-14 -opaque-pointers "$${file%%.*}".ll;\
		echo $$?;\
		fi \
	done;
