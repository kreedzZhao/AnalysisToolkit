#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#include <string>

namespace TestUtils {

// 测试用的函数声明
int original_test_function(int a, int b);
int hooked_test_function(int a, int b);

// 带副作用的测试函数
int counting_function(int value);
int counting_hook_function(int value);

// 调用计数相关函数
int getOriginalCallCount();
int getHookedCallCount();
void resetCallCounts();

// 工具函数
void* getFunctionAddress(const std::string& function_name);
bool isValidMemoryAddress(void* addr);

// 简单的断言函数
void assert_true(bool condition, const std::string& message);
void assert_equal(int expected, int actual, const std::string& message);
void assert_not_null(void* ptr, const std::string& message);

}  // namespace TestUtils

#endif  // TEST_UTILS_H
