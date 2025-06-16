#include "AnalysisToolkit/Monitor.h"

#include <dlfcn.h>

#include <atomic>
#include <sstream>

#include "AnalysisToolkit/Hook.h"
#include "AnalysisToolkit/Logger.h"

#ifdef __ANDROID__
#include <android/log.h>
#endif

namespace AnalysisToolkit {

// 静态成员初始化
std::unique_ptr<JNIMonitor> JNIMonitor::instance_ = nullptr;
std::mutex JNIMonitor::instance_mutex_;

std::unique_ptr<Monitor> Monitor::instance_ = nullptr;
std::mutex Monitor::instance_mutex_;

static std::atomic<size_t> g_call_counter{0};

// ===== JNI Hook 函数声明 =====

// JNI 方法调用 Hook 定义
#define JNI_HOOK_DEF(ret, func, ...)               \
    ret (*original_##func)(__VA_ARGS__) = nullptr; \
    ret hooked_##func(__VA_ARGS__)

// 工具宏
#define GET_JNI_MONITOR()                                                            \
    JNIMonitor* monitor = JNIMonitor::getInstance();                                 \
    if (!monitor || !monitor->isMonitoring()) {                                      \
        return original_##func ? original_##func(env, obj, method_id, args) : ret{}; \
    }

#define LOG_JNI_CALL(method_name) \
    g_call_counter++;             \
    ATKIT_DEBUG("JNI Call: %s", method_name);

// ===== JNI Method Call Hooks =====

JNI_HOOK_DEF(jobject,
             CallObjectMethodV,
             JNIEnv* env,
             jobject obj,
             jmethodID method_id,
             va_list args) {
    GET_JNI_MONITOR()
    LOG_JNI_CALL("CallObjectMethodV")

    // 执行原函数
    jobject result = original_CallObjectMethodV(env, obj, method_id, args);

    // 记录调用信息
    JNICallInfo call_info;
    call_info.method_name = "CallObjectMethodV";
    call_info.is_static = false;
    call_info.method_id = method_id;
    call_info.object_ref = obj;

    if (monitor->shouldMonitorClass(monitor->getJObjectClassName(env, obj))) {
        monitor->logJNICall(call_info, "jobject");
    }

    return result;
}

JNI_HOOK_DEF(void, CallVoidMethodV, JNIEnv* env, jobject obj, jmethodID method_id, va_list args) {
    GET_JNI_MONITOR()
    LOG_JNI_CALL("CallVoidMethodV")

    // 执行原函数
    original_CallVoidMethodV(env, obj, method_id, args);

    // 记录调用信息
    JNICallInfo call_info;
    call_info.method_name = "CallVoidMethodV";
    call_info.is_static = false;
    call_info.method_id = method_id;
    call_info.object_ref = obj;

    if (monitor->shouldMonitorClass(monitor->getJObjectClassName(env, obj))) {
        monitor->logJNICall(call_info, "void");
    }
}

JNI_HOOK_DEF(jint, CallIntMethodV, JNIEnv* env, jobject obj, jmethodID method_id, va_list args) {
    GET_JNI_MONITOR()
    LOG_JNI_CALL("CallIntMethodV")

    // 执行原函数
    jint result = original_CallIntMethodV(env, obj, method_id, args);

    // 记录调用信息
    JNICallInfo call_info;
    call_info.method_name = "CallIntMethodV";
    call_info.is_static = false;
    call_info.method_id = method_id;
    call_info.object_ref = obj;

    if (monitor->shouldMonitorClass(monitor->getJObjectClassName(env, obj))) {
        monitor->logJNICall(call_info, std::to_string(result));
    }

    return result;
}

JNI_HOOK_DEF(jstring, NewStringUTF, JNIEnv* env, const char* utf) {
    GET_JNI_MONITOR()
    LOG_JNI_CALL("NewStringUTF")

    // 执行原函数
    jstring result = original_NewStringUTF(env, utf);

    if (monitor->getConfig().enable_string_operations) {
        ATKIT_DEBUG("NewStringUTF: creating string: %s", utf ? utf : "null");
    }

    return result;
}

JNI_HOOK_DEF(const char*, GetStringUTFChars, JNIEnv* env, jstring str, jboolean* isCopy) {
    GET_JNI_MONITOR()
    LOG_JNI_CALL("GetStringUTFChars")

    // 执行原函数
    const char* result = original_GetStringUTFChars(env, str, isCopy);

    if (monitor->getConfig().enable_string_operations && result) {
        ATKIT_DEBUG("GetStringUTFChars: extracted string: %s", result);
    }

    return result;
}

JNI_HOOK_DEF(jclass, FindClass, JNIEnv* env, const char* name) {
    GET_JNI_MONITOR()
    LOG_JNI_CALL("FindClass")

    // 执行原函数
    jclass result = original_FindClass(env, name);

    if (monitor->shouldMonitorClass(name ? name : "unknown")) {
        ATKIT_INFO("FindClass: loading class: %s", name ? name : "null");
    }

    return result;
}

// ===== JNIMonitor 实现 =====

JNIMonitor* JNIMonitor::getInstance() {
    std::lock_guard<std::mutex> lock(instance_mutex_);
    if (instance_ == nullptr) {
        instance_ = std::unique_ptr<JNIMonitor>(new JNIMonitor());
    }
    return instance_.get();
}

JNIMonitor::~JNIMonitor() {
    cleanup();
}

bool JNIMonitor::initialize(JNIEnv* env, const MonitorConfig& config) {
    std::lock_guard<std::mutex> lock(config_mutex_);

    if (is_initialized_) {
        ATKIT_WARN("JNIMonitor already initialized");
        return true;
    }

    jni_env_ = env;
    config_ = config;

    if (config.enable_jni_monitoring) {
        if (!initializeJNIHooks()) {
            ATKIT_ERROR("Failed to initialize JNI hooks");
            return false;
        }
    }

    is_initialized_ = true;
    ATKIT_INFO("JNIMonitor initialized successfully");
    return true;
}

bool JNIMonitor::initializeJNIHooks() {
    HookManager* hook_manager = HookManager::getInstance();
    bool all_success = true;

    // Hook JNI 方法调用函数
    if (config_.enable_method_calls) {
        auto status1 = hook_manager->hookFunction(
            reinterpret_cast<void*>(jni_env_->functions->CallObjectMethodV),
            reinterpret_cast<void*>(hooked_CallObjectMethodV),
            reinterpret_cast<void**>(&original_CallObjectMethodV),
            "JNI_CallObjectMethodV");

        auto status2 = hook_manager->hookFunction(
            reinterpret_cast<void*>(jni_env_->functions->CallVoidMethodV),
            reinterpret_cast<void*>(hooked_CallVoidMethodV),
            reinterpret_cast<void**>(&original_CallVoidMethodV),
            "JNI_CallVoidMethodV");

        auto status3 =
            hook_manager->hookFunction(reinterpret_cast<void*>(jni_env_->functions->CallIntMethodV),
                                       reinterpret_cast<void*>(hooked_CallIntMethodV),
                                       reinterpret_cast<void**>(&original_CallIntMethodV),
                                       "JNI_CallIntMethodV");

        all_success = all_success && (status1 == HookStatus::SUCCESS) &&
                      (status2 == HookStatus::SUCCESS) && (status3 == HookStatus::SUCCESS);
    }

    // Hook 字符串操作
    if (config_.enable_string_operations) {
        auto status1 =
            hook_manager->hookFunction(reinterpret_cast<void*>(jni_env_->functions->NewStringUTF),
                                       reinterpret_cast<void*>(hooked_NewStringUTF),
                                       reinterpret_cast<void**>(&original_NewStringUTF),
                                       "JNI_NewStringUTF");

        auto status2 = hook_manager->hookFunction(
            reinterpret_cast<void*>(jni_env_->functions->GetStringUTFChars),
            reinterpret_cast<void*>(hooked_GetStringUTFChars),
            reinterpret_cast<void**>(&original_GetStringUTFChars),
            "JNI_GetStringUTFChars");

        all_success =
            all_success && (status1 == HookStatus::SUCCESS) && (status2 == HookStatus::SUCCESS);
    }

    // Hook 类查找
    auto status =
        hook_manager->hookFunction(reinterpret_cast<void*>(jni_env_->functions->FindClass),
                                   reinterpret_cast<void*>(hooked_FindClass),
                                   reinterpret_cast<void**>(&original_FindClass),
                                   "JNI_FindClass");
    all_success = all_success && (status == HookStatus::SUCCESS);

    if (all_success) {
        ATKIT_INFO("All JNI hooks installed successfully");
    } else {
        ATKIT_ERROR("Some JNI hooks failed to install");
    }

    return all_success;
}

void JNIMonitor::cleanupJNIHooks() {
    HookManager* hook_manager = HookManager::getInstance();

    // 移除所有 JNI Hook
    hook_manager->unhookFunction(reinterpret_cast<void*>(jni_env_->functions->CallObjectMethodV));
    hook_manager->unhookFunction(reinterpret_cast<void*>(jni_env_->functions->CallVoidMethodV));
    hook_manager->unhookFunction(reinterpret_cast<void*>(jni_env_->functions->CallIntMethodV));
    hook_manager->unhookFunction(reinterpret_cast<void*>(jni_env_->functions->NewStringUTF));
    hook_manager->unhookFunction(reinterpret_cast<void*>(jni_env_->functions->GetStringUTFChars));
    hook_manager->unhookFunction(reinterpret_cast<void*>(jni_env_->functions->FindClass));

    ATKIT_INFO("JNI hooks cleaned up");
}

std::string JNIMonitor::getJObjectClassName(JNIEnv* env, jobject obj) {
    if (!env || !obj)
        return "null";

    jclass cls = env->GetObjectClass(obj);
    if (!cls)
        return "unknown";

    jclass class_cls = env->FindClass("java/lang/Class");
    jmethodID get_name = env->GetMethodID(class_cls, "getName", "()Ljava/lang/String;");

    if (!get_name)
        return "unknown";

    jstring name_str = static_cast<jstring>(env->CallObjectMethod(cls, get_name));
    if (!name_str)
        return "unknown";

    const char* name_chars = env->GetStringUTFChars(name_str, nullptr);
    if (!name_chars)
        return "unknown";

    std::string class_name(name_chars);
    env->ReleaseStringUTFChars(name_str, name_chars);
    env->DeleteLocalRef(name_str);
    env->DeleteLocalRef(cls);
    env->DeleteLocalRef(class_cls);

    return class_name;
}

std::string JNIMonitor::getJObjectString(JNIEnv* env, jobject obj) {
    if (!env || !obj)
        return "null";

    if (auxiliary_class_ && to_string_method_) {
        jstring result = static_cast<jstring>(
            env->CallStaticObjectMethod(auxiliary_class_, to_string_method_, obj));
        if (result) {
            const char* chars = env->GetStringUTFChars(result, nullptr);
            std::string str(chars ? chars : "null");
            env->ReleaseStringUTFChars(result, chars);
            env->DeleteLocalRef(result);
            return str;
        }
    }

    // 回退到默认 toString
    jclass cls = env->GetObjectClass(obj);
    jmethodID toString = env->GetMethodID(cls, "toString", "()Ljava/lang/String;");
    if (toString) {
        jstring result = static_cast<jstring>(env->CallObjectMethod(obj, toString));
        if (result) {
            const char* chars = env->GetStringUTFChars(result, nullptr);
            std::string str(chars ? chars : "null");
            env->ReleaseStringUTFChars(result, chars);
            env->DeleteLocalRef(result);
            env->DeleteLocalRef(cls);
            return str;
        }
    }
    env->DeleteLocalRef(cls);

    return "unknown";
}

bool JNIMonitor::shouldMonitorClass(const std::string& class_name) {
    if (class_name.empty())
        return false;

    // 检查排除列表
    if (!config_.exclude_classes.empty()) {
        for (const auto& exclude : config_.exclude_classes) {
            if (class_name.find(exclude) != std::string::npos) {
                return false;
            }
        }
    }

    // 检查过滤列表
    if (!config_.filter_classes.empty()) {
        for (const auto& filter : config_.filter_classes) {
            if (class_name.find(filter) != std::string::npos) {
                return true;
            }
        }
        return false;  // 有过滤器但不匹配
    }

    return true;  // 无过滤器则全部监控
}

bool JNIMonitor::shouldMonitorMethod(const std::string& method_name) {
    if (method_name.empty())
        return false;

    // 检查排除列表
    if (!config_.exclude_methods.empty()) {
        for (const auto& exclude : config_.exclude_methods) {
            if (method_name.find(exclude) != std::string::npos) {
                return false;
            }
        }
    }

    // 检查过滤列表
    if (!config_.filter_methods.empty()) {
        for (const auto& filter : config_.filter_methods) {
            if (method_name.find(filter) != std::string::npos) {
                return true;
            }
        }
        return false;
    }

    return true;
}

void JNIMonitor::logJNICall(const JNICallInfo& call_info, const std::string& result) {
    std::stringstream ss;
    ss << "JNI Call: " << call_info.method_name;
    ss << " | Class: " << call_info.class_name;
    ss << " | Static: " << (call_info.is_static ? "yes" : "no");
    if (!result.empty()) {
        ss << " | Result: " << result;
    }

    ATKIT_INFO("%s", ss.str().c_str());
}

void JNIMonitor::updateConfig(const MonitorConfig& config) {
    std::lock_guard<std::mutex> lock(config_mutex_);
    config_ = config;
    ATKIT_DEBUG("JNIMonitor config updated");
}

MonitorConfig JNIMonitor::getConfig() const {
    std::lock_guard<std::mutex> lock(config_mutex_);
    return config_;
}

bool JNIMonitor::startMonitoring() {
    std::lock_guard<std::mutex> lock(config_mutex_);
    config_.enable_jni_monitoring = true;
    ATKIT_INFO("JNI monitoring started");
    return true;
}

bool JNIMonitor::stopMonitoring() {
    std::lock_guard<std::mutex> lock(config_mutex_);
    config_.enable_jni_monitoring = false;
    ATKIT_INFO("JNI monitoring stopped");
    return true;
}

bool JNIMonitor::isMonitoring() const {
    std::lock_guard<std::mutex> lock(config_mutex_);
    return config_.enable_jni_monitoring;
}

void JNIMonitor::addClassFilter(const std::string& class_name) {
    std::lock_guard<std::mutex> lock(config_mutex_);
    config_.filter_classes.insert(class_name);
    ATKIT_DEBUG("Added class filter: %s", class_name.c_str());
}

void JNIMonitor::addMethodFilter(const std::string& method_name) {
    std::lock_guard<std::mutex> lock(config_mutex_);
    config_.filter_methods.insert(method_name);
    ATKIT_DEBUG("Added method filter: %s", method_name.c_str());
}

size_t JNIMonitor::getCallCount() const {
    return g_call_counter.load();
}

void JNIMonitor::resetStatistics() {
    g_call_counter.store(0);
    ATKIT_DEBUG("JNI call statistics reset");
}

void JNIMonitor::setAuxiliaryClass(jclass clazz) {
    auxiliary_class_ = clazz;
    if (jni_env_ && clazz) {
        to_string_method_ = jni_env_->GetStaticMethodID(clazz,
                                                        "toString",
                                                        "(Ljava/lang/Object;)Ljava/lang/String;");
    }
    ATKIT_DEBUG("Auxiliary class set for object string conversion");
}

void JNIMonitor::cleanup() {
    if (is_initialized_) {
        cleanupJNIHooks();
        is_initialized_ = false;
        ATKIT_INFO("JNIMonitor cleanup completed");
    }
}

// ===== Monitor 主管理器实现 =====

Monitor* Monitor::getInstance() {
    std::lock_guard<std::mutex> lock(instance_mutex_);
    if (instance_ == nullptr) {
        instance_ = std::unique_ptr<Monitor>(new Monitor());
    }
    return instance_.get();
}

Monitor::~Monitor() {
    cleanup();
}

bool Monitor::initialize() {
    if (is_initialized_) {
        return true;
    }

    // 初始化 Hook 管理器
    HookManager::getInstance()->initialize();

    is_initialized_ = true;
    ATKIT_INFO("Monitor initialized successfully");
    return true;
}

void Monitor::cleanup() {
    if (is_initialized_) {
        if (jni_monitor_) {
            jni_monitor_->cleanup();
            jni_monitor_.reset();
        }

        HookManager::getInstance()->cleanup();
        is_initialized_ = false;
        ATKIT_INFO("Monitor cleanup completed");
    }
}

JNIMonitor* Monitor::getJNIMonitor() {
    std::lock_guard<std::mutex> lock(monitors_mutex_);
    if (!jni_monitor_) {
        jni_monitor_ = std::make_unique<JNIMonitor>();
    }
    return jni_monitor_.get();
}

bool Monitor::enableJNIMonitoring(JNIEnv* env, const MonitorConfig& config) {
    if (!is_initialized_) {
        ATKIT_ERROR("Monitor not initialized");
        return false;
    }

    JNIMonitor* jni_monitor = getJNIMonitor();
    return jni_monitor->initialize(env, config);
}

bool Monitor::disableJNIMonitoring() {
    std::lock_guard<std::mutex> lock(monitors_mutex_);
    if (jni_monitor_) {
        jni_monitor_->cleanup();
        return true;
    }
    return false;
}

bool Monitor::startMonitoring(MonitorType type) {
    switch (type) {
        case MonitorType::JNI_CALLS:
            return getJNIMonitor()->startMonitoring();
        default:
            ATKIT_WARN("Unsupported monitor type: %d", static_cast<int>(type));
            return false;
    }
}

bool Monitor::stopMonitoring(MonitorType type) {
    switch (type) {
        case MonitorType::JNI_CALLS:
            return getJNIMonitor()->stopMonitoring();
        default:
            ATKIT_WARN("Unsupported monitor type: %d", static_cast<int>(type));
            return false;
    }
}

bool Monitor::isMonitoring(MonitorType type) const {
    switch (type) {
        case MonitorType::JNI_CALLS:
            return jni_monitor_ ? jni_monitor_->isMonitoring() : false;
        default:
            return false;
    }
}

// ===== MonitorUtils 实现 =====

namespace MonitorUtils {

std::string jvalueToString(JNIEnv* env, jvalue value, char type) {
    switch (type) {
        case 'Z':
            return value.z ? "true" : "false";
        case 'B':
            return std::to_string(value.b);
        case 'C':
            return std::string(1, static_cast<char>(value.c));
        case 'S':
            return std::to_string(value.s);
        case 'I':
            return std::to_string(value.i);
        case 'J':
            return std::to_string(value.j);
        case 'F':
            return std::to_string(value.f);
        case 'D':
            return std::to_string(value.d);
        case 'L': {
            if (value.l && env) {
                return JNIMonitor::getInstance()->getJObjectString(env, value.l);
            }
            return "null";
        }
        default:
            return "unknown";
    }
}

bool isSystemClass(const std::string& class_name) {
    return class_name.find("java.") == 0 || class_name.find("android.") == 0 ||
           class_name.find("javax.") == 0;
}

}  // namespace MonitorUtils

}  // namespace AnalysisToolkit
