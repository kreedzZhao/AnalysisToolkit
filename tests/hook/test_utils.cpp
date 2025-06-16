#include "test_utils.h"

#include <dlfcn.h>

#include <iostream>

namespace TestUtils {

// 测试用的原始函数
int original_test_function(int a, int b) {
    return a + b;
}

// 测试用的钩子函数
int hooked_test_function(int a, int b) {
    return (a + b) * 2;  // 修改行为：返回双倍结果
}

// 测试用的带副作用的原始函数
static int function_call_count = 0;
int counting_function(int value) {
    function_call_count++;
    return value * function_call_count;
}

// 测试用的钩子函数，记录调用
static int hooked_call_count = 0;
int counting_hook_function(int value) {
    hooked_call_count++;
    return value + 1000;  // 明显不同的行为
}

// 获取调用计数
int getOriginalCallCount() {
    return function_call_count;
}

int getHookedCallCount() {
    return hooked_call_count;
}

void resetCallCounts() {
    function_call_count = 0;
    hooked_call_count = 0;
}

// 获取函数地址的工具函数
void* getFunctionAddress(const std::string& function_name) {
    // 首先尝试在当前可执行文件中查找
    void* handle = dlopen(nullptr, RTLD_LAZY);
    if (handle) {
        void* addr = dlsym(handle, function_name.c_str());
        dlclose(handle);
        if (addr) {
            return addr;
        }
    }

    // 尝试在系统库中查找
    handle = dlopen("libsystem_c.dylib", RTLD_LAZY);
    if (handle) {
        void* addr = dlsym(handle, function_name.c_str());
        dlclose(handle);
        return addr;
    }

    return nullptr;
}

// 检查地址是否有效的工具函数
bool isValidMemoryAddress(void* addr) {
    if (addr == nullptr) {
        return false;
    }

    Dl_info info;
    return dladdr(addr, &info) != 0;
}

// 简单的断言宏
void assert_true(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "Assertion failed: " << message << std::endl;
        abort();
    }
}

void assert_equal(int expected, int actual, const std::string& message) {
    if (expected != actual) {
        std::cerr << "Assertion failed: " << message << " (expected: " << expected
                  << ", actual: " << actual << ")" << std::endl;
        abort();
    }
}

void assert_not_null(void* ptr, const std::string& message) {
    if (ptr == nullptr) {
        std::cerr << "Assertion failed: " << message << " (pointer is null)" << std::endl;
        abort();
    }
}

}  // namespace TestUtils
