cmake_minimum_required(VERSION 3.20.5)

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
# 选项：是否构建示例程序
option(BUILD_EXAMPLES "Build the examples" ON)

# 对于 Android 构建，禁用测试和示例程序（因为它们无法在宿主机上运行）
if(ANDROID OR CMAKE_SYSTEM_NAME STREQUAL "Android")
  set(BUILD_TESTS OFF)
  set(BUILD_EXAMPLES OFF)
endif()

# executable app add_subdirectory(app)

add_subdirectory(modules/hook)
add_subdirectory(modules/utility)
add_subdirectory(modules/toolkit)
add_subdirectory(modules/trace)

# 添加测试
if(BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

# 添加示例程序
if(BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

# 作为子模块链接
set(SUBMODULE_PACKAGE_NAME AnalysisToolkit)

# 安装公共库目标 只有那些希望被父项目链接的库才需要被"导出" 这里假设 hook 和 utility 是公共的，module_c 可能是内部使用的。
install(
  TARGETS hook utility toolkit # 暂时移除 trace，因为它的依赖链中包含 QBDI 导出冲突
  EXPORT ${SUBMODULE_PACKAGE_NAME}Targets # 创建一个导出集合，命名为 AnalysisToolkitTargets
  ARCHIVE DESTINATION lib/${SUBMODULE_PACKAGE_NAME} # 静态库的安装路径
  LIBRARY DESTINATION lib/${SUBMODULE_PACKAGE_NAME} # 动态库的安装路径
          # RUNTIME DESTINATION bin/${SUBMODULE_PACKAGE_NAME} # 如果子模块有可执行文件（不常见）
)

# 单独安装 trace 库，但不导出到目标集合中
install(
  TARGETS trace trace_impl
  ARCHIVE DESTINATION lib/${SUBMODULE_PACKAGE_NAME}
  LIBRARY DESTINATION lib/${SUBMODULE_PACKAGE_NAME}
)

# 安装公共头文件 这确保父项目在链接子模块后，能够找到相应的头文件。 我们将公共头文件安装到
# <install_prefix>/include/${SUBMODULE_PACKAGE_NAME}/<module_name>/ 下
# 例如：<install_prefix>/include/AnalysisToolkit/hook/public_interface_a.h
install(
  DIRECTORY modules/hook/include/ # hook 的 include 目录
  DESTINATION include/${SUBMODULE_PACKAGE_NAME} # 安装到 include/AnalysisToolkit/
                                                # 目录下
  PATTERN ".gitkeep" EXCLUDE # 排除空的占位文件
)
install(
  DIRECTORY modules/utility/include/ # utility 的 include 目录
  DESTINATION include/${SUBMODULE_PACKAGE_NAME} # 安装到 include/AnalysisToolkit/
                                                # 目录下
  PATTERN ".gitkeep" EXCLUDE)
install(
  DIRECTORY modules/trace/include/ # trace 的 include 目录
  DESTINATION include/${SUBMODULE_PACKAGE_NAME} # 安装到 include/AnalysisToolkit/
                                                # 目录下
  PATTERN ".gitkeep" EXCLUDE)

install(
        DIRECTORY modules/toolkit/include/ # trace 的 include 目录
        DESTINATION include/${SUBMODULE_PACKAGE_NAME} # 安装到 include/AnalysisToolkit/
        # 目录下
        PATTERN ".gitkeep" EXCLUDE)
# 如果有其他公共模块的头文件需要暴露，也按此模式添加。

# 生成 CMake 包配置文件 (有助于 find_package 方式，但在 add_subdirectory 方式下也能提供信息) 这个文件是
# find_package 命令查找的核心。
install(
  EXPORT ${SUBMODULE_PACKAGE_NAME}Targets # 导出之前定义的 TARGETS 集合
  FILE ${SUBMODULE_PACKAGE_NAME}Targets.cmake # 生成的目标文件名称
  NAMESPACE ${SUBMODULE_PACKAGE_NAME}:: # 为导入的目标添加命名空间，如 AnalysisToolkit::hook
  DESTINATION lib/cmake/${SUBMODULE_PACKAGE_NAME} # 安装目标文件的路径
)

# 生成包版本文件 (可选但推荐，用于 find_package 的版本检查)
include(CMakePackageConfigHelpers)
# 假设你在项目顶部定义了版本，例如 set(PROJECT_VERSION "1.0.0")
set(PROJECT_VERSION "1.0.0") # 假设子模块的版本
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/${SUBMODULE_PACKAGE_NAME}ConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY AnyNewerVersion # 兼容任何新版本
)

# 安装 Config 文件 (find_package 需要的入口文件) 这个文件通常会包含
# ${SUBMODULE_PACKAGE_NAME}Targets.cmake
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/${SUBMODULE_PACKAGE_NAME}Config.cmake.in" # 模板文件路径
  "${CMAKE_CURRENT_BINARY_DIR}/${SUBMODULE_PACKAGE_NAME}Config.cmake"
  @ONLY)

install(
  FILES
    "${CMAKE_CURRENT_BINARY_DIR}/${SUBMODULE_PACKAGE_NAME}Config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/${SUBMODULE_PACKAGE_NAME}ConfigVersion.cmake"
  DESTINATION lib/cmake/${SUBMODULE_PACKAGE_NAME})
