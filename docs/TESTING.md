# 测试指南

## 概述

本项目为 `inline_hook.cpp` 模块提供了全面的单元测试，使用 Google Test 框架进行测试。测试覆盖了 HookManager 类的所有主要功能。

## 测试结构

### 测试文件组织

```
tests/
├── CMakeLists.txt              # 测试构建配置
├── hook/
│   ├── test_inline_hook.cpp    # 主要测试文件
│   ├── test_utils.cpp          # 测试工具实现
│   └── test_utils.h            # 测试工具声明
```

### 测试覆盖范围

#### ✅ HookManager 核心功能测试

1. **单例模式测试** (`SingletonPattern`)
   - 验证 HookManager::getInstance() 返回同一个实例
   - 确保单例模式正确实现

2. **初始化测试** (`Initialize`)
   - 测试 initialize() 方法
   - 验证重复初始化的安全性

3. **地址验证测试** (`IsValidAddress`)
   - 测试空指针处理
   - 测试有效函数地址
   - 测试无效地址的安全处理

4. **符号解析测试** (`SymbolResolution`)
   - 测试系统库符号解析
   - 测试不存在符号的错误处理
   - 测试不存在库的错误处理

5. **库路径获取测试** (`GetLibraryPath`)
   - 测试从函数地址获取库路径
   - 测试空指针的处理

#### ✅ Hook 功能测试

6. **初始状态测试** (`InitialHookState`)
   - 验证未 hook 状态的查询
   - 验证初始状态下无 hook 信息

7. **基本函数 Hook 测试** (`BasicFunctionHook`)
   - 测试函数 hook 的基本流程
   - 验证 hook 信息的正确性
   - 测试重复 hook 的处理

8. **符号 Hook 测试** (`SymbolHook`)
   - 测试通过符号名称进行 hook
   - 验证系统函数的 hook 能力

9. **无效地址 Hook 测试** (`HookInvalidAddress`)
   - 测试对无效地址的 hook 尝试
   - 验证错误状态返回

10. **Unhook 测试** (`UnhookFunction`)
    - 测试 unhook 功能
    - 验证 unhook 后状态清理
    - 测试重复 unhook 的处理

11. **多重 Hook 测试** (`MultipleHooks`)
    - 测试同时 hook 多个函数
    - 验证独立 unhook 功能

#### ✅ 错误处理和边界情况测试

12. **错误处理测试** (`ErrorHandling`)
    - 测试各种错误情况的处理
    - 验证错误状态码的正确性

13. **清理功能测试** (`Cleanup`)
    - 测试 cleanup() 方法
    - 验证所有 hook 被正确清理

14. **线程安全测试** (`ThreadSafety`)
    - 简单的线程安全验证
    - 确保并发访问不会导致崩溃

## 运行测试

### 方法一：使用测试脚本（推荐）

```bash
# 运行完整的测试套件
./scripts/run-tests.sh
```

这将：
- 清理旧的构建目录
- 重新配置和构建项目
- 运行所有测试
- 生成测试报告

### 方法二：手动构建和运行

```bash
# 创建测试构建目录
mkdir build-tests && cd build-tests

# 配置 CMake（启用测试）
cmake .. -DBUILD_TESTS=ON

# 构建
make -j$(nproc)

# 运行测试
cd tests
./run_tests

# 或使用 CTest
cd ..
ctest --output-on-failure --verbose
```

### 方法三：运行特定测试

```bash
# 运行单个测试
./run_tests --gtest_filter=InlineHookTest.BasicFunctionHook

# 运行匹配模式的测试
./run_tests --gtest_filter=*Hook*

# 列出所有测试
./run_tests --gtest_list_tests
```

## 测试输出解读

### 成功示例

```
[==========] Running 14 tests from 1 test suite.
[----------] Global test environment set-up.
[----------] 14 tests from InlineHookTest
[ RUN      ] InlineHookTest.SingletonPattern
[       OK ] InlineHookTest.SingletonPattern (0 ms)
...
[==========] 14 tests from 1 test suite ran. (9 ms total)
[  PASSED  ] 14 tests.
```

### 日志级别说明

- `[I][]` - 信息日志（正常操作）
- `[W][]` - 警告日志（预期的警告情况）
- `[E][]` - 错误日志（预期的错误情况）
- `[D][]` - 调试日志（详细操作信息）

## 测试设计原则

### 1. 安全性优先

- 测试使用自定义的测试函数，避免影响系统
- 每个测试都有独立的 SetUp 和 TearDown
- 错误情况下使用 GTEST_SKIP() 而不是失败

### 2. 全面覆盖

- 测试所有公共方法
- 覆盖正常流程和异常情况
- 包含边界条件测试

### 3. 可维护性

- 使用测试工具类减少重复代码
- 清晰的测试命名和注释
- 模块化的测试结构

## 添加新测试

### 1. 扩展现有测试

在 `test_inline_hook.cpp` 中添加新的 `TEST_F` 方法：

```cpp
TEST_F(InlineHookTest, YourNewTest) {
    // 测试实现
}
```

### 2. 添加测试工具

在 `test_utils.h` 和 `test_utils.cpp` 中添加辅助函数：

```cpp
// test_utils.h
int your_test_helper_function();

// test_utils.cpp
int your_test_helper_function() {
    // 实现
}
```

### 3. 创建新测试模块

为其他模块创建测试：

```cpp
// tests/other_module/test_other.cpp
#include <gtest/gtest.h>
#include "other_module/other.h"

class OtherModuleTest : public ::testing::Test {
    // 测试类实现
};

TEST_F(OtherModuleTest, SomeTest) {
    // 测试实现
}
```

## 持续集成

测试可以轻松集成到 CI/CD 流程中：

```yaml
# GitHub Actions 示例
- name: Run Tests
  run: |
    ./scripts/run-tests.sh

- name: Upload Test Results
  uses: actions/upload-artifact@v2
  with:
    name: test-results
    path: build-tests/tests/test_results.xml
```

## 故障排除

### 常见问题

1. **Hook 操作失败**
   - 某些系统可能有权限限制
   - 测试会自动跳过受限的操作

2. **符号解析失败**
   - 动态库路径在不同系统上可能不同
   - 测试设计了回退机制

3. **编译错误**
   - 确保安装了所需的依赖
   - 检查 C++20 编译器支持

### 调试测试

```bash
# 运行测试并输出详细信息
./run_tests --gtest_verbose

# 使用调试器
gdb ./run_tests
```

## 性能测试

当前测试主要关注功能正确性。对于性能测试，可以：

1. 添加性能基准测试
2. 使用 Google Benchmark 库
3. 测试大量 hook 操作的性能

## 测试最佳实践

1. **每个测试应该独立**
2. **使用描述性的测试名称**
3. **测试一个概念**
4. **包含正面和负面测试**
5. **保持测试简单和快速**

通过这个完善的测试系统，我们确保 `inline_hook.cpp` 模块的可靠性和稳定性。
