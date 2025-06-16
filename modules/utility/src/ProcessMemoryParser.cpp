/**
 * @file ProcessMemoryParser.cpp
 * @brief Implementation of Modern C++ Process Memory Map Parser
 */

#include "utility/ProcessMemoryParser.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cstring>
#include <cerrno>

// Platform-specific includes
#ifdef __linux__
    #include <unistd.h>
    #include <sys/types.h>
    #include <linux/limits.h>
#elif __APPLE__
    #include <unistd.h>
    #include <sys/types.h>
    #include <mach/mach.h>
    #include <mach/vm_map.h>
    #include <mach/vm_region.h>
    #include <libproc.h>
    #include <sys/proc_info.h>
#endif

namespace AnalysisToolkit {

// ============================================================================
// MemoryPermissions Implementation
// ============================================================================

std::string MemoryPermissions::toString() const {
    std::string result;
    result += readable ? 'r' : '-';
    result += writable ? 'w' : '-';
    result += executable ? 'x' : '-';
    result += private_mapping ? 'p' : 's';
    return result;
}

MemoryPermissions MemoryPermissions::fromString(const std::string& perm_str) {
    MemoryPermissions perms;
    if (perm_str.size() >= 4) {
        perms.readable = (perm_str[0] == 'r');
        perms.writable = (perm_str[1] == 'w');
        perms.executable = (perm_str[2] == 'x');
        perms.private_mapping = (perm_str[3] == 'p');
    }
    return perms;
}

// ============================================================================
// MemoryRegion Implementation
// ============================================================================

MemoryRegion::MemoryRegion(uintptr_t start, uintptr_t end, const MemoryPermissions& perms,
                          uint64_t offset, const std::string& device, uint32_t inode,
                          const std::string& pathname, const std::string& original_line)
    : start_address_(start), end_address_(end), permissions_(perms), offset_(offset),
      device_(device), inode_(inode), pathname_(pathname), original_line_(original_line) {
}

bool MemoryRegion::contains(uintptr_t address) const {
    return address >= start_address_ && address < end_address_;
}

std::string MemoryRegion::toString() const {
    std::ostringstream oss;
    oss << "0x" << std::hex << start_address_ << "-0x" << end_address_
        << " " << permissions_.toString()
        << " 0x" << std::setfill('0') << std::setw(8) << offset_
        << " " << device_
        << " " << std::dec << inode_
        << " " << (pathname_.empty() ? "[anonymous]" : pathname_);
    return oss.str();
}

// ============================================================================
// ProcessMemoryParser Implementation
// ============================================================================

ProcessMemoryParser::Result<std::vector<MemoryRegion>> ProcessMemoryParser::parseProcess(int pid) {
#ifdef __linux__
    return parseLinuxMaps(pid);
#elif __APPLE__
    return parseMacOSMaps(pid);
#else
    return Result<std::vector<MemoryRegion>>(ErrorCode::PLATFORM_NOT_SUPPORTED, 
                                           "Platform not supported");
#endif
}

ProcessMemoryParser::Result<std::vector<MemoryRegion>> 
ProcessMemoryParser::findRegionsContaining(uintptr_t address, int pid) {
    auto parse_result = parseProcess(pid);
    if (parse_result.hasError()) {
        return parse_result;
    }
    
    std::vector<MemoryRegion> matching_regions;
    for (const auto& region : parse_result.getValue()) {
        if (region.contains(address)) {
            matching_regions.push_back(region);
        }
    }
    
    return Result<std::vector<MemoryRegion>>(std::move(matching_regions));
}

ProcessMemoryParser::Result<std::vector<MemoryRegion>> 
ProcessMemoryParser::findRegionsByPath(const std::string& pathname, int pid, bool exact_match) {
    auto parse_result = parseProcess(pid);
    if (parse_result.hasError()) {
        return parse_result;
    }
    
    std::vector<MemoryRegion> matching_regions;
    for (const auto& region : parse_result.getValue()) {
        bool matches = exact_match ? 
            (region.getPathname() == pathname) :
            (region.getPathname().find(pathname) != std::string::npos);
            
        if (matches) {
            matching_regions.push_back(region);
        }
    }
    
    return Result<std::vector<MemoryRegion>>(std::move(matching_regions));
}

ProcessMemoryParser::Result<std::vector<MemoryRegion>> 
ProcessMemoryParser::findRegionsByPermissions(const MemoryPermissions& permissions, int pid) {
    auto parse_result = parseProcess(pid);
    if (parse_result.hasError()) {
        return parse_result;
    }
    
    std::vector<MemoryRegion> matching_regions;
    for (const auto& region : parse_result.getValue()) {
        const auto& region_perms = region.getPermissions();
        bool matches = true;
        
        if (permissions.readable && !region_perms.readable) matches = false;
        if (permissions.writable && !region_perms.writable) matches = false;
        if (permissions.executable && !region_perms.executable) matches = false;
        if (permissions.private_mapping && !region_perms.private_mapping) matches = false;
        
        if (matches) {
            matching_regions.push_back(region);
        }
    }
    
    return Result<std::vector<MemoryRegion>>(std::move(matching_regions));
}

void ProcessMemoryParser::printMemoryMap(const std::vector<MemoryRegion>& regions, int limit) {
    std::cout << std::left << std::setw(20) << "Address Range"
              << std::setw(8) << "Perms"
              << std::setw(12) << "Offset"
              << std::setw(12) << "Device"
              << std::setw(8) << "Inode"
              << std::setw(12) << "Size"
              << "Pathname" << std::endl;
    std::cout << std::string(80, '-') << std::endl;
    
    int count = 0;
    for (const auto& region : regions) {
        if (limit > 0 && count >= limit) break;
        
        std::cout << std::hex << "0x" << std::setfill('0') << std::setw(8) << region.getStartAddress()
                  << "-0x" << std::setw(8) << region.getEndAddress()
                  << " " << std::setfill(' ') << std::setw(4) << region.getPermissions().toString()
                  << " " << std::hex << "0x" << std::setfill('0') << std::setw(8) << region.getOffset()
                  << " " << std::setfill(' ') << std::setw(8) << region.getDevice()
                  << " " << std::dec << std::setw(6) << region.getInode()
                  << " " << std::setw(8) << region.getSize()
                  << " " << (region.getPathname().empty() ? "[anonymous]" : region.getPathname())
                  << std::endl;
        count++;
    }
    
    std::cout << std::endl << "Total regions: " << regions.size() << std::endl;
}

bool ProcessMemoryParser::isPlatformSupported() {
#if defined(__linux__) || defined(__APPLE__)
    return true;
#else
    return false;
#endif
}

std::string ProcessMemoryParser::getErrorString(ErrorCode error) {
    switch (error) {
        case ErrorCode::SUCCESS:
            return "Success";
        case ErrorCode::PROCESS_NOT_FOUND:
            return "Process not found";
        case ErrorCode::PERMISSION_DENIED:
            return "Permission denied";
        case ErrorCode::FILE_NOT_FOUND:
            return "File not found";
        case ErrorCode::PARSE_ERROR:
            return "Parse error";
        case ErrorCode::PLATFORM_NOT_SUPPORTED:
            return "Platform not supported";
        case ErrorCode::UNKNOWN_ERROR:
        default:
            return "Unknown error";
    }
}

// ============================================================================
// Platform-specific implementations
// ============================================================================

#ifdef __linux__
ProcessMemoryParser::Result<std::vector<MemoryRegion>> 
ProcessMemoryParser::parseLinuxMaps(int pid) {
    std::string maps_path = getMapsFilePath(pid);
    std::ifstream file(maps_path);
    
    if (!file.is_open()) {
        if (errno == ENOENT) {
            return Result<std::vector<MemoryRegion>>(ErrorCode::PROCESS_NOT_FOUND, 
                                                   "Process not found: " + std::to_string(pid));
        } else if (errno == EACCES) {
            return Result<std::vector<MemoryRegion>>(ErrorCode::PERMISSION_DENIED, 
                                                   "Permission denied accessing process: " + std::to_string(pid));
        } else {
            return Result<std::vector<MemoryRegion>>(ErrorCode::FILE_NOT_FOUND, 
                                                   "Cannot open maps file: " + maps_path);
        }
    }
    
    std::vector<MemoryRegion> regions;
    std::string line;
    
    while (std::getline(file, line)) {
        auto region = parseMapsLine(line);
        if (region.has_value() && shouldIncludeRegion(region.value())) {
            regions.push_back(std::move(region.value()));
        }
    }
    
    return Result<std::vector<MemoryRegion>>(std::move(regions));
}
#endif

#ifdef __APPLE__
ProcessMemoryParser::Result<std::vector<MemoryRegion>> 
ProcessMemoryParser::parseMacOSMaps(int pid) {
    // For current process, use task_self()
    mach_port_t task;
    if (pid <= 0) {
        task = mach_task_self();
    } else {
        kern_return_t kr = task_for_pid(mach_task_self(), pid, &task);
        if (kr != KERN_SUCCESS) {
            return Result<std::vector<MemoryRegion>>(ErrorCode::PERMISSION_DENIED, 
                                                   "Cannot get task for process: " + std::to_string(pid));
        }
    }
    
    std::vector<MemoryRegion> regions;
    vm_address_t address = 0;
    vm_size_t size;
    vm_region_flavor_t flavor = VM_REGION_BASIC_INFO_64;
    vm_region_basic_info_data_64_t info;
    mach_msg_type_number_t info_count = VM_REGION_BASIC_INFO_COUNT_64;
    mach_port_t object_name;
    
    while (true) {
        kern_return_t kr = vm_region_64(task, &address, &size, flavor,
                                       (vm_region_info_t)&info, &info_count, &object_name);
        
        if (kr != KERN_SUCCESS) {
            break;
        }
        
        // Convert VM protection to our MemoryPermissions
        MemoryPermissions perms;
        perms.readable = (info.protection & VM_PROT_READ) != 0;
        perms.writable = (info.protection & VM_PROT_WRITE) != 0;
        perms.executable = (info.protection & VM_PROT_EXECUTE) != 0;
        perms.private_mapping = !info.shared;
        
        // Create region (note: macOS doesn't provide device, inode, pathname easily)
        MemoryRegion region(address, address + size, perms, 0, "00:00", 0, "");
        
        if (shouldIncludeRegion(region)) {
            regions.push_back(std::move(region));
        }
        
        address += size;
    }
    
    return Result<std::vector<MemoryRegion>>(std::move(regions));
}
#endif

std::optional<MemoryRegion> ProcessMemoryParser::parseMapsLine(const std::string& line) {
    std::istringstream iss(line);
    std::string addr_range, perms, offset_str, device, inode_str, pathname;
    
    // Parse the line: address permissions offset device inode pathname
    if (!(iss >> addr_range >> perms >> offset_str >> device >> inode_str)) {
        return std::nullopt;
    }
    
    // Get pathname (may contain spaces)
    std::getline(iss, pathname);
    // Trim leading whitespace
    pathname.erase(0, pathname.find_first_not_of(" \t"));
    
    // Parse address range
    size_t dash_pos = addr_range.find('-');
    if (dash_pos == std::string::npos) {
        return std::nullopt;
    }
    
    std::string start_str = addr_range.substr(0, dash_pos);
    std::string end_str = addr_range.substr(dash_pos + 1);
    
    uintptr_t start_addr, end_addr;
    uint64_t offset;
    uint32_t inode;
    
    try {
        start_addr = std::stoull(start_str, nullptr, 16);
        end_addr = std::stoull(end_str, nullptr, 16);
        offset = std::stoull(offset_str, nullptr, 16);
        inode = std::stoul(inode_str);
    } catch (const std::exception&) {
        return std::nullopt;
    }
    
    MemoryPermissions permissions = MemoryPermissions::fromString(perms);
    
    return MemoryRegion(start_addr, end_addr, permissions, offset, device, inode, pathname, line);
}

std::string ProcessMemoryParser::getMapsFilePath(int pid) {
    if (pid <= 0) {
        return "/proc/self/maps";
    } else {
        return "/proc/" + std::to_string(pid) + "/maps";
    }
}

bool ProcessMemoryParser::shouldIncludeRegion(const MemoryRegion& region) const {
    return !region_filter_ || region_filter_(region);
}

} // namespace AnalysisToolkit 