cmake_minimum_required(VERSION 3.22.1)
enable_language(C ASM)

add_library(hook STATIC src/inline_hook.cpp)
# 设置公共头文件目录
# PUBLIC 意味着依赖此库的任何目标都会继承此 include 路径
target_include_directories(hook PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> # 在构建时使用相对路径
)

# 设置私有头文件目录
# PRIVATE 意味着只有 module 内部的源文件才能使用此 include 路径
target_include_directories(hook PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
)


# dobby start
target_include_directories(
    hook 
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/external/dobby/include>
)
target_include_directories(
    hook
    PRIVATE
    # ${CMAKE_CURRENT_SOURCE_DIR}/external/dobby/include
    ${CMAKE_CURRENT_SOURCE_DIR}/external/dobby/source
    ${CMAKE_CURRENT_SOURCE_DIR}/external/dobby/builtin-plugin
    ${CMAKE_CURRENT_SOURCE_DIR}/external/dobby/builtin-plugin/AndroidRestriction
    ${CMAKE_CURRENT_SOURCE_DIR}/external/dobby/builtin-plugin/SymbolResolver
    ${CMAKE_CURRENT_SOURCE_DIR}/external/dobby/external/logging
)
macro(SET_OPTION option value)
    set(${option} ${value} CACHE INTERNAL "" FORCE)
endmacro()
# 关闭DobbyDebug日志
SET_OPTION(DOBBY_DEBUG NO)
# 关闭shared库,和我们的so 集成到一起。
SET_OPTION(DOBBY_GENERATE_SHARED OFF)
# 是否加载linker加载so的回调
SET_OPTION(Plugin.LinkerLoadCallback OFF)
add_subdirectory(external/Dobby dobby.build)
# dobby end

# 处理模块依赖 (如果 module_a 依赖 module_b)
# target_link_libraries(module_a PUBLIC module_b)
# PUBLIC: 依赖 module_a 的目标也会隐式依赖 module_b
# PRIVATE: 只有 module_a 内部需要 module_b，外部不需要
# INTERFACE: module_a 本身不直接使用 module_b，但依赖 module_a 的目标需要 module_b
# 具体取决于 module_a 如何使用 module_b 的接口或实现
target_link_libraries(hook
    PUBLIC
    utility
    dobby
)