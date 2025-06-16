#ifndef ANALYSIS_TOOLKIT_MONITOR_H
#define ANALYSIS_TOOLKIT_MONITOR_H

#include <functional>
#include <memory>
#include <set>
#include <string>

#include "Hook.h"
#include "Logger.h"

#ifdef __ANDROID__
#include <jni.h>
#endif

namespace AnalysisToolkit {

// 监控类型枚举
enum class MonitorType {
    JNI_CALLS,         // JNI 调用监控
    NATIVE_FUNCTIONS,  // Native 函数监控
    LIBRARY_LOADING,   // 库加载监控
    MEMORY_ACCESS      // 内存访问监控
};

// 监控事件回调
using MonitorCallback = std::function<void(const std::string& event, const std::string& details)>;

// JNI 调用信息
struct JNICallInfo {
    std::string method_name;
    std::string class_name;
    std::string signature;
    std::string return_type;
    std::vector<std::string> arguments;
    bool is_static;
    void* method_id;
    void* object_ref;
};

// 监控配置
struct MonitorConfig {
    bool enable_jni_monitoring = false;
    bool enable_method_calls = true;
    bool enable_field_access = true;
    bool enable_object_creation = true;
    bool enable_string_operations = true;
    bool enable_array_operations = true;
    bool log_arguments = true;
    bool log_return_values = true;
    bool log_stack_trace = false;
    std::set<std::string> filter_classes;   // 要监控的类名过滤器
    std::set<std::string> exclude_classes;  // 排除的类名
    std::set<std::string> filter_methods;   // 要监控的方法名过滤器
    std::set<std::string> exclude_methods;  // 排除的方法名
};

class JNIMonitor {
  private:
    static std::unique_ptr<JNIMonitor> instance_;
    static std::mutex instance_mutex_;

    mutable std::mutex config_mutex_;
    MonitorConfig config_;
    bool is_initialized_;

    // JNI 环境和缓存
    JNIEnv* jni_env_;
    jclass auxiliary_class_;
    jmethodID to_string_method_;

    // 辅助方法
    std::string getJObjectClassName(JNIEnv* env, jobject obj);
    std::string getJObjectString(JNIEnv* env, jobject obj);
    std::string getMethodSignature(JNIEnv* env, jmethodID method_id);
    std::string formatArguments(JNIEnv* env, jmethodID method_id, va_list args, bool is_static);
    bool shouldMonitorClass(const std::string& class_name);
    bool shouldMonitorMethod(const std::string& method_name);
    void logJNICall(const JNICallInfo& call_info, const std::string& result = "");

    // 初始化 JNI Hook
    bool initializeJNIHooks();
    void cleanupJNIHooks();

  public:
    static JNIMonitor* getInstance();
    ~JNIMonitor();

    // 初始化和配置
    bool initialize(JNIEnv* env, const MonitorConfig& config = MonitorConfig{});
    void updateConfig(const MonitorConfig& config);
    MonitorConfig getConfig() const;

    // 监控控制
    bool startMonitoring();
    bool stopMonitoring();
    bool isMonitoring() const;

    // JNI 监控
    bool enableJNIMonitoring();
    bool disableJNIMonitoring();

    // 添加/移除过滤器
    void addClassFilter(const std::string& class_name);
    void removeClassFilter(const std::string& class_name);
    void addMethodFilter(const std::string& method_name);
    void removeMethodFilter(const std::string& method_name);

    // 统计信息
    size_t getCallCount() const;
    void resetStatistics();

    // 工具方法
    void setAuxiliaryClass(jclass clazz);
    void cleanup();
};

// Monitor 主管理器
class Monitor {
  private:
    static std::unique_ptr<Monitor> instance_;
    static std::mutex instance_mutex_;

    mutable std::mutex monitors_mutex_;
    std::unique_ptr<JNIMonitor> jni_monitor_;
    bool is_initialized_;

  public:
    static Monitor* getInstance();
    ~Monitor();

    // 初始化
    bool initialize();
    void cleanup();

    // JNI 监控接口
    JNIMonitor* getJNIMonitor();
    bool enableJNIMonitoring(JNIEnv* env, const MonitorConfig& config = MonitorConfig{});
    bool disableJNIMonitoring();

    // 通用监控接口
    bool startMonitoring(MonitorType type);
    bool stopMonitoring(MonitorType type);
    bool isMonitoring(MonitorType type) const;

    // 状态查询
    bool isInitialized() const {
        return is_initialized_;
    }
};

// 便利函数
namespace MonitorUtils {
std::string jvalueToString(JNIEnv* env, jvalue value, char type);
std::string getJNISignature(JNIEnv* env, jmethodID method_id);
std::string getCallStack(JNIEnv* env, int max_depth = 10);
bool isSystemClass(const std::string& class_name);
}  // namespace MonitorUtils

// Monitor 辅助宏
#define ATKIT_JNI_MONITOR_INIT(env, config) Monitor::getInstance()->enableJNIMonitoring(env, config)

#define ATKIT_JNI_MONITOR_START() Monitor::getInstance()->getJNIMonitor()->startMonitoring()

#define ATKIT_JNI_MONITOR_STOP() Monitor::getInstance()->getJNIMonitor()->stopMonitoring()

#define ATKIT_MONITOR_ADD_CLASS_FILTER(class_name) \
    Monitor::getInstance()->getJNIMonitor()->addClassFilter(class_name)

#define ATKIT_MONITOR_ADD_METHOD_FILTER(method_name) \
    Monitor::getInstance()->getJNIMonitor()->addMethodFilter(method_name)

}  // namespace AnalysisToolkit

#endif  // ANALYSIS_TOOLKIT_MONITOR_H
