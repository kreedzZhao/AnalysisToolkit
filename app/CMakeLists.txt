cmake_minimum_required(VERSION 3.22.1)

add_executable(AnalysisToolkit src/main.cpp)

target_link_libraries(AnalysisToolkit hook utility)
