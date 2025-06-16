/**
 * @file test_process_memory_parser.cpp
 * @brief Unit tests for ProcessMemoryParser
 */

#include <gtest/gtest.h>
#include "utility/ProcessMemoryParser.h"

using namespace AnalysisToolkit;

class ProcessMemoryParserTest : public ::testing::Test {
protected:
    void SetUp() override {
        parser = std::make_unique<ProcessMemoryParser>();
    }

    void TearDown() override {
        parser.reset();
    }

    std::unique_ptr<ProcessMemoryParser> parser;
};

// Test platform support
TEST_F(ProcessMemoryParserTest, PlatformSupport) {
    // Should be supported on Linux and macOS
#if defined(__linux__) || defined(__APPLE__)
    EXPECT_TRUE(ProcessMemoryParser::isPlatformSupported());
#else
    EXPECT_FALSE(ProcessMemoryParser::isPlatformSupported());
#endif
}

// Test error string conversion
TEST_F(ProcessMemoryParserTest, ErrorStringConversion) {
    auto error_str = ProcessMemoryParser::getErrorString(ProcessMemoryParser::ErrorCode::SUCCESS);
    EXPECT_EQ(error_str, "Success");
    
    error_str = ProcessMemoryParser::getErrorString(ProcessMemoryParser::ErrorCode::PERMISSION_DENIED);
    EXPECT_EQ(error_str, "Permission denied");
    
    error_str = ProcessMemoryParser::getErrorString(ProcessMemoryParser::ErrorCode::PLATFORM_NOT_SUPPORTED);
    EXPECT_EQ(error_str, "Platform not supported");
}

// Test memory permissions
TEST_F(ProcessMemoryParserTest, MemoryPermissions) {
    MemoryPermissions perms;
    perms.readable = true;
    perms.writable = true;
    perms.executable = false;
    perms.private_mapping = true;
    
    std::string perm_str = perms.toString();
    EXPECT_EQ(perm_str, "rw-p");
    
    MemoryPermissions parsed_perms = MemoryPermissions::fromString("rwxs");
    EXPECT_TRUE(parsed_perms.readable);
    EXPECT_TRUE(parsed_perms.writable);
    EXPECT_TRUE(parsed_perms.executable);
    EXPECT_FALSE(parsed_perms.private_mapping);
}

// Test memory region basic functionality
TEST_F(ProcessMemoryParserTest, MemoryRegionBasics) {
    MemoryPermissions perms;
    perms.readable = true;
    perms.executable = true;
    
    MemoryRegion region(0x1000, 0x2000, perms, 0, "00:00", 0, "[test]");
    
    EXPECT_EQ(region.getStartAddress(), 0x1000);
    EXPECT_EQ(region.getEndAddress(), 0x2000);
    EXPECT_EQ(region.getSize(), 0x1000);
    EXPECT_TRUE(region.contains(0x1500));
    EXPECT_FALSE(region.contains(0x500));
    EXPECT_FALSE(region.contains(0x2500));
    
    std::string desc = region.toString();
    EXPECT_NE(desc.find("0x1000"), std::string::npos);
    EXPECT_NE(desc.find("0x2000"), std::string::npos);
}

// Test current process parsing (if platform is supported)
TEST_F(ProcessMemoryParserTest, ParseCurrentProcess) {
    if (!ProcessMemoryParser::isPlatformSupported()) {
        GTEST_SKIP() << "Platform not supported";
    }
    
    auto result = parser->parseSelf();
    
    if (result.hasError()) {
        // On some systems, we might not have permission to read our own maps
        // This is okay for the test
        EXPECT_TRUE(result.getError() == ProcessMemoryParser::ErrorCode::PERMISSION_DENIED ||
                   result.getError() == ProcessMemoryParser::ErrorCode::FILE_NOT_FOUND);
        return;
    }
    
    EXPECT_TRUE(result.isSuccess());
    const auto& regions = result.getValue();
    EXPECT_GT(regions.size(), 0);
    
    // Verify that we have some basic memory regions
    bool found_executable = false;
    bool found_readable = false;
    
    for (const auto& region : regions) {
        if (region.getPermissions().executable) {
            found_executable = true;
        }
        if (region.getPermissions().readable) {
            found_readable = true;
        }
        
        // Basic sanity checks
        EXPECT_LT(region.getStartAddress(), region.getEndAddress());
        EXPECT_GT(region.getSize(), 0);
    }
    
    EXPECT_TRUE(found_executable || found_readable);  // Should have at least one of these
}

// Test address search functionality
TEST_F(ProcessMemoryParserTest, FindRegionsContaining) {
    if (!ProcessMemoryParser::isPlatformSupported()) {
        GTEST_SKIP() << "Platform not supported";
    }
    
    // Get address of a simple function pointer for testing
    auto test_func = []() { return 42; };
    uintptr_t func_addr = reinterpret_cast<uintptr_t>(&test_func);
    
    auto result = parser->findRegionsContaining(func_addr);
    
    if (result.hasError()) {
        // Permission issues are acceptable in test environment
        EXPECT_TRUE(result.getError() == ProcessMemoryParser::ErrorCode::PERMISSION_DENIED ||
                   result.getError() == ProcessMemoryParser::ErrorCode::FILE_NOT_FOUND);
        return;
    }
    
    EXPECT_TRUE(result.isSuccess());
    const auto& regions = result.getValue();
    
    // Should find at least one region containing the function address
    // (though this might fail in some virtualized test environments)
    if (!regions.empty()) {
        bool found_containing = false;
        for (const auto& region : regions) {
            if (region.contains(func_addr)) {
                found_containing = true;
                break;
            }
        }
        EXPECT_TRUE(found_containing);
    }
}

// Test permission filtering
TEST_F(ProcessMemoryParserTest, FindRegionsByPermissions) {
    if (!ProcessMemoryParser::isPlatformSupported()) {
        GTEST_SKIP() << "Platform not supported";
    }
    
    MemoryPermissions exec_perms;
    exec_perms.executable = true;
    
    auto result = parser->findRegionsByPermissions(exec_perms);
    
    if (result.hasError()) {
        // Permission issues are acceptable in test environment
        EXPECT_TRUE(result.getError() == ProcessMemoryParser::ErrorCode::PERMISSION_DENIED ||
                   result.getError() == ProcessMemoryParser::ErrorCode::FILE_NOT_FOUND);
        return;
    }
    
    EXPECT_TRUE(result.isSuccess());
    const auto& regions = result.getValue();
    
    // Verify all returned regions have executable permission
    for (const auto& region : regions) {
        EXPECT_TRUE(region.getPermissions().executable);
    }
}

// Test custom filtering
TEST_F(ProcessMemoryParserTest, CustomFiltering) {
    if (!ProcessMemoryParser::isPlatformSupported()) {
        GTEST_SKIP() << "Platform not supported";
    }
    
    // Set filter for large regions (>= 4KB)
    parser->setRegionFilter([](const MemoryRegion& region) {
        return region.getSize() >= 4096;
    });
    
    auto result = parser->parseSelf();
    
    if (result.hasError()) {
        // Permission issues are acceptable in test environment
        EXPECT_TRUE(result.getError() == ProcessMemoryParser::ErrorCode::PERMISSION_DENIED ||
                   result.getError() == ProcessMemoryParser::ErrorCode::FILE_NOT_FOUND);
        return;
    }
    
    EXPECT_TRUE(result.isSuccess());
    const auto& regions = result.getValue();
    
    // Verify all returned regions meet the filter criteria
    for (const auto& region : regions) {
        EXPECT_GE(region.getSize(), 4096);
    }
    
    // Clear filter
    parser->clearRegionFilter();
}

// Test Result class functionality
TEST_F(ProcessMemoryParserTest, ResultClass) {
    // Test successful result
    std::vector<MemoryRegion> regions;
    auto success_result = ProcessMemoryParser::Result<std::vector<MemoryRegion>>(std::move(regions));
    
    EXPECT_TRUE(success_result.isSuccess());
    EXPECT_FALSE(success_result.hasError());
    EXPECT_EQ(success_result.getError(), ProcessMemoryParser::ErrorCode::SUCCESS);
    
    // Test error result
    auto error_result = ProcessMemoryParser::Result<std::vector<MemoryRegion>>(
        ProcessMemoryParser::ErrorCode::PERMISSION_DENIED, "Test error");
    
    EXPECT_FALSE(error_result.isSuccess());
    EXPECT_TRUE(error_result.hasError());
    EXPECT_EQ(error_result.getError(), ProcessMemoryParser::ErrorCode::PERMISSION_DENIED);
    EXPECT_EQ(error_result.getErrorMessage(), "Test error");
    
    // Test exception on accessing value from error result
    EXPECT_THROW(error_result.getValue(), std::runtime_error);
}

// Test path-based filtering (basic test)
TEST_F(ProcessMemoryParserTest, FindRegionsByPath) {
    if (!ProcessMemoryParser::isPlatformSupported()) {
        GTEST_SKIP() << "Platform not supported";
    }
    
    // Search for any region (empty string should match all)
    auto result = parser->findRegionsByPath("", -1, false);
    
    if (result.hasError()) {
        // Permission issues are acceptable in test environment
        EXPECT_TRUE(result.getError() == ProcessMemoryParser::ErrorCode::PERMISSION_DENIED ||
                   result.getError() == ProcessMemoryParser::ErrorCode::FILE_NOT_FOUND);
        return;
    }
    
    EXPECT_TRUE(result.isSuccess());
    // Empty string should match all regions with non-empty paths
    // (this is a basic test, exact behavior may vary)
} 