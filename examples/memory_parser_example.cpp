/**
 * @file memory_parser_example.cpp
 * @brief Example usage of ProcessMemoryParser
 *
 * This example demonstrates how to use the ProcessMemoryParser to:
 * 1. Parse current process memory maps
 * 2. Find specific memory regions
 * 3. Filter regions by various criteria
 * 4. Print memory information
 */

#include <iomanip>
#include <iostream>

#include "utility/ProcessMemoryParser.h"

using namespace AnalysisToolkit;

void printSeparator(const std::string& title) {
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "  " << title << std::endl;
    std::cout << std::string(60, '=') << std::endl;
}

void demonstrateBasicUsage() {
    printSeparator("Basic Usage: Parse Current Process");

    ProcessMemoryParser parser;

    auto result = parser.parseSelf();
    if (result.hasError()) {
        std::cerr << "Error parsing memory maps: "
                  << ProcessMemoryParser::getErrorString(result.getError()) << " - "
                  << result.getErrorMessage() << std::endl;
        return;
    }

    const auto& regions = result.getValue();
    std::cout << "Found " << regions.size() << " memory regions" << std::endl;

    // Print first 10 regions
    ProcessMemoryParser::printMemoryMap(regions, 10);
}

void demonstrateAddressSearch() {
    printSeparator("Find Regions Containing Specific Address");

    ProcessMemoryParser parser;

    // Get the address of this function
    uintptr_t func_address = reinterpret_cast<uintptr_t>(&demonstrateAddressSearch);
    std::cout << "Searching for regions containing address: 0x" << std::hex << func_address
              << std::dec << std::endl;

    auto result = parser.findRegionsContaining(func_address);
    if (result.hasError()) {
        std::cerr << "Error finding regions: "
                  << ProcessMemoryParser::getErrorString(result.getError()) << std::endl;
        return;
    }

    const auto& regions = result.getValue();
    if (regions.empty()) {
        std::cout << "No regions found containing this address" << std::endl;
    } else {
        std::cout << "Found " << regions.size() << " regions:" << std::endl;
        ProcessMemoryParser::printMemoryMap(regions);
    }
}

void demonstratePermissionFiltering() {
    printSeparator("Find Executable Regions");

    ProcessMemoryParser parser;

    // Create permissions filter for executable regions
    MemoryPermissions exec_perms;
    exec_perms.executable = true;

    auto result = parser.findRegionsByPermissions(exec_perms);
    if (result.hasError()) {
        std::cerr << "Error finding executable regions: "
                  << ProcessMemoryParser::getErrorString(result.getError()) << std::endl;
        return;
    }

    const auto& regions = result.getValue();
    std::cout << "Found " << regions.size() << " executable regions:" << std::endl;
    ProcessMemoryParser::printMemoryMap(regions, 5);  // Show first 5
}

void demonstratePathFiltering() {
    printSeparator("Find Regions by Library Path");

    ProcessMemoryParser parser;

    // Search for regions containing "lib" in the path
    auto result = parser.findRegionsByPath("lib", -1, false);
    if (result.hasError()) {
        std::cerr << "Error finding library regions: "
                  << ProcessMemoryParser::getErrorString(result.getError()) << std::endl;
        return;
    }

    const auto& regions = result.getValue();
    std::cout << "Found " << regions.size() << " regions with 'lib' in path:" << std::endl;
    ProcessMemoryParser::printMemoryMap(regions, 10);  // Show first 10
}

void demonstrateCustomFiltering() {
    printSeparator("Custom Filtering: Large Anonymous Regions");

    ProcessMemoryParser parser;

    // Set a custom filter for large anonymous regions (> 1MB)
    parser.setRegionFilter([](const MemoryRegion& region) {
        return region.isAnonymous() && region.getSize() > (1024 * 1024);
    });

    auto result = parser.parseSelf();
    if (result.hasError()) {
        std::cerr << "Error parsing with filter: "
                  << ProcessMemoryParser::getErrorString(result.getError()) << std::endl;
        return;
    }

    const auto& regions = result.getValue();
    std::cout << "Found " << regions.size() << " large anonymous regions (>1MB):" << std::endl;

    for (const auto& region : regions) {
        std::cout << "  " << region.toString() << " (Size: " << (region.getSize() / 1024 / 1024)
                  << " MB)" << std::endl;
    }

    // Clear the filter
    parser.clearRegionFilter();
}

void demonstrateRegionAnalysis() {
    printSeparator("Memory Region Analysis");

    ProcessMemoryParser parser;
    auto result = parser.parseSelf();

    if (result.hasError()) {
        std::cerr << "Error parsing memory: "
                  << ProcessMemoryParser::getErrorString(result.getError()) << std::endl;
        return;
    }

    const auto& regions = result.getValue();

    // Analyze memory usage
    size_t total_memory = 0;
    size_t executable_memory = 0;
    size_t writable_memory = 0;
    size_t anonymous_memory = 0;
    int heap_regions = 0;
    int stack_regions = 0;

    for (const auto& region : regions) {
        size_t size = region.getSize();
        total_memory += size;

        if (region.getPermissions().executable) {
            executable_memory += size;
        }
        if (region.getPermissions().writable) {
            writable_memory += size;
        }
        if (region.isAnonymous()) {
            anonymous_memory += size;
        }
        if (region.isHeap()) {
            heap_regions++;
        }
        if (region.isStack()) {
            stack_regions++;
        }
    }

    std::cout << "Memory Usage Summary:" << std::endl;
    std::cout << "  Total regions: " << regions.size() << std::endl;
    std::cout << "  Total memory: " << (total_memory / 1024 / 1024) << " MB" << std::endl;
    std::cout << "  Executable memory: " << (executable_memory / 1024 / 1024) << " MB" << std::endl;
    std::cout << "  Writable memory: " << (writable_memory / 1024 / 1024) << " MB" << std::endl;
    std::cout << "  Anonymous memory: " << (anonymous_memory / 1024 / 1024) << " MB" << std::endl;
    std::cout << "  Heap regions: " << heap_regions << std::endl;
    std::cout << "  Stack regions: " << stack_regions << std::endl;
}

int main() {
    std::cout << "ProcessMemoryParser Example" << std::endl;
    std::cout << "Platform supported: "
              << (ProcessMemoryParser::isPlatformSupported() ? "Yes" : "No") << std::endl;

    if (!ProcessMemoryParser::isPlatformSupported()) {
        std::cerr << "This platform is not supported!" << std::endl;
        return 1;
    }

    try {
        // Demonstrate various features
        demonstrateBasicUsage();
        demonstrateAddressSearch();
        demonstratePermissionFiltering();
        demonstratePathFiltering();
        demonstrateCustomFiltering();
        demonstrateRegionAnalysis();

        std::cout << "\nAll examples completed successfully!" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Exception occurred: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
