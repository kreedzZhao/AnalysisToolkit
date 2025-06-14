#include "AnalysisToolkit/AnalysisToolkit.h"
#include <atomic>

namespace AnalysisToolkit {

    static std::atomic<bool> g_initialized{false};

    bool initialize(const Config& config) {
        if (g_initialized.load()) {
            return true; // 已经初始化
        }

        // 初始化 Logger
        auto* logger = Logger::getInstance();
        bool success = logger->initialize(
            config.app_tag,
            config.enable_file_log ? config.log_file_path : "",
            config.log_level,
            config.enable_console_log
        );

        if (!success) {
            return false;
        }

        logger->info("AnalysisToolkit initializing...");

        // 初始化 Hook 管理器
        if (config.enable_hook_manager) {
            HookManager* hook_manager = HookManager::getInstance();
            if (!hook_manager->initialize()) {
                logger->error("Failed to initialize HookManager");
                return false;
            }
            logger->info("HookManager initialized successfully");
        }

        // 初始化 Monitor
        if (config.enable_jni_monitoring) {
            Monitor* monitor = Monitor::getInstance();
            if (!monitor->initialize()) {
                logger->error("Failed to initialize Monitor");
                return false;
            }
            logger->info("Monitor initialized successfully");
        }

        g_initialized.store(true);
        logger->info("AnalysisToolkit initialized successfully");
        return true;
    }

    void cleanup() {
        if (g_initialized.load()) {
            auto* logger = Logger::getInstance();
            logger->info("AnalysisToolkit cleanup starting...");
            
            // 清理 Monitor
            Monitor::getInstance()->cleanup();
            
            // 清理 Hook 管理器
            HookManager::getInstance()->cleanup();
            
            logger->info("AnalysisToolkit cleanup completed");
            logger->flush();
            g_initialized.store(false);
        }
    }

    std::string getLibraryInfo() {
        return "AnalysisToolkit v1.0.0 - Android Native Analysis Library with Hook & Monitor capabilities";
    }

    bool isInitialized() {
        return g_initialized.load();
    }

    Logger* getLogger() {
        return Logger::getInstance();
    }

    HookManager* getHookManager() {
        return HookManager::getInstance();
    }

    Monitor* getMonitor() {
        return Monitor::getInstance();
    }

} // namespace AnalysisToolkit 