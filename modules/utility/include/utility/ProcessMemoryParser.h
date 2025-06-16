/**
 * @file ProcessMemoryParser.h
 * @brief Modern C++ Process Memory Map Parser
 * @author AnalysisToolkit
 * @date 2024
 * 
 * A modern C++ implementation for parsing process memory maps.
 * Supports both Linux (/proc/[pid]/maps) and macOS (vm_map).
 */

#ifndef ANALYSIS_TOOLKIT_PROCESS_MEMORY_PARSER_H
#define ANALYSIS_TOOLKIT_PROCESS_MEMORY_PARSER_H

#include <memory>
#include <string>
#include <vector>
#include <optional>
#include <functional>

namespace AnalysisToolkit {

/**
 * @brief Represents memory permissions
 */
struct MemoryPermissions {
    bool readable = false;
    bool writable = false;
    bool executable = false;
    bool private_mapping = false;
    
    /**
     * @brief Convert permissions to string format (e.g., "rwxp")
     */
    std::string toString() const;
    
    /**
     * @brief Parse permissions from string format
     */
    static MemoryPermissions fromString(const std::string& perm_str);
};

/**
 * @brief Represents a memory mapping region
 */
class MemoryRegion {
public:
    MemoryRegion() = default;
    MemoryRegion(uintptr_t start, uintptr_t end, const MemoryPermissions& perms,
                 uint64_t offset, const std::string& device, uint32_t inode,
                 const std::string& pathname, const std::string& original_line = "");
    
    // Getters
    uintptr_t getStartAddress() const { return start_address_; }
    uintptr_t getEndAddress() const { return end_address_; }
    size_t getSize() const { return end_address_ - start_address_; }
    const MemoryPermissions& getPermissions() const { return permissions_; }
    uint64_t getOffset() const { return offset_; }
    const std::string& getDevice() const { return device_; }
    uint32_t getInode() const { return inode_; }
    const std::string& getPathname() const { return pathname_; }
    const std::string& getOriginalLine() const { return original_line_; }
    
    // Utility methods
    bool contains(uintptr_t address) const;
    bool isAnonymous() const { return pathname_.empty() || pathname_ == "[anon]"; }
    bool isStack() const { return pathname_ == "[stack]"; }
    bool isHeap() const { return pathname_ == "[heap]"; }
    bool isVdso() const { return pathname_ == "[vdso]"; }
    
    /**
     * @brief Get a human-readable description of this region
     */
    std::string toString() const;

private:
    uintptr_t start_address_ = 0;
    uintptr_t end_address_ = 0;
    MemoryPermissions permissions_;
    uint64_t offset_ = 0;
    std::string device_;
    uint32_t inode_ = 0;
    std::string pathname_;
    std::string original_line_;
};

/**
 * @brief Process Memory Parser - Main class for parsing process memory maps
 */
class ProcessMemoryParser {
public:
    /**
     * @brief Error codes for memory parsing operations
     */
    enum class ErrorCode {
        SUCCESS = 0,
        PROCESS_NOT_FOUND,
        PERMISSION_DENIED,
        FILE_NOT_FOUND,
        PARSE_ERROR,
        PLATFORM_NOT_SUPPORTED,
        UNKNOWN_ERROR
    };

    /**
     * @brief Result wrapper for operations that might fail
     */
    template<typename T>
    class Result {
    public:
        Result(T&& value) : value_(std::forward<T>(value)), error_(ErrorCode::SUCCESS) {}
        Result(ErrorCode error, const std::string& message = "") 
            : error_(error), error_message_(message) {}
        
        bool isSuccess() const { return error_ == ErrorCode::SUCCESS; }
        bool hasError() const { return error_ != ErrorCode::SUCCESS; }
        ErrorCode getError() const { return error_; }
        const std::string& getErrorMessage() const { return error_message_; }
        
        const T& getValue() const { 
            if (hasError()) {
                throw std::runtime_error("Attempting to get value from failed result: " + error_message_);
            }
            return value_; 
        }
        
        T& getValue() { 
            if (hasError()) {
                throw std::runtime_error("Attempting to get value from failed result: " + error_message_);
            }
            return value_; 
        }
        
    private:
        T value_;
        ErrorCode error_;
        std::string error_message_;
    };

    /**
     * @brief Constructor
     */
    ProcessMemoryParser() = default;
    
    /**
     * @brief Destructor
     */
    ~ProcessMemoryParser() = default;
    
    // Non-copyable but movable
    ProcessMemoryParser(const ProcessMemoryParser&) = delete;
    ProcessMemoryParser& operator=(const ProcessMemoryParser&) = delete;
    ProcessMemoryParser(ProcessMemoryParser&&) = default;
    ProcessMemoryParser& operator=(ProcessMemoryParser&&) = default;
    
    /**
     * @brief Parse memory maps for a specific process
     * @param pid Process ID (use -1 or 0 for current process)
     * @return Result containing vector of memory regions or error
     */
    Result<std::vector<MemoryRegion>> parseProcess(int pid = -1);
    
    /**
     * @brief Parse memory maps for current process
     * @return Result containing vector of memory regions or error
     */
    Result<std::vector<MemoryRegion>> parseSelf() { return parseProcess(-1); }
    
    /**
     * @brief Find memory regions that contain the specified address
     * @param address The address to search for
     * @param pid Process ID (use -1 for current process)
     * @return Result containing vector of matching regions
     */
    Result<std::vector<MemoryRegion>> findRegionsContaining(uintptr_t address, int pid = -1);
    
    /**
     * @brief Find memory regions with specific pathname
     * @param pathname The pathname to search for (supports partial matching)
     * @param pid Process ID (use -1 for current process)
     * @param exact_match Whether to perform exact string matching
     * @return Result containing vector of matching regions
     */
    Result<std::vector<MemoryRegion>> findRegionsByPath(const std::string& pathname, 
                                                        int pid = -1, 
                                                        bool exact_match = false);
    
    /**
     * @brief Find memory regions with specific permissions
     * @param permissions The permissions to match
     * @param pid Process ID (use -1 for current process)
     * @return Result containing vector of matching regions
     */
    Result<std::vector<MemoryRegion>> findRegionsByPermissions(const MemoryPermissions& permissions, 
                                                               int pid = -1);
    
    /**
     * @brief Print memory map information
     * @param regions Vector of memory regions to print
     * @param limit Maximum number of regions to print (-1 for all)
     */
    static void printMemoryMap(const std::vector<MemoryRegion>& regions, int limit = -1);
    
    /**
     * @brief Get platform-specific implementation status
     */
    static bool isPlatformSupported();
    
    /**
     * @brief Get error message for an error code
     */
    static std::string getErrorString(ErrorCode error);
    
    /**
     * @brief Set custom filter function for regions
     * @param filter Function that returns true for regions to include
     */
    void setRegionFilter(std::function<bool(const MemoryRegion&)> filter) {
        region_filter_ = std::move(filter);
    }
    
    /**
     * @brief Clear region filter
     */
    void clearRegionFilter() { region_filter_ = nullptr; }

private:
    /**
     * @brief Parse Linux-style /proc/[pid]/maps file
     */
    Result<std::vector<MemoryRegion>> parseLinuxMaps(int pid);
    
    /**
     * @brief Parse macOS memory maps using vm_map
     */
    Result<std::vector<MemoryRegion>> parseMacOSMaps(int pid);
    
    /**
     * @brief Parse a single line from /proc/[pid]/maps
     */
    std::optional<MemoryRegion> parseMapsLine(const std::string& line);
    
    /**
     * @brief Get maps file path for process
     */
    std::string getMapsFilePath(int pid);
    
    /**
     * @brief Apply region filter if set
     */
    bool shouldIncludeRegion(const MemoryRegion& region) const;
    
    // Optional filter function
    std::function<bool(const MemoryRegion&)> region_filter_;
};

} // namespace AnalysisToolkit

#endif // ANALYSIS_TOOLKIT_PROCESS_MEMORY_PARSER_H 