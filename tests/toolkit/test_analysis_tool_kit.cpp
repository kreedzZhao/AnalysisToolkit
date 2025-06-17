//
// Created by kz.zhao on 2025/6/17.
//

#include <gtest/gtest.h>

#include <chrono>
#include <thread>

#include "../../modules/toolkit/include/toolkit/AnalysisToolkit.h"
#include "toolkit/AnalysisToolkit.h"
#include "utility/Logger.h"

using namespace AnalysisToolkit;

class AnalysisToolkitTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // 确保每个测试开始时都是清理状态
        if (isInitialized()) {
            cleanup();
        }
    }

    void TearDown() override {
        // 每个测试结束后清理
        if (isInitialized()) {
            cleanup();
        }
    }
};

// Test basic initialization and configuration
TEST_F(AnalysisToolkitTest, BasicInitialization) {
    EXPECT_FALSE(isInitialized());

    Config config;
    config.app_tag = "AnalysisToolkitTest";
    config.log_level = LogLevel::DEBUG;
    config.enable_console_log = true;
    config.enable_file_log = false;
    config.enable_hook_manager = false;

    EXPECT_TRUE(initialize(config));
    EXPECT_TRUE(isInitialized());
}

// Test default configuration initialization
TEST_F(AnalysisToolkitTest, DefaultConfigInitialization) {
    EXPECT_FALSE(isInitialized());

    // 使用默认配置
    EXPECT_TRUE(initialize());
    EXPECT_TRUE(isInitialized());
}

// Test multiple initialization calls
TEST_F(AnalysisToolkitTest, MultipleInitialization) {
    Config config;
    config.app_tag = "MultipleInitTest";

    EXPECT_TRUE(initialize(config));
    EXPECT_TRUE(isInitialized());

    // 再次初始化应该返回true但不重复初始化
    EXPECT_TRUE(initialize(config));
    EXPECT_TRUE(isInitialized());
}

// Test initialization with hook manager enabled
TEST_F(AnalysisToolkitTest, InitializationWithHookManager) {
    Config config;
    config.app_tag = "HookManagerTest";
    config.enable_hook_manager = true;

    EXPECT_TRUE(initialize(config));
    EXPECT_TRUE(isInitialized());

    // 验证HookManager是否可用
    HookManager* hook_manager = getHookManager();
    EXPECT_NE(hook_manager, nullptr);
}

// Test cleanup functionality
TEST_F(AnalysisToolkitTest, Cleanup) {
    Config config;
    config.app_tag = "CleanupTest";

    EXPECT_TRUE(initialize(config));
    EXPECT_TRUE(isInitialized());

    cleanup();
    EXPECT_FALSE(isInitialized());

    // 多次清理应该是安全的
    cleanup();
    EXPECT_FALSE(isInitialized());
}

// Test logger access
TEST_F(AnalysisToolkitTest, LoggerAccess) {
    Config config;
    config.app_tag = "LoggerTest";
    config.log_level = LogLevel::INFO;

    EXPECT_TRUE(initialize(config));

    Logger* logger = getLogger();
    EXPECT_NE(logger, nullptr);

    // 测试日志记录功能
    logger->info("Test info message");
    logger->debug("Test debug message");
    logger->warn("Test warning message");
    logger->error("Test error message");
}

// Test hook manager access
TEST_F(AnalysisToolkitTest, HookManagerAccess) {
    Config config;
    config.enable_hook_manager = true;

    EXPECT_TRUE(initialize(config));

    HookManager* hook_manager = getHookManager();
    EXPECT_NE(hook_manager, nullptr);

    // 验证单例模式 - 多次获取应该返回同一个实例
    HookManager* hook_manager2 = getHookManager();
    EXPECT_EQ(hook_manager, hook_manager2);
}

// Test library info
TEST_F(AnalysisToolkitTest, LibraryInfo) {
    std::string info = getLibraryInfo();
    EXPECT_FALSE(info.empty());
    EXPECT_TRUE(info.find("AnalysisToolkit") != std::string::npos);
    EXPECT_TRUE(info.find("v1.0.0") != std::string::npos);
}

// Test different log levels
TEST_F(AnalysisToolkitTest, DifferentLogLevels) {
    std::vector<LogLevel> levels = {LogLevel::DEBUG,
                                    LogLevel::INFO,
                                    LogLevel::WARN,
                                    LogLevel::ERROR};

    for (auto level : levels) {
        // 清理之前的状态
        if (isInitialized()) {
            cleanup();
        }

        Config config;
        config.app_tag = "LogLevelTest";
        config.log_level = level;
        config.enable_console_log = true;

        EXPECT_TRUE(initialize(config));
        EXPECT_TRUE(isInitialized());

        Logger* logger = getLogger();
        EXPECT_NE(logger, nullptr);
    }
}

// Test file logging configuration
TEST_F(AnalysisToolkitTest, FileLoggingConfig) {
    Config config;
    config.app_tag = "FileLogTest";
    config.enable_file_log = true;
    config.log_file_path = "/tmp/test_analysis_toolkit.log";
    config.enable_console_log = false;

    EXPECT_TRUE(initialize(config));
    EXPECT_TRUE(isInitialized());

    Logger* logger = getLogger();
    EXPECT_NE(logger, nullptr);

    // 写入一些测试日志
    logger->info("File logging test message");
}

// Test thread safety of initialization
TEST_F(AnalysisToolkitTest, ThreadSafetyInitialization) {
    const int num_threads = 10;
    std::vector<std::thread> threads;
    std::vector<bool> results(num_threads, false);

    Config config;
    config.app_tag = "ThreadSafetyTest";

    // 多线程同时初始化
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&config, &results, i]() { results[i] = initialize(config); });
    }

    // 等待所有线程完成
    for (auto& thread : threads) {
        thread.join();
    }

    // 所有线程都应该成功
    for (bool result : results) {
        EXPECT_TRUE(result);
    }

    EXPECT_TRUE(isInitialized());
}

// Test reinitialization after cleanup
TEST_F(AnalysisToolkitTest, ReinitializationAfterCleanup) {
    Config config1;
    config1.app_tag = "FirstInit";
    config1.log_level = LogLevel::DEBUG;

    EXPECT_TRUE(initialize(config1));
    EXPECT_TRUE(isInitialized());

    cleanup();
    EXPECT_FALSE(isInitialized());

    // 清理后重新初始化
    Config config2;
    config2.app_tag = "SecondInit";
    config2.log_level = LogLevel::INFO;
    config2.enable_hook_manager = true;

    EXPECT_TRUE(initialize(config2));
    EXPECT_TRUE(isInitialized());
}

// Test singleton pattern consistency
TEST_F(AnalysisToolkitTest, SingletonConsistency) {
    Config config;
    config.enable_hook_manager = true;

    EXPECT_TRUE(initialize(config));

    // 获取多个实例，验证单例模式
    Logger* logger1 = getLogger();
    Logger* logger2 = getLogger();
    EXPECT_EQ(logger1, logger2);

    HookManager* hook1 = getHookManager();
    HookManager* hook2 = getHookManager();
    EXPECT_EQ(hook1, hook2);
}
