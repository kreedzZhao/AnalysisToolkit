# AnalysisToolkit

An advanced Android native analysis library providing logging, hooking, and monitoring capabilities for reverse engineering and security research.

## Features

- **High-Performance Logging**: Thread-safe logging system with Android logcat integration
- **Advanced Hook System**: Function hooking using Dobby framework
- **JNI Monitoring**: Comprehensive JNI function call monitoring
- **Memory-Efficient**: Optimized for minimal memory footprint
- **Cross-Platform**: Supports ARM64, ARM32, x86, x86_64 architectures

## Architecture

```
AnalysisToolkit/
├── Logger          # High-performance logging system
├── HookManager      # Function hooking with Dobby
├── Monitor          # JNI and native function monitoring
└── Utils           # Helper utilities and tools
```

## Quick Start

### Basic Integration

```cpp
#include <AnalysisToolkit/AnalysisToolkit.h>

// Initialize the toolkit
AnalysisToolkit::Config config;
config.app_tag = "MyApp";
config.enable_hook_manager = true;
config.enable_jni_monitoring = true;

bool success = AnalysisToolkit::initialize(config);
if (success) {
    ATKIT_INFO("AnalysisToolkit initialized successfully");
}
```

### Logging

```cpp
// Simple logging
ATKIT_INFO("Application started");
ATKIT_DEBUG("Debug information: %d", value);
ATKIT_WARN("Warning message");
ATKIT_ERROR("Error occurred: %s", error_msg);

// Advanced logging
auto* logger = AnalysisToolkit::getLogger();
logger->setMinLevel(AnalysisToolkit::LogLevel::INFO);
logger->enableFile(true);
logger->setLogFile("/data/local/tmp/app.log");
```

### Function Hooking

```cpp
auto* hook_manager = AnalysisToolkit::getHookManager();

// Hook a function by address
ATKIT_HOOK_DEF(int, malloc_hook, size_t size) {
    ATKIT_INFO("malloc called with size: %zu", size);
    return original_malloc_hook(size);
}

hook_manager->hookFunction(
    target_address,
    reinterpret_cast<void*>(hooked_malloc_hook),
    reinterpret_cast<void**>(&original_malloc_hook),
    "malloc_hook"
);

// Hook a library symbol
hook_manager->hookSymbol(
    "libc.so",
    "malloc",
    reinterpret_cast<void*>(hooked_malloc_hook),
    reinterpret_cast<void**>(&original_malloc_hook),
    "libc_malloc"
);
```

### JNI Monitoring

```cpp
// Configure JNI monitoring
AnalysisToolkit::MonitorConfig monitor_config;
monitor_config.enable_jni_monitoring = true;
monitor_config.enable_method_calls = true;
monitor_config.enable_string_operations = true;
monitor_config.log_arguments = true;
monitor_config.log_return_values = true;

auto* monitor = AnalysisToolkit::getMonitor();
monitor->enableJNIMonitoring(env, monitor_config);

// Add filters
auto* jni_monitor = monitor->getJNIMonitor();
jni_monitor->addClassFilter("com.myapp");
jni_monitor->addMethodFilter("onCreate");

// Start monitoring
jni_monitor->startMonitoring();

// Get statistics
size_t call_count = jni_monitor->getCallCount();
ATKIT_INFO("JNI calls intercepted: %zu", call_count);
```

### Advanced Hook Examples

```cpp
// Hook with instrumentation
hook_manager->instrumentFunction(
    target_address,
    [](void* address, void* context) {
        ATKIT_DEBUG("Function %p called", address);
    },
    "function_tracer"
);

// Check hook status
if (hook_manager->isHooked(target_address)) {
    auto* hook_info = hook_manager->getHookInfo(target_address);
    ATKIT_INFO("Hook active: %s", hook_info->symbol_name.c_str());
}

// Remove hook
hook_manager->unhookFunction(target_address);
```

### Monitor Configuration

```cpp
// Detailed monitor configuration
AnalysisToolkit::MonitorConfig config;
config.enable_jni_monitoring = true;
config.enable_method_calls = true;
config.enable_field_access = true;
config.enable_object_creation = true;
config.enable_string_operations = true;
config.enable_array_operations = true;
config.log_arguments = true;
config.log_return_values = true;
config.log_stack_trace = false;

// Add specific class filters
config.filter_classes.insert("com.target.app");
config.filter_classes.insert("java.lang.String");

// Exclude system classes
config.exclude_classes.insert("android.app");
config.exclude_classes.insert("java.util");

// Method filters
config.filter_methods.insert("onCreate");
config.filter_methods.insert("onDestroy");

// Apply configuration
monitor->enableJNIMonitoring(env, config);
```

## Build Configuration

### CMakeLists.txt

```cmake
# Add AnalysisToolkit to your project
add_subdirectory(external/AnalysisToolkit)

# Link with your target
target_link_libraries(your_target
    AnalysisToolkit_static
    dobby  # Required for hook functionality
    log
    dl
)

# Include headers
target_include_directories(your_target PRIVATE
    external/AnalysisToolkit/include
)

# Enable features
target_compile_definitions(your_target PRIVATE
    ENABLE_ANALYSIS_TOOLKIT
    __ANDROID__
)
```

### Gradle Configuration

```gradle
android {
    defaultConfig {
        externalNativeBuild {
            cmake {
                cppFlags "-std=c++17"
                arguments "-DANDROID_STL=c++_shared"
            }
        }
    }
}
```

## Performance Characteristics

- **Logging**: 5-10 microseconds per log entry
- **Hook Setup**: 100-500 microseconds per hook
- **JNI Monitoring**: 1-5 microseconds overhead per JNI call
- **Memory Usage**: <1MB for typical usage
- **Thread Safety**: Full support for multi-threaded applications

## API Reference

### Core Functions

```cpp
namespace AnalysisToolkit {
    bool initialize(const Config& config);
    void cleanup();
    std::string getLibraryInfo();
    bool isInitialized();
    
    Logger* getLogger();
    HookManager* getHookManager();
    Monitor* getMonitor();
}
```

### Hook Manager

```cpp
class HookManager {
public:
    HookStatus hookFunction(void* address, void* replace, void** original, const std::string& tag);
    HookStatus hookSymbol(const std::string& lib, const std::string& symbol, void* replace, void** original, const std::string& tag);
    HookStatus instrumentFunction(void* address, InstrumentCallback callback, const std::string& tag);
    HookStatus unhookFunction(void* address);
    bool isHooked(void* address) const;
    std::vector<HookInfo*> getAllHooks() const;
};
```

### JNI Monitor

```cpp
class JNIMonitor {
public:
    bool initialize(JNIEnv* env, const MonitorConfig& config);
    bool startMonitoring();
    bool stopMonitoring();
    void addClassFilter(const std::string& class_name);
    void addMethodFilter(const std::string& method_name);
    size_t getCallCount() const;
    void resetStatistics();
};
```

## Error Handling

```cpp
// Check initialization
if (!AnalysisToolkit::isInitialized()) {
    ATKIT_ERROR("Toolkit not initialized");
    return;
}

// Check hook status
auto status = hook_manager->hookFunction(address, replace, &original, "tag");
switch (status) {
    case HookStatus::SUCCESS:
        ATKIT_INFO("Hook successful");
        break;
    case HookStatus::ALREADY_HOOKED:
        ATKIT_WARN("Already hooked");
        break;
    case HookStatus::INVALID_ADDRESS:
        ATKIT_ERROR("Invalid address");
        break;
    default:
        ATKIT_ERROR("Hook failed: %d", static_cast<int>(status));
        break;
}
```

## Best Practices

1. **Initialize Early**: Call `initialize()` in JNI_OnLoad
2. **Cleanup Properly**: Call `cleanup()` before app termination
3. **Use Filters**: Add specific filters to reduce monitoring overhead
4. **Monitor Selectively**: Enable only needed monitoring features
5. **Check Status**: Always verify hook and monitor status
6. **Handle Errors**: Implement proper error handling for all operations

## Troubleshooting

### Common Issues

1. **Hook Fails**: Ensure target address is valid and not already hooked
2. **JNI Monitor Silent**: Check if filters are too restrictive
3. **Performance Issues**: Reduce monitoring scope with filters
4. **Memory Leaks**: Ensure proper cleanup on app termination

### Debug Logging

```cpp
// Enable debug logging
auto* logger = AnalysisToolkit::getLogger();
logger->setMinLevel(AnalysisToolkit::LogLevel::DEBUG);

// Check toolkit status
ATKIT_DEBUG("Toolkit initialized: %s", AnalysisToolkit::isInitialized() ? "yes" : "no");
ATKIT_DEBUG("Active hooks: %zu", hook_manager->getAllHooks().size());
ATKIT_DEBUG("JNI monitoring: %s", monitor->isMonitoring(MonitorType::JNI_CALLS) ? "active" : "inactive");
```

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Contributing

1. Fork the repository
2. Create a feature branch
3. Commit your changes
4. Push to the branch
5. Create a Pull Request

## Support

For questions and support, please open an issue on GitHub. 