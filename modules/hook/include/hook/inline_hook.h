#ifndef ANALYSIS_TOOLKIT_HOOK_H
#define ANALYSIS_TOOLKIT_HOOK_H

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "dobby.h"
#include "utility/Logger.h"

#ifdef __ANDROID__
#include <jni.h>
#endif

namespace AnalysisToolkit {

// Hook 状态枚举
enum class HookStatus {
    SUCCESS = 0,
    FAILED = -1,
    ALREADY_HOOKED = -2,
    INVALID_ADDRESS = -3,
    SYMBOL_NOT_FOUND = -4,
    MEMORY_ERROR = -5
};

// Hook 类型枚举
enum class HookType {
    FUNCTION_INLINE,  // 函数内联 Hook
    INSTRUCTION,      // 指令级 Hook
    SYMBOL_RESOLVER   // 符号解析 Hook
};

// Hook 回调函数类型
using HookCallback = std::function<void(void*, void*, void*)>;
using InstrumentCallback = std::function<void(void*, void*)>;

// Hook 信息结构体
struct HookInfo {
    void* target_address;
    void* replace_function;
    void* original_function;
    HookType type;
    std::string symbol_name;
    std::string library_name;
    bool is_active;
};

class HookManager {
  private:
    std::unordered_map<void*, std::unique_ptr<HookInfo>> active_hooks_;

    // 内部工具方法
    void* resolveSymbol(const std::string& library_name, const std::string& symbol_name);
    bool isValidAddress(void* address);
    std::mutex& getHooksMutex() const;

  public:
    static HookManager* getInstance();
    ~HookManager();

    // Hook 函数地址
    HookStatus hookFunction(void* target_address,
                            void* replace_function,
                            void** original_function,
                            const std::string& tag = "");

    // Hook 库符号
    HookStatus hookSymbol(const std::string& library_name,
                          const std::string& symbol_name,
                          void* replace_function,
                          void** original_function,
                          const std::string& tag = "");

    // 指令级插桩
    HookStatus instrumentFunction(void* target_address,
                                  InstrumentCallback pre_callback,
                                  const std::string& tag = "");

    // 移除 Hook
    HookStatus unhookFunction(void* target_address);

    // 查询 Hook 状态
    bool isHooked(void* target_address) const;
    HookInfo* getHookInfo(void* target_address) const;

    // 获取所有 Hook 信息
    std::vector<HookInfo*> getAllHooks() const;

    // 工具方法
    void* getSymbolAddress(const std::string& library_name, const std::string& symbol_name);
    std::string getLibraryPath(void* address);

    // 初始化和清理
    bool initialize();
    void cleanup();
};

// Hook 辅助宏定义
#define ATKIT_HOOK_DEF(ret_type, func_name, ...)             \
    ret_type (*original_##func_name)(__VA_ARGS__) = nullptr; \
    ret_type hooked_##func_name(__VA_ARGS__)

#define ATKIT_HOOK_SYMBOL(lib_name, symbol, func_name)                                      \
    HookManager::getInstance()->hookSymbol(lib_name,                                        \
                                           symbol,                                          \
                                           reinterpret_cast<void*>(hooked_##func_name),     \
                                           reinterpret_cast<void**>(&original_##func_name), \
                                           #func_name)

#define ATKIT_HOOK_ADDRESS(address, func_name)                                                \
    HookManager::getInstance()->hookFunction(address,                                         \
                                             reinterpret_cast<void*>(hooked_##func_name),     \
                                             reinterpret_cast<void**>(&original_##func_name), \
                                             #func_name)

#define ATKIT_UNHOOK(address) HookManager::getInstance()->unhookFunction(address)

}  // namespace AnalysisToolkit

#endif  // ANALYSIS_TOOLKIT_HOOK_H
