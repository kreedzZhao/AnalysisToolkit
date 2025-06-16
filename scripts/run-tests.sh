#!/bin/bash

# 运行 AnalysisToolkit 项目的测试
# 包括构建测试和运行测试

set -e

# 项目根目录
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$PROJECT_ROOT"

# 构建目录
BUILD_DIR="build"
TEST_BUILD_DIR="build-tests"

echo "🧪 开始构建和运行测试..."

# 清理旧的测试构建目录
if [ -d "$TEST_BUILD_DIR" ]; then
    echo "🧹 清理旧的测试构建目录..."
    rm -rf "$TEST_BUILD_DIR"
fi

# 创建测试构建目录
mkdir -p "$TEST_BUILD_DIR"
cd "$TEST_BUILD_DIR"

echo "🔧 配置 CMake (启用测试)..."
cmake .. -DBUILD_TESTS=ON

echo "🔨 构建项目和测试..."
make -j$(nproc 2>/dev/null || echo 4)

echo "🏃 运行测试..."
cd tests

# 运行测试并显示详细输出
./run_tests --gtest_output=xml:test_results.xml

echo ""
echo "✅ 测试完成!"
echo "📊 测试结果已保存到: $TEST_BUILD_DIR/tests/test_results.xml"

# 可选：运行 ctest 以获得更好的输出格式
echo ""
echo "📋 运行 CTest 以获得更好的测试报告..."
cd ..
ctest --output-on-failure --verbose

echo ""
echo "🎉 所有测试执行完毕!"
