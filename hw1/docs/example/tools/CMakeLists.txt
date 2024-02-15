# 生成可执行文件
add_executable(main.out main.c)

# 添加链接库
target_link_libraries(main.out PUBLIC 
  frontend
  hello
)