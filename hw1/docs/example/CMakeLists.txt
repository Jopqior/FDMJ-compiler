# CMake最低版本号要求
cmake_minimum_required(VERSION 3.20)

# 项目信息
project(main)

# 添加包含路径
include_directories(include/frontend)
include_directories(include/hello)
include_directories(${PROJECT_BINARY_DIR}/include/frontend)

# 递归构建
add_subdirectory(lib)
add_subdirectory(tools)