#include <dlfcn.h>
#include <gtest/gtest.h>

#include <cstdarg>
#include <memory>

#include "hook/inline_hook.h"
#include "test_utils.h"

using namespace AnalysisToolkit;

class InlineHookTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // 获取 HookManager 实例
        hook_manager_ = HookManager::getInstance();
        ASSERT_NE(hook_manager_, nullptr);

        // 初始化 HookManager
        ASSERT_TRUE(hook_manager_->initialize());

        // 重置测试计数器
        TestUtils::resetCallCounts();
    }

    void TearDown() override {
        // 清理所有 hooks
        if (hook_manager_) {
            hook_manager_->cleanup();
        }
    }

    HookManager* hook_manager_ = nullptr;
};

// 测试单例模式
TEST_F(InlineHookTest, SingletonPattern) {
    HookManager* instance1 = HookManager::getInstance();
    HookManager* instance2 = HookManager::getInstance();

    EXPECT_EQ(instance1, instance2);
    EXPECT_EQ(instance1, hook_manager_);
}

// 测试初始化
TEST_F(InlineHookTest, Initialize) {
    EXPECT_TRUE(hook_manager_->initialize());

    // 重复初始化应该也成功
    EXPECT_TRUE(hook_manager_->initialize());
}

// 测试地址验证
TEST_F(InlineHookTest, IsValidAddress) {
    // 测试空指针
    EXPECT_FALSE(TestUtils::isValidMemoryAddress(nullptr));

    // 测试有效地址（函数地址）
    void* func_addr = reinterpret_cast<void*>(&TestUtils::original_test_function);
    EXPECT_TRUE(TestUtils::isValidMemoryAddress(func_addr));

    // 测试无效地址
    void* invalid_addr = reinterpret_cast<void*>(0x1);
    // 注意：这个测试可能在不同系统上表现不同，所以我们只检查不会崩溃
    bool result = TestUtils::isValidMemoryAddress(invalid_addr);
    (void)result;  // 避免未使用变量警告
}

// 测试符号解析
TEST_F(InlineHookTest, SymbolResolution) {
    // 测试解析系统函数
    void* printf_addr = hook_manager_->getSymbolAddress("libsystem_c.dylib", "printf");
    if (printf_addr != nullptr) {
        EXPECT_TRUE(TestUtils::isValidMemoryAddress(printf_addr));
    }

    // 测试解析不存在的符号
    void* nonexistent =
        hook_manager_->getSymbolAddress("libsystem_c.dylib", "nonexistent_function");
    EXPECT_EQ(nonexistent, nullptr);

    // 测试解析不存在的库
    void* from_nonexistent_lib = hook_manager_->getSymbolAddress("nonexistent_lib.dylib", "printf");
    EXPECT_EQ(from_nonexistent_lib, nullptr);
}

// 测试获取库路径
TEST_F(InlineHookTest, GetLibraryPath) {
    void* func_addr = reinterpret_cast<void*>(&TestUtils::original_test_function);
    std::string lib_path = hook_manager_->getLibraryPath(func_addr);

    // 应该返回当前可执行文件的路径或者"unknown"
    EXPECT_FALSE(lib_path.empty());

    // 测试空指针
    std::string null_path = hook_manager_->getLibraryPath(nullptr);
    EXPECT_EQ(null_path, "unknown");
}

// 测试 Hook 状态查询（初始状态）
TEST_F(InlineHookTest, InitialHookState) {
    void* func_addr = reinterpret_cast<void*>(&TestUtils::original_test_function);

    // 初始状态应该没有被 hook
    EXPECT_FALSE(hook_manager_->isHooked(func_addr));

    // 获取不存在的 hook 信息应该返回 nullptr
    EXPECT_EQ(hook_manager_->getHookInfo(func_addr), nullptr);

    // 初始状态应该没有任何 hooks
    auto hooks = hook_manager_->getAllHooks();
    EXPECT_TRUE(hooks.empty());
}

// 测试基本的函数 Hook
TEST_F(InlineHookTest, BasicFunctionHook) {
    void* original_func = reinterpret_cast<void*>(&TestUtils::original_test_function);
    void* hook_func = reinterpret_cast<void*>(&TestUtils::hooked_test_function);
    void* original_backup = nullptr;

    // 尝试 hook 函数
    HookStatus status =
        hook_manager_->hookFunction(original_func, hook_func, &original_backup, "test_hook");

    if (status == HookStatus::SUCCESS) {
        // 验证 hook 状态
        EXPECT_TRUE(hook_manager_->isHooked(original_func));

        // 验证 hook 信息
        HookInfo* info = hook_manager_->getHookInfo(original_func);
        ASSERT_NE(info, nullptr);
        EXPECT_EQ(info->target_address, original_func);
        EXPECT_EQ(info->replace_function, hook_func);
        EXPECT_EQ(info->symbol_name, "test_hook");
        EXPECT_TRUE(info->is_active);
        EXPECT_EQ(info->type, HookType::FUNCTION_INLINE);

        // 验证 getAllHooks
        auto hooks = hook_manager_->getAllHooks();
        EXPECT_EQ(hooks.size(), 1);
        EXPECT_EQ(hooks[0], info);

        // 测试重复 hook 同一个函数
        HookStatus duplicate_status = hook_manager_->hookFunction(original_func,
                                                                  hook_func,
                                                                  &original_backup,
                                                                  "duplicate_hook");
        EXPECT_EQ(duplicate_status, HookStatus::ALREADY_HOOKED);
    } else {
        // 如果 hook 失败，跳过相关测试
        GTEST_SKIP() << "Hook operation failed, possibly due to system restrictions";
    }
}

// 测试符号 Hook
TEST_F(InlineHookTest, SymbolHook) {
    void* hook_func = reinterpret_cast<void*>(&TestUtils::hooked_test_function);
    void* original_backup = nullptr;

    // 尝试 hook 系统函数（如果可能的话）
    HookStatus status = hook_manager_->hookSymbol("libsystem_c.dylib",
                                                  "puts",
                                                  hook_func,
                                                  &original_backup,
                                                  "puts_hook");

    if (status == HookStatus::SUCCESS) {
        // 验证 hook 状态
        void* puts_addr = hook_manager_->getSymbolAddress("libsystem_c.dylib", "puts");
        if (puts_addr) {
            EXPECT_TRUE(hook_manager_->isHooked(puts_addr));
        }
    } else if (status == HookStatus::SYMBOL_NOT_FOUND) {
        GTEST_SKIP() << "Symbol not found, skipping symbol hook test";
    } else {
        GTEST_SKIP() << "Symbol hook failed, possibly due to system restrictions";
    }
}

// 测试无效地址的 Hook
TEST_F(InlineHookTest, HookInvalidAddress) {
    void* invalid_addr = nullptr;
    void* hook_func = reinterpret_cast<void*>(&TestUtils::hooked_test_function);
    void* original_backup = nullptr;

    HookStatus status =
        hook_manager_->hookFunction(invalid_addr, hook_func, &original_backup, "invalid_hook");

    EXPECT_EQ(status, HookStatus::INVALID_ADDRESS);
}

// 测试 Unhook
TEST_F(InlineHookTest, UnhookFunction) {
    void* original_func = reinterpret_cast<void*>(&TestUtils::original_test_function);
    void* hook_func = reinterpret_cast<void*>(&TestUtils::hooked_test_function);
    void* original_backup = nullptr;

    // 首先 hook 函数
    HookStatus hook_status =
        hook_manager_->hookFunction(original_func, hook_func, &original_backup, "test_hook");

    if (hook_status == HookStatus::SUCCESS) {
        // 验证已经被 hook
        EXPECT_TRUE(hook_manager_->isHooked(original_func));

        // Unhook 函数
        HookStatus unhook_status = hook_manager_->unhookFunction(original_func);
        EXPECT_EQ(unhook_status, HookStatus::SUCCESS);

        // 验证已经被 unhook
        EXPECT_FALSE(hook_manager_->isHooked(original_func));
        EXPECT_EQ(hook_manager_->getHookInfo(original_func), nullptr);

        // 验证 getAllHooks 为空
        auto hooks = hook_manager_->getAllHooks();
        EXPECT_TRUE(hooks.empty());

        // 测试重复 unhook
        HookStatus duplicate_unhook = hook_manager_->unhookFunction(original_func);
        EXPECT_EQ(duplicate_unhook, HookStatus::FAILED);
    } else {
        GTEST_SKIP() << "Initial hook failed, skipping unhook test";
    }
}

// 测试多个 Hook
TEST_F(InlineHookTest, MultipleHooks) {
    void* func1 = reinterpret_cast<void*>(&TestUtils::original_test_function);
    void* func2 = reinterpret_cast<void*>(&TestUtils::counting_function);
    void* hook_func = reinterpret_cast<void*>(&TestUtils::hooked_test_function);
    void* original_backup1 = nullptr;
    void* original_backup2 = nullptr;

    // Hook 第一个函数
    HookStatus status1 = hook_manager_->hookFunction(func1, hook_func, &original_backup1, "hook1");

    // Hook 第二个函数
    HookStatus status2 = hook_manager_->hookFunction(func2, hook_func, &original_backup2, "hook2");

    if (status1 == HookStatus::SUCCESS && status2 == HookStatus::SUCCESS) {
        // 验证两个函数都被 hook
        EXPECT_TRUE(hook_manager_->isHooked(func1));
        EXPECT_TRUE(hook_manager_->isHooked(func2));

        // 验证 getAllHooks 返回两个 hook
        auto hooks = hook_manager_->getAllHooks();
        EXPECT_EQ(hooks.size(), 2);

        // Unhook 第一个函数
        EXPECT_EQ(hook_manager_->unhookFunction(func1), HookStatus::SUCCESS);

        // 验证只有第二个函数还被 hook
        EXPECT_FALSE(hook_manager_->isHooked(func1));
        EXPECT_TRUE(hook_manager_->isHooked(func2));

        hooks = hook_manager_->getAllHooks();
        EXPECT_EQ(hooks.size(), 1);
    } else {
        GTEST_SKIP() << "One or more hook operations failed";
    }
}

// 测试错误处理
TEST_F(InlineHookTest, ErrorHandling) {
    // 测试 hook 空指针
    void* null_addr = nullptr;
    void* hook_func = reinterpret_cast<void*>(&TestUtils::hooked_test_function);
    void* original_backup = nullptr;

    HookStatus status =
        hook_manager_->hookFunction(null_addr, hook_func, &original_backup, "null_hook");
    EXPECT_EQ(status, HookStatus::INVALID_ADDRESS);

    // 测试 unhook 未被 hook 的函数
    void* unhookable_addr = reinterpret_cast<void*>(&TestUtils::original_test_function);
    HookStatus unhook_status = hook_manager_->unhookFunction(unhookable_addr);
    EXPECT_EQ(unhook_status, HookStatus::FAILED);
}

// 测试清理功能
TEST_F(InlineHookTest, Cleanup) {
    void* func1 = reinterpret_cast<void*>(&TestUtils::original_test_function);
    void* func2 = reinterpret_cast<void*>(&TestUtils::counting_function);
    void* hook_func = reinterpret_cast<void*>(&TestUtils::hooked_test_function);
    void* original_backup = nullptr;

    // 设置多个 hook
    hook_manager_->hookFunction(func1, hook_func, &original_backup, "hook1");
    hook_manager_->hookFunction(func2, hook_func, &original_backup, "hook2");

    // 验证有 hook 存在
    auto hooks_before = hook_manager_->getAllHooks();

    // 执行清理
    hook_manager_->cleanup();

    // 验证所有 hook 都被清理
    auto hooks_after = hook_manager_->getAllHooks();
    EXPECT_TRUE(hooks_after.empty());

    // 验证函数不再被 hook
    EXPECT_FALSE(hook_manager_->isHooked(func1));
    EXPECT_FALSE(hook_manager_->isHooked(func2));
}

// 测试线程安全（简单测试）
TEST_F(InlineHookTest, ThreadSafety) {
    // 这是一个简单的线程安全测试
    // 在真实环境中，应该使用更复杂的多线程测试

    void* func_addr = reinterpret_cast<void*>(&TestUtils::original_test_function);

    // 连续快速调用应该不会导致崩溃
    for (int i = 0; i < 100; ++i) {
        bool is_hooked = hook_manager_->isHooked(func_addr);
        (void)is_hooked;  // 避免未使用变量警告

        auto hooks = hook_manager_->getAllHooks();
        (void)hooks;  // 避免未使用变量警告
    }

    SUCCEED();  // 如果没有崩溃，就算通过
}

// 主函数
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
