cmake_minimum_required(VERSION 3.22.1)

project(
  AnalysisToolkit
  VERSION 1.0.0
  DESCRIPTION "Android Native Analysis Toolkit"
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_BUILD_TYPE Debug)

# 选项：是否构建测试
option(BUILD_TESTS "Build the tests" ON)

# executable app
add_subdirectory(app)

add_subdirectory(modules/hook)
add_subdirectory(modules/utility)

# 添加测试
if(BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
