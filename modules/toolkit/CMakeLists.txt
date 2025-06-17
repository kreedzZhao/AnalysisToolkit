cmake_minimum_required(VERSION 3.22.1)
enable_language(C ASM)

add_library(toolkit STATIC src/AnalysisToolkit.cpp)
# 设置公共头文件目录 PUBLIC 意味着依赖此库的任何目标都会继承此 include 路径
target_include_directories(
  toolkit
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> # 在构建时使用相对路径
)

# 设置私有头文件目录 PRIVATE 意味着只有 module 内部的源文件才能使用此 include 路径
target_include_directories(
  toolkit PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>)

target_link_libraries(toolkit PUBLIC hook utility)
