#ifndef ANALYSIS_TOOLKIT_H
#define ANALYSIS_TOOLKIT_H

#include "hook/inline_hook.h"
#include "utility/Logger.h"

namespace AnalysisToolkit {

struct Config {
    std::string app_tag = "AnalysisToolkit";
    std::string log_file_path = "";
    LogLevel log_level = LogLevel::DEBUG;
    bool enable_console_log = true;
    bool enable_file_log = false;

    bool enable_hook_manager = false;
};

bool initialize(const Config& config = Config{});
void cleanup();
std::string getLibraryInfo();
bool isInitialized();

Logger* getLogger();
HookManager* getHookManager();

}  // namespace AnalysisToolkit

#endif  // ANALYSIS_TOOLKIT_H
