#include "hook/inline_hook.h"

void (*original_printf)(const char*, ...);
void hooked_printf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    // 在这里可以添加自定义的处理逻辑
    original_printf("[Hooked] ");
    original_printf(format, args);
    va_end(args);
}

int main() {
    AnalysisToolkit::HookManager* hook_manager = AnalysisToolkit::HookManager::getInstance();
    if (hook_manager->initialize()) {
        ATKIT_INFO("HookManager initialized successfully");
    } else {
        ATKIT_ERROR("Failed to initialize HookManager");
        return -1;
    }

    printf("above nihao from printf\n");

    auto symbol_address = hook_manager->getSymbolAddress("libsystem_c.dylib", "printf");
    if (symbol_address == nullptr) {
        ATKIT_ERROR("Failed to resolve symbol address for printf");
        return -1;
    }
    auto hook_function = hook_manager->hookFunction(symbol_address,
                                                    reinterpret_cast<void*>(hooked_printf),
                                                    reinterpret_cast<void**>(&original_printf),
                                                    "printf_hook");
    if (hook_function != AnalysisToolkit::HookStatus::SUCCESS) {
        ATKIT_ERROR("Failed to hook printf function");
        return -1;
    }

    // 测试 Hook 是否成功
    printf("nihao from prinf\n");

    hook_manager->cleanup();
    return 0;
}
