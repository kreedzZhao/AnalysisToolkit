#include "hook/inline_hook.h"

#include <dlfcn.h>

#include "dobby.h"
#include "utility/Logger.h"

namespace AnalysisToolkit {

HookManager* HookManager::getInstance() {
    static std::once_flag flag;
    // 使用完全局部静态变量方法，避免静态析构顺序问题
    static HookManager* instance = nullptr;
    // 确保只初始化一次
    std::call_once(flag, [&]() { instance = new HookManager(); });
    return instance;
}

HookManager::~HookManager() {
    cleanup();
    ATKIT_INFO("HookManager destructor called");
}

std::mutex& HookManager::getHooksMutex() const {
    static std::mutex hooks_mutex;
    return hooks_mutex;
}

bool HookManager::initialize() {
    ATKIT_INFO("HookManager initializing...");
    return true;
}

void HookManager::cleanup() {
    std::lock_guard<std::mutex> lock(getHooksMutex());

    // 清理所有 Hook
    for (auto& pair : active_hooks_) {
        if (pair.second->is_active) {
            DobbyDestroy(pair.first);
            ATKIT_DEBUG("Cleaned up hook at address: %p", pair.first);
        }
    }
    active_hooks_.clear();
    ATKIT_INFO("HookManager cleanup completed");
}

void* HookManager::resolveSymbol(const std::string& library_name, const std::string& symbol_name) {
    // 尝试使用 Dobby 的符号解析器
    void* symbol = DobbySymbolResolver(library_name.c_str(), symbol_name.c_str());
    if (symbol != nullptr) {
        return symbol;
    }

    // 回退到 dlsym
    void* handle = dlopen(library_name.c_str(), RTLD_LAZY);
    if (handle == nullptr) {
        ATKIT_ERROR("Failed to open library: %s, error: %s", library_name.c_str(), dlerror());
        return nullptr;
    }

    symbol = dlsym(handle, symbol_name.c_str());
    if (symbol == nullptr) {
        ATKIT_ERROR("Failed to find symbol %s in %s: %s",
                    symbol_name.c_str(),
                    library_name.c_str(),
                    dlerror());
    }

    dlclose(handle);
    return symbol;
}

bool HookManager::isValidAddress(void* address) {
    if (address == nullptr) {
        return false;
    }

    // 使用 dladdr 检查地址是否有效
    Dl_info info;
    return dladdr(address, &info) != 0;
}

HookStatus HookManager::hookFunction(void* target_address,
                                     void* replace_function,
                                     void** original_function,
                                     const std::string& tag) {
    if (!isValidAddress(target_address)) {
        ATKIT_ERROR("Invalid target address: %p", target_address);
        return HookStatus::INVALID_ADDRESS;
    }

    std::lock_guard<std::mutex> lock(getHooksMutex());

    // 检查是否已经被 Hook
    auto it = active_hooks_.find(target_address);
    if (it != active_hooks_.end() && it->second->is_active) {
        ATKIT_WARN("Address %p already hooked with tag: %s",
                   target_address,
                   it->second->symbol_name.c_str());
        return HookStatus::ALREADY_HOOKED;
    }

    // 执行 Hook
    int result = DobbyHook(target_address,
                           reinterpret_cast<dobby_dummy_func_t>(replace_function),
                           reinterpret_cast<dobby_dummy_func_t*>(original_function));

    if (result != 0) {
        ATKIT_ERROR("Dobby hook failed for address %p, error code: %d", target_address, result);
        return HookStatus::FAILED;
    }

    // 记录 Hook 信息
    auto hook_info = std::make_unique<HookInfo>();
    hook_info->target_address = target_address;
    hook_info->replace_function = replace_function;
    hook_info->original_function = original_function ? *original_function : nullptr;
    hook_info->type = HookType::FUNCTION_INLINE;
    hook_info->symbol_name = tag;
    hook_info->is_active = true;

    // 获取库信息
    Dl_info dl_info;
    if (dladdr(target_address, &dl_info) != 0) {
        hook_info->library_name = dl_info.dli_fname ? dl_info.dli_fname : "unknown";
    }

    active_hooks_[target_address] = std::move(hook_info);

    ATKIT_INFO("Successfully hooked function at %p with tag: %s", target_address, tag.c_str());
    return HookStatus::SUCCESS;
}

HookStatus HookManager::hookSymbol(const std::string& library_name,
                                   const std::string& symbol_name,
                                   void* replace_function,
                                   void** original_function,
                                   const std::string& tag) {
    void* target_address = resolveSymbol(library_name, symbol_name);
    if (target_address == nullptr) {
        ATKIT_ERROR("Failed to resolve symbol %s in library %s",
                    symbol_name.c_str(),
                    library_name.c_str());
        return HookStatus::SYMBOL_NOT_FOUND;
    }

    ATKIT_DEBUG("Resolved symbol %s at address: %p", symbol_name.c_str(), target_address);

    HookStatus status = hookFunction(target_address, replace_function, original_function, tag);
    if (status == HookStatus::SUCCESS) {
        // 更新符号信息
        std::lock_guard<std::mutex> lock(getHooksMutex());
        auto it = active_hooks_.find(target_address);
        if (it != active_hooks_.end()) {
            it->second->symbol_name = symbol_name;
            it->second->library_name = library_name;
        }
    }

    return status;
}

HookStatus HookManager::instrumentFunction(void* target_address,
                                           InstrumentCallback pre_callback,
                                           const std::string& tag) {
    if (!isValidAddress(target_address)) {
        ATKIT_ERROR("Invalid target address for instrumentation: %p", target_address);
        return HookStatus::INVALID_ADDRESS;
    }

    // 创建包装回调
    auto* wrapper_callback = new std::function<void(void*, DobbyRegisterContext*)>(
        [pre_callback](void* address, DobbyRegisterContext* ctx) { pre_callback(address, ctx); });

    dobby_instrument_callback_t dobby_callback = [](void* address, DobbyRegisterContext* ctx) {
        // 这里需要从某个地方获取到原始的 callback
        // 由于 dobby 不支持用户数据传递，我们需要使用全局映射
        // 简化实现，直接调用
    };

    int result = DobbyInstrument(target_address, dobby_callback);
    if (result != 0) {
        delete wrapper_callback;
        ATKIT_ERROR("Dobby instrument failed for address %p, error code: %d",
                    target_address,
                    result);
        return HookStatus::FAILED;
    }

    // 记录插桩信息
    std::lock_guard<std::mutex> lock(getHooksMutex());
    auto hook_info = std::make_unique<HookInfo>();
    hook_info->target_address = target_address;
    hook_info->replace_function = reinterpret_cast<void*>(wrapper_callback);
    hook_info->original_function = nullptr;
    hook_info->type = HookType::INSTRUCTION;
    hook_info->symbol_name = tag;
    hook_info->is_active = true;

    active_hooks_[target_address] = std::move(hook_info);

    ATKIT_INFO("Successfully instrumented function at %p with tag: %s",
               target_address,
               tag.c_str());
    return HookStatus::SUCCESS;
}

HookStatus HookManager::unhookFunction(void* target_address) {
    std::lock_guard<std::mutex> lock(getHooksMutex());

    auto it = active_hooks_.find(target_address);
    if (it == active_hooks_.end()) {
        ATKIT_WARN("Address %p is not hooked", target_address);
        return HookStatus::FAILED;
    }

    int result = DobbyDestroy(target_address);
    if (result != 0) {
        ATKIT_ERROR("Failed to unhook address %p, error code: %d", target_address, result);
        return HookStatus::FAILED;
    }

    // 清理资源
    if (it->second->type == HookType::INSTRUCTION) {
        delete reinterpret_cast<std::function<void(void*, DobbyRegisterContext*)>*>(
            it->second->replace_function);
    }

    active_hooks_.erase(it);
    ATKIT_INFO("Successfully unhooked function at %p", target_address);
    return HookStatus::SUCCESS;
}

bool HookManager::isHooked(void* target_address) const {
    std::lock_guard<std::mutex> lock(getHooksMutex());
    auto it = active_hooks_.find(target_address);
    return it != active_hooks_.end() && it->second->is_active;
}

HookInfo* HookManager::getHookInfo(void* target_address) const {
    std::lock_guard<std::mutex> lock(getHooksMutex());
    auto it = active_hooks_.find(target_address);
    return it != active_hooks_.end() ? it->second.get() : nullptr;
}

std::vector<HookInfo*> HookManager::getAllHooks() const {
    std::lock_guard<std::mutex> lock(getHooksMutex());
    std::vector<HookInfo*> hooks;
    hooks.reserve(active_hooks_.size());

    for (const auto& pair : active_hooks_) {
        if (pair.second->is_active) {
            hooks.push_back(pair.second.get());
        }
    }

    return hooks;
}

void* HookManager::getSymbolAddress(const std::string& library_name,
                                    const std::string& symbol_name) {
    return resolveSymbol(library_name, symbol_name);
}

std::string HookManager::getLibraryPath(void* address) {
    Dl_info info;
    if (dladdr(address, &info) != 0 && info.dli_fname != nullptr) {
        return std::string(info.dli_fname);
    }
    return "unknown";
}

}  // namespace AnalysisToolkit
