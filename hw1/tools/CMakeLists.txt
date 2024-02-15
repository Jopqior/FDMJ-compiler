set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-result")

# 指定生成目标
add_executable(main main.c)

# 添加链接库
target_link_libraries(main PUBLIC
  backend_llvm
  frontend
  utils
)

