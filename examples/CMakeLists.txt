cmake_minimum_required(VERSION 3.22.1)

# 添加示例可执行文件
add_executable(memory_parser_example memory_parser_example.cpp)

# 链接 utility 库
target_link_libraries(memory_parser_example PRIVATE utility)

# 设置 C++ 标准
target_compile_features(memory_parser_example PRIVATE cxx_std_20)

# 包含头文件目录
target_include_directories(memory_parser_example
                           PRIVATE ${CMAKE_SOURCE_DIR}/modules/utility/include)

# 设置编译选项
target_compile_options(
  memory_parser_example
  PRIVATE $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra>
          $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra>
          $<$<CXX_COMPILER_ID:AppleClang>:-Wall -Wextra>)

# 设置输出目录
set_target_properties(
  memory_parser_example PROPERTIES RUNTIME_OUTPUT_DIRECTORY
                                   ${CMAKE_BINARY_DIR}/examples)
