//
// Created by kz.zhao on 2025/6/18.
//

#include "trace/qbdi.h"

#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>
#include <unordered_set>

#include "QBDI.h"
#include "utility/Logger.h"

namespace AnalysisToolkit {
namespace Trace {

// QBDITracer的私有实现类
class QBDITracer::Impl {
  public:
    Impl()
        : vm_(nullptr),
          initialized_(false),
          tracing_(false),
          enable_logging_(true),
          log_level_(0),
          instruction_count_(0),
          start_time_(0),
          stack_base_(0),
          stack_size_(0) {
        logger_ = Logger::getInstance();
    }

    ~Impl() {
        cleanup();
    }

    bool initialize() {
        if (initialized_) {
            return true;
        }

        try {
            // 初始化QBDI VM
            vm_ = new QBDI::VM();
            if (!vm_) {
                if (logger_) {
                    logger_->error("Failed to create QBDI VM");
                }
                return false;
            }

            // 获取当前进程状态并设置到VM中
            QBDI::GPRState* state = vm_->getGPRState();

            // 尝试获取当前线程的寄存器状态
            try {
                // 设置基本的栈指针 - 为VM分配一个栈空间
                stack_size_ = 0x10000;  // 64KB栈空间
                stack_base_ = reinterpret_cast<uint64_t>(malloc(stack_size_));
                if (stack_base_ == 0) {
                    if (logger_) {
                        logger_->error("Failed to allocate stack for QBDI VM");
                    }
                    return false;
                }

                // 设置栈指针到栈顶
                uint64_t stack_top = stack_base_ + stack_size_;
                vm_->getGPRState()->sp = stack_top;

                if (logger_) {
                    logger_->info("QBDI VM stack allocated: 0x%lx - 0x%lx", stack_base_, stack_top);
                }

            } catch (const std::exception& e) {
                if (logger_) {
                    logger_->warn("Exception setting VM state: %s", e.what());
                }
            }

            initialized_ = true;
            if (logger_) {
                logger_->info("QBDI Tracer initialized successfully");
            }
            return true;

        } catch (const std::exception& e) {
            if (logger_) {
                logger_->error("Exception during QBDI initialization: %s", e.what());
            }
            cleanup();
            return false;
        }
    }

    void cleanup() {
        if (tracing_) {
            stopTrace();
        }

        if (vm_) {
            delete vm_;
            vm_ = nullptr;
        }

        // 释放分配的栈空间
        if (stack_base_ != 0) {
            free(reinterpret_cast<void*>(stack_base_));
            stack_base_ = 0;
            stack_size_ = 0;
        }

        initialized_ = false;
        if (logger_) {
            logger_->info("QBDI Tracer cleaned up");
        }
    }

    bool isInitialized() const {
        return initialized_;
    }

    bool startTrace(uint64_t start_addr, uint64_t end_addr) {
        if (!initialized_) {
            if (logger_) {
                logger_->error("QBDI Tracer not initialized");
            }
            return false;
        }

        if (tracing_) {
            stopTrace();
        }

        try {
            // 添加指令范围
            vm_->addInstrumentedRange(start_addr, end_addr);
            if (logger_) {
                logger_->info("Added instrumented range [0x%lx, 0x%lx]", start_addr, end_addr);
            }

            // 注册指令回调
            uint32_t iid = vm_->addCodeCB(QBDI::PREINST, instructionCallback, this);
            if (iid == QBDI::INVALID_EVENTID) {
                if (logger_) {
                    logger_->error("Failed to register instruction callback");
                }
                return false;
            }

            callback_ids_.insert(iid);
            traced_ranges_.emplace_back(start_addr, end_addr);

            tracing_ = true;
            instruction_count_ = 0;
            start_time_ = getCurrentTimeMs();

            if (logger_) {
                logger_->info("Started tracing range [0x%lx, 0x%lx]", start_addr, end_addr);
            }

            return true;

        } catch (const std::exception& e) {
            if (logger_) {
                logger_->error("Exception during trace start: %s", e.what());
            }
            return false;
        }
    }

    bool startTraceModule(const std::string& module_name) {
        if (!initialized_) {
            if (logger_) {
                logger_->error("QBDI Tracer not initialized");
            }
            return false;
        }

        // 简化实现：获取主程序的文本段
        // 在实际应用中，可以通过解析模块信息来获取准确的地址范围
        try {
            std::vector<QBDI::MemoryMap> maps = QBDI::getCurrentProcessMaps();
            for (const auto& map : maps) {
                if (map.permission & QBDI::PF_EXEC) {
                    // 找到可执行段，开始跟踪
                    return startTrace(map.range.start(), map.range.end());
                }
            }

            if (logger_) {
                logger_->error("No executable segment found for module: %s", module_name.c_str());
            }
            return false;

        } catch (const std::exception& e) {
            if (logger_) {
                logger_->error("Exception during module trace start: %s", e.what());
            }
            return false;
        }
    }

    void stopTrace() {
        if (!tracing_) {
            return;
        }

        try {
            // 移除所有回调
            for (uint32_t id : callback_ids_) {
                vm_->deleteInstrumentation(id);
            }
            callback_ids_.clear();

            // 清理范围
            for (const auto& range : traced_ranges_) {
                vm_->removeInstrumentedRange(range.first, range.second);
            }
            traced_ranges_.clear();

            tracing_ = false;
            uint64_t end_time = getCurrentTimeMs();
            uint64_t execution_time = end_time - start_time_;

            if (logger_) {
                logger_->info("Stopped tracing. Instructions: %lu, Time: %lu ms",
                              instruction_count_.load(),
                              execution_time);
            }

        } catch (const std::exception& e) {
            if (logger_) {
                logger_->error("Exception during trace stop: %s", e.what());
            }
        }
    }

    bool isTracing() const {
        return tracing_;
    }

    void setInstructionCallback(InstructionCallback callback) {
        std::lock_guard<std::mutex> lock(callback_mutex_);
        user_callback_ = callback;
    }

    void enableInstructionLogging(bool enable) {
        enable_logging_ = enable;
    }

    void setLogLevel(int level) {
        log_level_ = level;
    }

    void run() {
        if (!tracing_) {
            if (logger_) {
                logger_->warn("Cannot run: not tracing");
            }
            return;
        }

        if (traced_ranges_.empty()) {
            if (logger_) {
                logger_->warn("No traced ranges defined");
            }
            return;
        }

        try {
            // 从第一个追踪范围的起始地址开始运行
            uint64_t start_addr = traced_ranges_[0].first;
            uint64_t end_addr = traced_ranges_[0].second;

            if (logger_) {
                logger_->info("Running QBDI VM from 0x%lx to 0x%lx", start_addr, end_addr);
            }

            // 运行执行引擎
            bool success = vm_->run(start_addr, end_addr);
            if (!success) {
                if (logger_) {
                    logger_->error("VM run failed");
                }
            }
        } catch (const std::exception& e) {
            if (logger_) {
                logger_->error("Exception during VM run: %s", e.what());
            }
        }
    }

    uint64_t callFunction(uint64_t func_addr, const std::vector<uint64_t>& args) {
        if (!initialized_) {
            if (logger_) {
                logger_->error("Cannot call function: QBDI not initialized");
            }
            return 0;
        }

        try {
            if (logger_) {
                logger_->info("Calling function at 0x%lx with %zu arguments",
                              func_addr,
                              args.size());
            }

            // 确保函数地址在跟踪范围内
            bool in_range = false;
            for (const auto& range : traced_ranges_) {
                if (func_addr >= range.first && func_addr < range.second) {
                    in_range = true;
                    break;
                }
            }

            if (!in_range) {
                if (logger_) {
                    logger_->warn(
                        "Function address 0x%lx not in traced ranges, adding temporary range",
                        func_addr);
                }
                // 临时添加包含该函数的范围
                vm_->addInstrumentedRange(func_addr, func_addr + 1024);
            }

            // 准备参数 - 根据调用约定设置寄存器
            QBDI::GPRState* gprState = vm_->getGPRState();

            // ARM64调用约定：前8个参数通过x0-x7传递
            if (args.size() > 0 && args.size() <= 8) {
                QBDI::rword* regs = &gprState->x0;
                for (size_t i = 0; i < args.size() && i < 8; ++i) {
                    regs[i] = static_cast<QBDI::rword>(args[i]);
                    if (logger_) {
                        logger_->debug("Set x%zu = 0x%lx", i, args[i]);
                    }
                }
            }

            // 设置返回地址为特殊值，便于识别函数返回
            uint64_t return_addr = 0xDEADBEEF;
            gprState->lr = return_addr;

            if (logger_) {
                logger_->info("Starting function execution via QBDI run()");
                logger_->debug("Stack pointer: 0x%lx", gprState->sp);
                logger_->debug("Function address: 0x%lx", func_addr);
            }

            // 使用vm->run()从函数地址开始执行，直到返回
            bool success = vm_->run(func_addr, return_addr);

            if (!success) {
                if (logger_) {
                    logger_->error("VM run failed");
                }
                return 0;
            }

            // 获取返回值 (ARM64约定：返回值在x0寄存器中)
            uint64_t result = static_cast<uint64_t>(gprState->x0);

            if (logger_) {
                logger_->info("Function call completed, result: 0x%lx (%lu)", result, result);
            }

            return result;

        } catch (const std::exception& e) {
            if (logger_) {
                logger_->error("Exception during function call: %s", e.what());
            }
            return 0;
        }
    }

    QBDITracer::TraceStats getStats() const {
        QBDITracer::TraceStats stats;
        stats.instruction_count = instruction_count_.load();
        stats.execution_time_ms = tracing_ ? (getCurrentTimeMs() - start_time_) : 0;
        stats.traced_addresses_count = traced_ranges_.size();
        return stats;
    }

  private:
    // 指令回调函数
    static QBDI::VMAction instructionCallback(QBDI::VMInstanceRef vm,
                                              QBDI::GPRState* gprState,
                                              QBDI::FPRState* fprState,
                                              void* data) {
        Impl* impl = static_cast<Impl*>(data);
        return impl->handleInstruction(vm, gprState, fprState);
    }

    QBDI::VMAction handleInstruction(QBDI::VMInstanceRef vm,
                                     QBDI::GPRState* gprState,
                                     QBDI::FPRState* fprState) {
        try {
            instruction_count_++;

            // 获取当前指令信息
            const QBDI::InstAnalysis* analysis = vm_->getInstAnalysis(QBDI::ANALYSIS_INSTRUCTION);
            if (!analysis) {
                return QBDI::VMAction::CONTINUE;
            }

            InstructionInfo info;
            info.address = analysis->address;
            info.thread_id = std::hash<std::thread::id>{}(std::this_thread::get_id());

            // 获取反汇编信息
            const QBDI::InstAnalysis* instAnalysis =
                vm_->getInstAnalysis(QBDI::ANALYSIS_DISASSEMBLY);
            if (instAnalysis) {
                info.mnemonic = instAnalysis->mnemonic ? instAnalysis->mnemonic : "";
                info.disassembly = instAnalysis->disassembly ? instAnalysis->disassembly : "";
            }

            // 记录到日志
            if (enable_logging_ && logger_) {
                logger_->debug("0x%lx: %s", info.address, info.disassembly.c_str());
            }

            // 调用用户回调
            {
                std::lock_guard<std::mutex> lock(callback_mutex_);
                if (user_callback_) {
                    user_callback_(info);
                }
            }

            return QBDI::VMAction::CONTINUE;

        } catch (const std::exception& e) {
            if (logger_) {
                logger_->error("Exception in instruction callback: %s", e.what());
            }
            return QBDI::VMAction::STOP;
        }
    }

    uint64_t getCurrentTimeMs() const {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch())
            .count();
    }

  private:
    QBDI::VM* vm_;
    Logger* logger_;

    std::atomic<bool> initialized_;
    std::atomic<bool> tracing_;
    bool enable_logging_;
    int log_level_;

    std::atomic<uint64_t> instruction_count_;
    uint64_t start_time_;

    std::unordered_set<uint32_t> callback_ids_;
    std::vector<std::pair<uint64_t, uint64_t>> traced_ranges_;

    std::mutex callback_mutex_;
    InstructionCallback user_callback_;

    // VM栈空间管理
    uint64_t stack_base_;
    size_t stack_size_;
};

// QBDITracer 公共接口实现
QBDITracer::QBDITracer() : pImpl(std::make_unique<Impl>()) {}

QBDITracer::~QBDITracer() = default;

bool QBDITracer::initialize() {
    return pImpl->initialize();
}

void QBDITracer::cleanup() {
    pImpl->cleanup();
}

bool QBDITracer::isInitialized() const {
    return pImpl->isInitialized();
}

bool QBDITracer::startTrace(uint64_t start_addr, uint64_t end_addr) {
    return pImpl->startTrace(start_addr, end_addr);
}

bool QBDITracer::startTraceModule(const std::string& module_name) {
    return pImpl->startTraceModule(module_name);
}

void QBDITracer::stopTrace() {
    pImpl->stopTrace();
}

bool QBDITracer::isTracing() const {
    return pImpl->isTracing();
}

void QBDITracer::setInstructionCallback(InstructionCallback callback) {
    pImpl->setInstructionCallback(callback);
}

void QBDITracer::enableInstructionLogging(bool enable) {
    pImpl->enableInstructionLogging(enable);
}

void QBDITracer::setLogLevel(int level) {
    pImpl->setLogLevel(level);
}

void QBDITracer::run() {
    pImpl->run();
}

uint64_t QBDITracer::callFunction(uint64_t func_addr, const std::vector<uint64_t>& args) {
    return pImpl->callFunction(func_addr, args);
}

QBDITracer::TraceStats QBDITracer::getStats() const {
    return pImpl->getStats();
}

// 全局接口实现
namespace Global {

static std::unique_ptr<QBDITracer> g_tracer = nullptr;
static std::mutex g_tracer_mutex;

bool initialize() {
    std::lock_guard<std::mutex> lock(g_tracer_mutex);

    if (g_tracer) {
        return true;  // 已经初始化
    }

    g_tracer = std::make_unique<QBDITracer>();
    bool success = g_tracer->initialize();

    if (!success) {
        g_tracer.reset();
        return false;
    }

    auto* logger = Logger::getInstance();
    if (logger) {
        logger->info("Trace module initialized successfully");
    }

    return true;
}

void cleanup() {
    std::lock_guard<std::mutex> lock(g_tracer_mutex);

    if (g_tracer) {
        g_tracer->cleanup();
        g_tracer.reset();

        auto* logger = Logger::getInstance();
        if (logger) {
            logger->info("Trace module cleaned up");
        }
    }
}

QBDITracer* getTracer() {
    std::lock_guard<std::mutex> lock(g_tracer_mutex);
    return g_tracer.get();
}

bool quickStartTrace(uint64_t start_addr, uint64_t end_addr, bool enable_logging) {
    auto* tracer = getTracer();
    if (!tracer) {
        return false;
    }

    tracer->enableInstructionLogging(enable_logging);
    return tracer->startTrace(start_addr, end_addr);
}

bool quickStartModuleTrace(const std::string& module_name, bool enable_logging) {
    auto* tracer = getTracer();
    if (!tracer) {
        return false;
    }

    tracer->enableInstructionLogging(enable_logging);
    return tracer->startTraceModule(module_name);
}

void stopTrace() {
    auto* tracer = getTracer();
    if (tracer) {
        tracer->stopTrace();
    }
}

bool isTracing() {
    auto* tracer = getTracer();
    return tracer ? tracer->isTracing() : false;
}

}  // namespace Global

}  // namespace Trace
}  // namespace AnalysisToolkit
