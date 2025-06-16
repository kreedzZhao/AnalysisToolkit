#ifndef ANALYSIS_TOOLKIT_LOGGER_H
#define ANALYSIS_TOOLKIT_LOGGER_H

#include <atomic>
#include <fstream>
#include <memory>
#include <mutex>
#include <string>

#ifdef __ANDROID__
#include <android/log.h>
#endif

namespace AnalysisToolkit {
enum class LogLevel { TRACE = 0, DEBUG = 1, INFO = 2, WARN = 3, ERROR = 4, FATAL = 5 };

class Logger {
  private:
    static std::unique_ptr<Logger> instance_;
    static std::mutex instance_mutex_;
    mutable std::mutex file_mutex_;
    mutable std::ofstream file_stream_;
    std::atomic<bool> file_enabled_{false};
    std::atomic<bool> console_enabled_{true};
    std::atomic<LogLevel> min_level_{LogLevel::DEBUG};
    std::string log_file_path_;
    std::string tag_;

    void writeLog(LogLevel level, const std::string& message) const;
    const char* getLevelString(LogLevel level) const;

  public:
    static Logger* getInstance();
    ~Logger();

    bool initialize(const std::string& tag = "AnalysisToolkit",
                    const std::string& file_path = "",
                    LogLevel min_level = LogLevel::DEBUG,
                    bool console_enabled = true);

    void setTag(const std::string& tag);
    void setMinLevel(LogLevel level);
    void enableConsole(bool enabled);
    void enableFile(bool enabled);
    bool setLogFile(const std::string& file_path);

    void trace(const std::string& message) const;
    void debug(const std::string& message) const;
    void info(const std::string& message) const;
    void warn(const std::string& message) const;
    void error(const std::string& message) const;
    void fatal(const std::string& message) const;

    template <typename... Args>
    void info(const std::string& format, Args... args) const {
        if (min_level_.load() <= LogLevel::INFO) {
            char buffer[1024];
            snprintf(buffer, sizeof(buffer), format.c_str(), args...);
            writeLog(LogLevel::INFO, std::string(buffer));
        }
    }

    template <typename... Args>
    void debug(const std::string& format, Args... args) const {
        if (min_level_.load() <= LogLevel::DEBUG) {
            char buffer[1024];
            snprintf(buffer, sizeof(buffer), format.c_str(), args...);
            writeLog(LogLevel::DEBUG, std::string(buffer));
        }
    }

    template <typename... Args>
    void warn(const std::string& format, Args... args) const {
        if (min_level_.load() <= LogLevel::WARN) {
            char buffer[1024];
            snprintf(buffer, sizeof(buffer), format.c_str(), args...);
            writeLog(LogLevel::WARN, std::string(buffer));
        }
    }

    template <typename... Args>
    void error(const std::string& format, Args... args) const {
        if (min_level_.load() <= LogLevel::ERROR) {
            char buffer[1024];
            snprintf(buffer, sizeof(buffer), format.c_str(), args...);
            writeLog(LogLevel::ERROR, std::string(buffer));
        }
    }

    void flush();
    bool isConsoleEnabled() const {
        return console_enabled_.load();
    }
    bool isFileEnabled() const {
        return file_enabled_.load();
    }
    LogLevel getMinLevel() const {
        return min_level_.load();
    }
    std::string getTag() const {
        return tag_;
    }
};

#define ATKIT_TRACE(msg) AnalysisToolkit::Logger::getInstance()->trace(msg)
#define ATKIT_DEBUG(...) AnalysisToolkit::Logger::getInstance()->debug(__VA_ARGS__)
#define ATKIT_INFO(...) AnalysisToolkit::Logger::getInstance()->info(__VA_ARGS__)
#define ATKIT_WARN(...) AnalysisToolkit::Logger::getInstance()->warn(__VA_ARGS__)
#define ATKIT_ERROR(...) AnalysisToolkit::Logger::getInstance()->error(__VA_ARGS__)
#define ATKIT_FATAL(msg) AnalysisToolkit::Logger::getInstance()->fatal(msg)
}  // namespace AnalysisToolkit

#endif
