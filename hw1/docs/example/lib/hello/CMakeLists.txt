# 编译+构建链接库
aux_source_directory(. HELLO_SRCS)
add_library(hello OBJECT ${HELLO_SRCS})