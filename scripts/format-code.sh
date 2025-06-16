#!/bin/bash

# 格式化 AnalysisToolkit 项目中的所有 C++ 代码
# 使用 clang-format 根据 .clang-format 配置文件进行格式化

set -e

# 项目根目录
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$PROJECT_ROOT"

echo "🚀 开始格式化 C++ 代码..."

# 查找并格式化所有 C++ 文件
find . -type f \( \
    -name "*.cpp" -o \
    -name "*.cc" -o \
    -name "*.c++" -o \
    -name "*.cxx" -o \
    -name "*.c" -o \
    -name "*.h" -o \
    -name "*.hpp" -o \
    -name "*.hh" -o \
    -name "*.h++" -o \
    -name "*.hxx" \
\) \
    -not -path "./build/*" \
    -not -path "./cmake-build-*/*" \
    -not -path "./.git/*" \
    -not -path "./modules/hook/external/*" \
    -print0 | xargs -0 clang-format -i

echo "✅ C++ 代码格式化完成!"

# 可选：同时格式化 CMake 文件
if command -v cmake-format &> /dev/null; then
    echo "🔧 开始格式化 CMake 文件..."
    find . -type f \( -name "CMakeLists.txt" -o -name "*.cmake" \) \
        -not -path "./build/*" \
        -not -path "./cmake-build-*/*" \
        -not -path "./.git/*" \
        -not -path "./modules/hook/external/*" \
        -print0 | xargs -0 cmake-format -i
    echo "✅ CMake 文件格式化完成!"
else
    echo "⚠️  cmake-format 未安装，跳过 CMake 文件格式化"
fi

echo "🎉 所有代码格式化完成!"
