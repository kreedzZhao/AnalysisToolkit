#ifndef ANALYSIS_TOOLKIT_H
#define ANALYSIS_TOOLKIT_H

#include "Hook.h"
#include "Logger.h"
#include "Monitor.h"

namespace AnalysisToolkit {

struct Config {
    std::string app_tag = "AnalysisToolkit";
    std::string log_file_path = "";
    LogLevel log_level = LogLevel::DEBUG;
    bool enable_console_log = true;
    bool enable_file_log = false;

    bool enable_hook_manager = false;

    bool enable_jni_monitoring = false;
    MonitorConfig monitor_config = MonitorConfig{};
};

bool initialize(const Config& config = Config{});
void cleanup();
std::string getLibraryInfo();
bool isInitialized();

Logger* getLogger();
HookManager* getHookManager();
Monitor* getMonitor();

}  // namespace AnalysisToolkit

#endif  // ANALYSIS_TOOLKIT_H
