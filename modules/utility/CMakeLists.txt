cmake_minimum_required(VERSION 3.22.1)

# 添加静态库，包含所有源文件
add_library(utility STATIC src/Logger.cpp src/ProcessMemoryParser.cpp)

# 设置 C++ 标准 target_compile_features(utility PUBLIC cxx_std_20)

# 设置公共头文件目录 PUBLIC 意味着依赖此库的任何目标都会继承此 include 路径
target_include_directories(
  utility
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> # 在构建时使用相对路径
)

# 设置私有头文件目录 PRIVATE 意味着只有 module 内部的源文件才能使用此 include 路径
target_include_directories(
  utility PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>)

# 平台特定的库链接
if(APPLE)
  # macOS 需要链接系统框架
  find_library(CORE_FOUNDATION CoreFoundation)
  find_library(CORE_SERVICES CoreServices)
  target_link_libraries(utility PRIVATE ${CORE_FOUNDATION} ${CORE_SERVICES})
endif()

# 设置编译选项
target_compile_options(
  utility
  PRIVATE $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra>
          $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra>
          $<$<CXX_COMPILER_ID:AppleClang>:-Wall -Wextra>)

# 为库添加版本信息和导出功能 set_target_properties(utility PROPERTIES VERSION 1.0.0 SOVERSION
# 1 PUBLIC_HEADER
# "include/utility/Logger.h;include/utility/ProcessMemoryParser.h" )
#
# 如果需要安装，设置安装规则 if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
# include(GNUInstallDirs)
#
# install(TARGETS utility EXPORT utilityTargets LIBRARY DESTINATION
# ${CMAKE_INSTALL_LIBDIR} ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} RUNTIME
# DESTINATION ${CMAKE_INSTALL_BINDIR} PUBLIC_HEADER DESTINATION
# ${CMAKE_INSTALL_INCLUDEDIR}/utility )
#
# install(EXPORT utilityTargets FILE utilityTargets.cmake NAMESPACE
# AnalysisToolkit:: DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/utility ) endif()
