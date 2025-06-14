#include "AnalysisToolkit/Logger.h"
#include <cstdio>
#include <ctime>
#include <iomanip>
#include <unistd.h>

namespace AnalysisToolkit {

    std::unique_ptr<Logger> Logger::instance_ = nullptr;
    std::mutex Logger::instance_mutex_;

    Logger* Logger::getInstance() {
        std::lock_guard<std::mutex> lock(instance_mutex_);
        if (instance_ == nullptr) {
            instance_ = std::unique_ptr<Logger>(new Logger());
        }
        return instance_.get();
    }

    Logger::~Logger() {
        if (file_stream_.is_open()) {
            file_stream_.flush();
            file_stream_.close();
        }
    }

    bool Logger::initialize(const std::string& tag, 
                           const std::string& file_path, 
                           LogLevel min_level,
                           bool console_enabled) {
        tag_ = tag.empty() ? "AnalysisToolkit" : tag;
        min_level_.store(min_level);
        console_enabled_.store(console_enabled);
        
        if (!file_path.empty()) {
            return setLogFile(file_path);
        }
        
        return true;
    }

    void Logger::setTag(const std::string& tag) {
        tag_ = tag.empty() ? "AnalysisToolkit" : tag;
    }

    void Logger::setMinLevel(LogLevel level) {
        min_level_.store(level);
    }

    void Logger::enableConsole(bool enabled) {
        console_enabled_.store(enabled);
    }

    void Logger::enableFile(bool enabled) {
        file_enabled_.store(enabled);
    }

    bool Logger::setLogFile(const std::string& file_path) {
        std::lock_guard<std::mutex> lock(file_mutex_);
        
        if (file_stream_.is_open()) {
            file_stream_.flush();
            file_stream_.close();
        }
        
        log_file_path_ = file_path;
        
        if (file_path.empty()) {
            file_enabled_.store(false);
            return true;
        }
        
        file_stream_.open(file_path, std::ios::app);
        bool success = file_stream_.is_open();
        file_enabled_.store(success);
        
        return success;
    }

    const char* Logger::getLevelString(LogLevel level) const {
        switch (level) {
            case LogLevel::TRACE:   return "T";
            case LogLevel::DEBUG:   return "D";
            case LogLevel::INFO:    return "I";
            case LogLevel::WARN:    return "W";
            case LogLevel::ERROR:   return "E";
            case LogLevel::FATAL:   return "F";
            default:                return "?";
        }
    }

    void Logger::writeLog(LogLevel level, const std::string& message) const {
        if (level < min_level_.load()) {
            return;
        }
        
        if (console_enabled_.load()) {
            #ifdef __ANDROID__
            int priority = ANDROID_LOG_DEBUG;
            switch (level) {
                case LogLevel::TRACE:   priority = ANDROID_LOG_VERBOSE; break;
                case LogLevel::DEBUG:   priority = ANDROID_LOG_DEBUG; break;
                case LogLevel::INFO:    priority = ANDROID_LOG_INFO; break;
                case LogLevel::WARN:    priority = ANDROID_LOG_WARN; break;
                case LogLevel::ERROR:   priority = ANDROID_LOG_ERROR; break;
                case LogLevel::FATAL:   priority = ANDROID_LOG_FATAL; break;
            }
            __android_log_print(priority, tag_.c_str(), "%s", message.c_str());
            #else
            printf("[%s][%s] %s\n", getLevelString(level), tag_.c_str(), message.c_str());
            fflush(stdout);
            #endif
        }
        
        if (file_enabled_.load()) {
            std::lock_guard<std::mutex> lock(file_mutex_);
            if (file_stream_.is_open()) {
                file_stream_ << getLevelString(level) << " " << tag_ 
                           << ": " << message << "\n" << std::flush;
            }
        }
    }

    void Logger::trace(const std::string& message) const {
        writeLog(LogLevel::TRACE, message);
    }

    void Logger::debug(const std::string& message) const {
        writeLog(LogLevel::DEBUG, message);
    }

    void Logger::info(const std::string& message) const {
        writeLog(LogLevel::INFO, message);
    }

    void Logger::warn(const std::string& message) const {
        writeLog(LogLevel::WARN, message);
    }

    void Logger::error(const std::string& message) const {
        writeLog(LogLevel::ERROR, message);
    }

    void Logger::fatal(const std::string& message) const {
        writeLog(LogLevel::FATAL, message);
    }

    void Logger::flush() {
        if (file_enabled_.load()) {
            std::lock_guard<std::mutex> lock(file_mutex_);
            if (file_stream_.is_open()) {
                file_stream_.flush();
            }
        }
    }

} // namespace AnalysisToolkit
