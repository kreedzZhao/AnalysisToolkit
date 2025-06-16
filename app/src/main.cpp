#include "hook/inline_hook.h"

int main() {
    AnalysisToolkit::HookManager* hook_manager = AnalysisToolkit::HookManager::getInstance();
    if (hook_manager->initialize()) {
        ATKIT_INFO("HookManager initialized successfully");
    } else {
        ATKIT_ERROR("Failed to initialize HookManager");
        return -1;
    }

    // 这里可以添加更多的测试代码来验证 Hook 功能

    hook_manager->cleanup();
    return 0;
}