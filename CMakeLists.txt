cmake_minimum_required(VERSION 3.10)

project(AnalysisToolkit
    VERSION 1.0.0
    DESCRIPTION "Android Native Analysis Toolkit"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(BUILD_EXAMPLES "Build examples" ON)
option(ENABLE_ANDROID "Enable Android specific features" OFF)

if(ANDROID OR CMAKE_SYSTEM_NAME STREQUAL "Android")
    set(ENABLE_ANDROID ON)
    add_definitions(-D__ANDROID__)
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

set(SOURCES
    src/Logger.cpp
    src/AnalysisToolkit.cpp
    src/Hook.cpp
    src/Monitor.cpp
)

set(HEADERS
    include/AnalysisToolkit/Logger.h
    include/AnalysisToolkit/AnalysisToolkit.h
    include/AnalysisToolkit/Hook.h
    include/AnalysisToolkit/Monitor.h
)

if(BUILD_SHARED_LIBS)
    add_library(${PROJECT_NAME}_shared SHARED ${SOURCES} ${HEADERS})
    message(STATUS "Building shared library")
else()
    add_library(${PROJECT_NAME}_static STATIC ${SOURCES} ${HEADERS})
    message(STATUS "Building static library")
endif()

set_target_properties(${PROJECT_NAME}_${CMAKE_BUILD_TYPE} PROPERTIES OUTPUT_NAME ${PROJECT_NAME})

if(ENABLE_ANDROID)
    target_link_libraries(${PROJECT_NAME}_${CMAKE_BUILD_TYPE} log)
    message(STATUS "Android build enabled - linking log library")
endif()

find_package(Threads REQUIRED)
target_link_libraries(${PROJECT_NAME}_${CMAKE_BUILD_TYPE} Threads::Threads)

target_include_directories(${PROJECT_NAME}_${CMAKE_BUILD_TYPE}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

#if(BUILD_EXAMPLES AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/examples)
#    add_subdirectory(examples)
#endif()

message(STATUS "=== AnalysisToolkit Build Configuration ===")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Android support: ${ENABLE_ANDROID}")
message(STATUS "===========================================")

install(TARGETS ${PROJECT_NAME}_${CMAKE_BUILD_TYPE}
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(DIRECTORY include/AnalysisToolkit
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)

# Hook functionality
option(ENABLE_HOOK_FUNCTIONALITY "Enable hook functionality with Dobby" ON)

if(ENABLE_HOOK_FUNCTIONALITY)
    # 尝试查找 dobby 库
    find_library(DOBBY_LIBRARY 
        NAMES dobby libdobby
        PATHS 
            ${CMAKE_CURRENT_SOURCE_DIR}/../nativeLib/src/main/cpp/hook/Dobby/build
            ${CMAKE_CURRENT_SOURCE_DIR}/../../nativeLib/src/main/cpp/hook/Dobby/build
            ${CMAKE_CURRENT_SOURCE_DIR}/libs
            /usr/local/lib
            /usr/lib
        NO_DEFAULT_PATH
    )

    # 查找 dobby 头文件
    find_path(DOBBY_INCLUDE_DIR 
        NAMES dobby.h
        PATHS 
            ${CMAKE_CURRENT_SOURCE_DIR}/../nativeLib/src/main/cpp/hook/Dobby/include
            ${CMAKE_CURRENT_SOURCE_DIR}/../../nativeLib/src/main/cpp/hook/Dobby/include
            ${CMAKE_CURRENT_SOURCE_DIR}/include/dobby
            /usr/local/include
            /usr/include
        NO_DEFAULT_PATH
    )

    if(DOBBY_LIBRARY AND DOBBY_INCLUDE_DIR)
        message(STATUS "Found Dobby library: ${DOBBY_LIBRARY}")
        message(STATUS "Found Dobby headers: ${DOBBY_INCLUDE_DIR}")
        
        # 添加 dobby 头文件
        target_include_directories(${PROJECT_NAME}_${CMAKE_BUILD_TYPE} PRIVATE ${DOBBY_INCLUDE_DIR})
        
        # 链接 dobby 库
        target_link_libraries(${PROJECT_NAME}_${CMAKE_BUILD_TYPE} ${DOBBY_LIBRARY})
        
        # 启用 Hook 功能
        target_compile_definitions(${PROJECT_NAME}_${CMAKE_BUILD_TYPE} PRIVATE ENABLE_HOOK_FUNCTIONALITY)
        
        if(BUILD_SHARED_LIBS)
            target_include_directories(${PROJECT_NAME}_shared PRIVATE ${DOBBY_INCLUDE_DIR})
            target_link_libraries(${PROJECT_NAME}_shared ${DOBBY_LIBRARY})
            target_compile_definitions(${PROJECT_NAME}_shared PRIVATE ENABLE_HOOK_FUNCTIONALITY)
        endif()
        
        message(STATUS "Hook functionality enabled with Dobby")
    else()
        message(WARNING "Dobby library not found. Hook functionality will be disabled.")
        message(STATUS "  DOBBY_LIBRARY: ${DOBBY_LIBRARY}")
        message(STATUS "  DOBBY_INCLUDE_DIR: ${DOBBY_INCLUDE_DIR}")
        
        # 禁用 Hook 功能
        target_compile_definitions(${PROJECT_NAME}_${CMAKE_BUILD_TYPE} PRIVATE DISABLE_HOOK_FUNCTIONALITY)
        
        if(BUILD_SHARED_LIBS)
            target_compile_definitions(${PROJECT_NAME}_shared PRIVATE DISABLE_HOOK_FUNCTIONALITY)
        endif()
    endif()
else()
    message(STATUS "Hook functionality disabled by configuration")
    target_compile_definitions(${PROJECT_NAME}_${CMAKE_BUILD_TYPE} PRIVATE DISABLE_HOOK_FUNCTIONALITY)
    
    if(BUILD_SHARED_LIBS)
        target_compile_definitions(${PROJECT_NAME}_shared PRIVATE DISABLE_HOOK_FUNCTIONALITY)
    endif()
endif()

# 链接基础库
target_link_libraries(${PROJECT_NAME}_${CMAKE_BUILD_TYPE} dl log)

if(BUILD_SHARED_LIBS)
    target_link_libraries(${PROJECT_NAME}_shared dl log)
endif()

# 安装配置
option(ENABLE_INSTALL "Enable install target" OFF)
if(ENABLE_INSTALL)
    install(TARGETS ${PROJECT_NAME}_${CMAKE_BUILD_TYPE}
            ARCHIVE DESTINATION lib
            LIBRARY DESTINATION lib
            RUNTIME DESTINATION bin)
    
    if(BUILD_SHARED_LIBS)
        install(TARGETS ${PROJECT_NAME}_shared
                ARCHIVE DESTINATION lib
                LIBRARY DESTINATION lib
                RUNTIME DESTINATION bin)
    endif()
    
    install(DIRECTORY include/AnalysisToolkit DESTINATION include)
endif()

# 编译选项
target_compile_options(${PROJECT_NAME}_${CMAKE_BUILD_TYPE} PRIVATE
    -Wall
    -Wextra
    -O2
    -fPIC
)

if(BUILD_SHARED_LIBS)
    target_compile_options(${PROJECT_NAME}_shared PRIVATE
        -Wall
        -Wextra
        -O2
        -fPIC
    )
endif()

# 调试信息
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(${PROJECT_NAME}_${CMAKE_BUILD_TYPE} PRIVATE DEBUG=1)
    if(BUILD_SHARED_LIBS)
        target_compile_definitions(${PROJECT_NAME}_shared PRIVATE DEBUG=1)
    endif()
endif()

# 设置默认目标
if(BUILD_SHARED_LIBS)
    add_library(${PROJECT_NAME} ALIAS ${PROJECT_NAME}_shared)
else()
    add_library(${PROJECT_NAME} ALIAS ${PROJECT_NAME}_static)
endif()
