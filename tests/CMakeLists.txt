cmake_minimum_required(VERSION 3.22.1)

# 启用测试
enable_testing()

# 查找 Google Test
find_package(GTest QUIET)

if(NOT GTest_FOUND)
  # 如果没有找到系统的 Google Test，则下载并构建
  include(FetchContent)
  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
    GIT_SHALLOW TRUE)
  # For Windows: Prevent overriding the parent project's compiler/linker
  # settings
  set(gtest_force_shared_crt
      ON
      CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)
endif()

# 添加测试可执行文件
add_executable(
  run_tests
  hook/test_inline_hook.cpp hook/test_utils.cpp
  utility/test_process_memory_parser.cpp toolkit/test_analysis_tool_kit.cpp)

# 链接库
target_link_libraries(run_tests PRIVATE gtest_main gtest hook utility toolkit)

# 包含头文件目录
target_include_directories(
  run_tests
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/modules/hook/include
          ${CMAKE_SOURCE_DIR}/modules/utility/include
          ${CMAKE_SOURCE_DIR}/modules/toolkit/include)

# 添加编译选项
target_compile_options(
  run_tests
  PRIVATE $<$<CXX_COMPILER_ID:GNU>:-Wall
          -Wextra
          -Wno-unused-parameter>
          $<$<CXX_COMPILER_ID:Clang>:-Wall
          -Wextra
          -Wno-unused-parameter>
          $<$<CXX_COMPILER_ID:AppleClang>:-Wall
          -Wextra
          -Wno-unused-parameter>)

# 注册测试
include(GoogleTest)
gtest_discover_tests(run_tests)
