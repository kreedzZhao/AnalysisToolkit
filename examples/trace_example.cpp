//
// Created by kz.zhao on 2025/6/18.
//
// QBDI Trace module usage example

#include <chrono>
#include <iostream>
#include <thread>

#include "trace/qbdi.h"
#include "utility/Logger.h"

using namespace AnalysisToolkit;
using namespace AnalysisToolkit::Trace;

// 示例函数 - 被跟踪的目标函数
void target_function() {
    volatile int x = 0;
    for (int i = 0; i < 10; ++i) {
        x += i * 2;
    }
    std::cout << "Target function result: " << x << std::endl;
}

// 自定义指令回调函数
void instructionCallback(const InstructionInfo& info) {
    std::cout << "[CALLBACK] Address: 0x" << std::hex << info.address << " - " << info.disassembly
              << std::endl;
}

// 简单的目标函数用于演示
extern "C" int simple_add(int a, int b) {
    return a + b;
}

int main() {
    // 初始化Logger
    auto* logger = Logger::getInstance();
    logger->initialize("TraceExample", "", LogLevel::DEBUG, true);

    std::cout << "=== QBDI Trace Module Example ===" << std::endl;

    /*
     * 重要说明：QBDI没有捕获到指令的原因
     *
     * 1. QBDI是一个动态二进制插桩框架，它需要在其虚拟机环境中执行代码
     * 2. 直接调用函数（如 target_function()）是在原生CPU上执行的，不会被QBDI拦截
     * 3. QBDI需要：
     *    - 完整的程序上下文（栈、寄存器状态等）
     *    - 正确设置内存映射和权限
     *    - 适当的函数调用约定模拟
     *
     * 正确的QBDI使用场景：
     * - 分析外部二进制文件
     * - Hook系统调用或库函数
     * - 运行时代码插桩和分析
     *
     * 当前的实现更适合作为框架基础，实际使用时需要：
     * - 使用QBDI的preload方式来hook整个程序
     * - 或者在独立的进程中运行目标代码
     * - 或者使用QBDI分析预编译的二进制文件
     */

    // 初始化trace模块
    if (!Global::initialize()) {
        std::cerr << "Failed to initialize trace module" << std::endl;
        return -1;
    }

    std::cout << "Trace module initialized successfully" << std::endl;

    // 获取跟踪器实例
    auto* tracer = Global::getTracer();
    if (!tracer) {
        std::cerr << "Failed to get tracer instance" << std::endl;
        Global::cleanup();
        return -1;
    }

    // 设置自定义回调函数
    // tracer->setInstructionCallback(instructionCallback);

    // 启用指令日志
    tracer->enableInstructionLogging(true);

    std::cout << "Starting instruction trace..." << std::endl;

    // 获取目标函数的地址范围
    uint64_t func_addr = reinterpret_cast<uint64_t>(&target_function);
    uint64_t func_size = 1024;  // 假设函数大小为1KB

    std::cout << "Target function address: 0x" << std::hex << func_addr << std::endl;

    // 方法1：使用便捷接口
    bool success = Global::quickStartTrace(func_addr, func_addr + func_size, true);
    if (!success) {
        std::cerr << "Failed to start trace" << std::endl;
        Global::cleanup();
        return -1;
    }

    std::cout << "Trace started. Executing target function to trigger instrumentation..."
              << std::endl;

    // 注意：在真实的QBDI使用中，我们通常有几种方法：
    // 1. 使用QBDI运行整个程序
    // 2. 使用QBDI hook指定的函数
    // 3. 使用QBDI的call方法来调用函数

    // 现在我们先执行目标函数，然后看看是否能捕获到指令
    std::cout << "Calling target_function()..." << std::endl;
    target_function();

    std::cout << "Calling simple_add(3, 5)..." << std::endl;
    int result = simple_add(3, 5);
    std::cout << "simple_add result: " << result << std::endl;

    // 等待一下让跟踪完成
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // 获取统计信息
    auto stats = tracer->getStats();
    std::cout << "\n=== Trace Statistics ===" << std::endl;
    std::cout << "Instructions traced: " << stats.instruction_count << std::endl;
    std::cout << "Execution time: " << stats.execution_time_ms << " ms" << std::endl;
    std::cout << "Traced ranges: " << stats.traced_addresses_count << std::endl;

    // 停止跟踪
    Global::stopTrace();
    std::cout << "Trace stopped" << std::endl;

    // 演示方法2：直接使用跟踪器对象
    std::cout << "\n=== Method 2: Direct tracer usage ===" << std::endl;

    // 尝试跟踪simple_add函数
    uint64_t simple_add_addr = reinterpret_cast<uint64_t>(&simple_add);
    uint64_t simple_add_size = 256;  // 假设函数大小

    std::cout << "Simple_add function address: 0x" << std::hex << simple_add_addr << std::endl;

    // 为simple_add函数添加跟踪范围并设置回调
    // tracer->setInstructionCallback(instructionCallback);
    // tracer->enableInstructionLogging(true);

    if (tracer->startTrace(simple_add_addr, simple_add_addr + simple_add_size)) {
        std::cout << "Direct trace started for simple_add" << std::endl;

        // 使用QBDI虚拟机调用函数来触发跟踪
        std::cout << "Calling simple_add(10, 20) through QBDI VM..." << std::endl;
        uint64_t result2 = tracer->callFunction(simple_add_addr, {10, 20});
        std::cout << "simple_add result (via QBDI): " << result2 << std::endl;

        // 停止跟踪
        tracer->stopTrace();

        // 获取新的统计信息
        auto new_stats = tracer->getStats();
        std::cout << "New trace - Instructions: " << new_stats.instruction_count << std::endl;
    }

    // 清理资源
    Global::cleanup();
    std::cout << "Trace module cleaned up" << std::endl;

    return 0;
}
