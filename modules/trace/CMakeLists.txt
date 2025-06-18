cmake_minimum_required(VERSION 3.22.1)

# 设置 QBDI 平台配置
if(APPLE)
  if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set(QBDI_PLATFORM "ios")
  else()
    set(QBDI_PLATFORM "osx")
  endif()
elseif(WIN32)
  set(QBDI_PLATFORM "windows")
elseif(ANDROID OR CMAKE_SYSTEM_NAME STREQUAL "Android")
  set(QBDI_PLATFORM "android")
elseif(UNIX)
  set(QBDI_PLATFORM "linux")
else()
  message(FATAL_ERROR "Unsupported platform for QBDI")
endif()

# 设置 QBDI 架构配置
if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64)")
  set(QBDI_ARCH "AARCH64")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm)")
  set(QBDI_ARCH "ARM")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64)")
  set(QBDI_ARCH "X86_64")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(i386|i686|x86)")
  set(QBDI_ARCH "X86")
else()
  message(
    FATAL_ERROR "Unsupported architecture for QBDI: ${CMAKE_SYSTEM_PROCESSOR}")
endif()

message(STATUS "Setting QBDI_PLATFORM to: ${QBDI_PLATFORM}")
message(STATUS "Setting QBDI_ARCH to: ${QBDI_ARCH}")

# 设置 QBDI 选项 - 只保留核心功能，关闭不必要的依赖
set(QBDI_STATIC_LIBRARY
    ON
    CACHE BOOL "Build QBDI static library")
set(QBDI_SHARED_LIBRARY
    OFF
    CACHE BOOL "Build QBDI shared library")
set(QBDI_TEST
    OFF
    CACHE BOOL "Build QBDI tests")
set(QBDI_BENCHMARK
    OFF
    CACHE BOOL "Build QBDI benchmarks")
set(QBDI_EXAMPLES
    OFF
    CACHE BOOL "Build QBDI examples")
set(QBDI_TOOLS_QBDIPRELOAD
    OFF
    CACHE BOOL "Build QBDI preload tools")
set(QBDI_TOOLS_VALIDATOR
    OFF
    CACHE BOOL "Build QBDI validator tools")
set(QBDI_TOOLS_PYQBDI
    OFF
    CACHE BOOL "Build Python bindings")
set(QBDI_TOOLS_FRIDAQBDI
    OFF
    CACHE BOOL "Build Frida bindings")
set(QBDI_CCACHE
    OFF
    CACHE BOOL "Use ccache for compilation")
set(QBDI_ASAN
    OFF
    CACHE BOOL "Use AddressSanitizer")

message(STATUS "QBDI configured for minimal C/C++ only build")

# 添加 QBDI 外部依赖
add_subdirectory(external/QBDI)

# 创建一个内部的实现库，包含 QBDI 依赖
add_library(trace_impl STATIC src/qbdi.cpp)

# 设置 C++ 标准（QBDI 需要 C++17 或更高）
target_compile_features(trace_impl PUBLIC cxx_std_17)

# 设置私有头文件目录
target_include_directories(
  trace_impl PRIVATE 
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# 链接 QBDI 到实现库
if(TARGET QBDI_static)
  target_link_libraries(trace_impl PRIVATE QBDI_static)
  message(STATUS "Linking trace implementation with QBDI static library")
elseif(TARGET QBDI)
  target_link_libraries(trace_impl PRIVATE QBDI)
  message(STATUS "Linking trace implementation with QBDI shared library")
else()
  message(FATAL_ERROR "QBDI library not found")
endif()

# 创建公共的 trace 库，不直接依赖 QBDI
add_library(trace INTERFACE)

# 设置公共头文件目录
target_include_directories(
  trace
  INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# 链接依赖库
target_link_libraries(trace INTERFACE utility trace_impl)

# 添加 QBDI 头文件目录到实现库
target_include_directories(
  trace_impl
  PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/external/QBDI/include>
          $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/external/QBDI/include>)
