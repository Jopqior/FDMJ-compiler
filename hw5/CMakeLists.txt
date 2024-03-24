# CMake最低版本号要求
cmake_minimum_required(VERSION 3.20)

# 项目信息
project(FudanCompiler)

# 添加包含路径
include_directories(include/frontend)
include_directories(include/utils/dsa)
include_directories(include/utils/printer)
include_directories(include/lxml)

# 递归构建
add_subdirectory(lib)
add_subdirectory(tools)