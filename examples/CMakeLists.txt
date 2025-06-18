cmake_minimum_required(VERSION 3.22.1)

# 添加示例可执行文件
add_executable(memory_parser_example memory_parser_example.cpp)

# 链接 utility 库
target_link_libraries(memory_parser_example PRIVATE utility)

# 设置 C++ 标准
set_property(TARGET memory_parser_example PROPERTY CXX_STANDARD 20)
set_property(TARGET memory_parser_example PROPERTY CXX_STANDARD_REQUIRED ON)

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

# 添加 trace 示例可执行文件
add_executable(trace_example trace_example.cpp)

# 链接所需的库
target_link_libraries(trace_example PRIVATE trace utility)

# 设置 C++ 标准
set_property(TARGET trace_example PROPERTY CXX_STANDARD 17)
set_property(TARGET trace_example PROPERTY CXX_STANDARD_REQUIRED ON)

# 包含头文件目录
target_include_directories(
  trace_example PRIVATE ${CMAKE_SOURCE_DIR}/modules/trace/include
                        ${CMAKE_SOURCE_DIR}/modules/utility/include)

# 设置编译选项
target_compile_options(
  trace_example
  PRIVATE $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra>
          $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra>
          $<$<CXX_COMPILER_ID:AppleClang>:-Wall -Wextra>)

# 设置输出目录
set_target_properties(trace_example PROPERTIES RUNTIME_OUTPUT_DIRECTORY
                                               ${CMAKE_BINARY_DIR}/examples)
