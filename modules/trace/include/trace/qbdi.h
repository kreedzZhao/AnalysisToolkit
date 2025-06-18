//
// Created by kz.zhao on 2025/6/18.
//

#ifndef TRACE_QBDI_H
#define TRACE_QBDI_H

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

namespace AnalysisToolkit {

// 前向声明
class Logger;

namespace Trace {

// 指令信息结构
struct InstructionInfo {
    uint64_t address;         // 指令地址
    std::string mnemonic;     // 指令助记符
    std::string operand;      // 操作数
    uint64_t thread_id;       // 线程ID
    std::string disassembly;  // 完整反汇编字符串
};

// 跟踪回调函数类型
using InstructionCallback = std::function<void(const InstructionInfo& info)>;

// QBDI跟踪管理器
class QBDITracer {
  public:
    QBDITracer();
    ~QBDITracer();

    // 初始化跟踪器
    bool initialize();

    // 清理资源
    void cleanup();

    // 检查是否已初始化
    bool isInitialized() const;

    // 开始跟踪指定地址范围
    bool startTrace(uint64_t start_addr, uint64_t end_addr);

    // 开始跟踪整个模块
    bool startTraceModule(const std::string& module_name);

    // 停止跟踪
    void stopTrace();

    // 检查是否正在跟踪
    bool isTracing() const;

    // 设置指令回调函数
    void setInstructionCallback(InstructionCallback callback);

    // 启用/禁用指令打印到日志
    void enableInstructionLogging(bool enable = true);

    // 设置日志级别过滤
    void setLogLevel(int level);

    // 运行跟踪（阻塞式）
    void run();

    // 通过QBDI虚拟机调用函数（这样可以被插桩）
    uint64_t callFunction(uint64_t func_addr, const std::vector<uint64_t>& args = {});

    // 获取跟踪统计信息
    struct TraceStats {
        uint64_t instruction_count;
        uint64_t execution_time_ms;
        uint64_t traced_addresses_count;
    };

    TraceStats getStats() const;

  private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

// 全局接口函数
namespace Global {

// 初始化trace模块
bool initialize();

// 清理trace模块
void cleanup();

// 获取全局跟踪器实例
QBDITracer* getTracer();

// 快速开始跟踪（便捷接口）
bool quickStartTrace(uint64_t start_addr, uint64_t end_addr, bool enable_logging = true);

// 快速开始模块跟踪
bool quickStartModuleTrace(const std::string& module_name, bool enable_logging = true);

// 停止跟踪
void stopTrace();

// 检查是否正在跟踪
bool isTracing();
}  // namespace Global

}  // namespace Trace
}  // namespace AnalysisToolkit

#endif  // TRACE_QBDI_H
