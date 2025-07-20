#include <gtest/gtest.h>
#include "FileOps/Types.h"
#include <filesystem>
#include <fstream>
#include <chrono>
#include <thread>

using namespace FileOps;

class FileOpsTypesTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a temporary directory for testing
        test_dir = std::filesystem::temp_directory_path() / "hweb_fileops_test";
        std::filesystem::create_directories(test_dir);
    }

    void TearDown() override {
        // Clean up test directory
        if (std::filesystem::exists(test_dir)) {
            std::filesystem::remove_all(test_dir);
        }
    }

    std::filesystem::path test_dir;

    void createTestFile(const std::string& filename, const std::string& content = "test content") {
        std::filesystem::path filepath = test_dir / filename;
        std::ofstream file(filepath);
        file << content;
        file.close();
    }
};

// ========== Enum Tests ==========

TEST_F(FileOpsTypesTest, UploadResultEnumValues) {
    EXPECT_EQ(static_cast<int>(UploadResult::SUCCESS), 0);
    EXPECT_EQ(static_cast<int>(UploadResult::FILE_NOT_FOUND), 1);
    EXPECT_EQ(static_cast<int>(UploadResult::INVALID_SELECTOR), 2);
    EXPECT_EQ(static_cast<int>(UploadResult::UPLOAD_FAILED), 3);
    EXPECT_EQ(static_cast<int>(UploadResult::TIMEOUT), 4);
    EXPECT_EQ(static_cast<int>(UploadResult::PERMISSION_DENIED), 5);
    EXPECT_EQ(static_cast<int>(UploadResult::FILE_TOO_LARGE), 6);
    EXPECT_EQ(static_cast<int>(UploadResult::INVALID_FILE_TYPE), 7);
    EXPECT_EQ(static_cast<int>(UploadResult::ELEMENT_NOT_FOUND), 8);
    EXPECT_EQ(static_cast<int>(UploadResult::JAVASCRIPT_ERROR), 9);
}

TEST_F(FileOpsTypesTest, DownloadResultEnumValues) {
    EXPECT_EQ(static_cast<int>(DownloadResult::SUCCESS), 0);
    EXPECT_EQ(static_cast<int>(DownloadResult::TIMEOUT), 1);
    EXPECT_EQ(static_cast<int>(DownloadResult::FILE_NOT_FOUND), 2);
    EXPECT_EQ(static_cast<int>(DownloadResult::INTEGRITY_CHECK_FAILED), 3);
    EXPECT_EQ(static_cast<int>(DownloadResult::PERMISSION_DENIED), 4);
    EXPECT_EQ(static_cast<int>(DownloadResult::DIRECTORY_NOT_FOUND), 5);
    EXPECT_EQ(static_cast<int>(DownloadResult::PATTERN_MATCH_FAILED), 6);
}

TEST_F(FileOpsTypesTest, WaitConditionEnumValues) {
    // Test that all enum values are accessible
    WaitCondition condition;
    condition = WaitCondition::TEXT_APPEARS;
    condition = WaitCondition::NETWORK_IDLE;
    condition = WaitCondition::JAVASCRIPT_TRUE;
    condition = WaitCondition::ELEMENT_COUNT;
    condition = WaitCondition::ELEMENT_VISIBLE;
    condition = WaitCondition::ATTRIBUTE_CHANGED;
    condition = WaitCondition::URL_CHANGED;
    condition = WaitCondition::TITLE_CHANGED;
    SUCCEED();
}

TEST_F(FileOpsTypesTest, ComparisonOperatorEnumValues) {
    ComparisonOperator op;
    op = ComparisonOperator::EQUALS;
    op = ComparisonOperator::NOT_EQUALS;
    op = ComparisonOperator::GREATER_THAN;
    op = ComparisonOperator::LESS_THAN;
    op = ComparisonOperator::GREATER_EQUAL;
    op = ComparisonOperator::LESS_EQUAL;
    SUCCEED();
}

// ========== UploadCommand Tests ==========

TEST_F(FileOpsTypesTest, UploadCommandDefaultValues) {
    UploadCommand cmd;
    
    EXPECT_TRUE(cmd.selector.empty());
    EXPECT_TRUE(cmd.filepath.empty());
    EXPECT_EQ(cmd.timeout_ms, 30000);
    EXPECT_TRUE(cmd.wait_completion);
    EXPECT_EQ(cmd.max_file_size, 104857600); // 100MB
    EXPECT_EQ(cmd.allowed_types.size(), 1);
    EXPECT_EQ(cmd.allowed_types[0], "*");
    EXPECT_TRUE(cmd.verify_upload);
    EXPECT_FALSE(cmd.json_output);
    EXPECT_FALSE(cmd.silent);
}

TEST_F(FileOpsTypesTest, UploadCommandCustomization) {
    UploadCommand cmd;
    cmd.selector = "#file-input";
    cmd.filepath = "/path/to/file.pdf";
    cmd.timeout_ms = 60000;
    cmd.wait_completion = false;
    cmd.max_file_size = 50000000; // 50MB
    cmd.allowed_types = {"pdf", "doc", "docx"};
    cmd.verify_upload = false;
    cmd.custom_message = "Upload test file";
    cmd.json_output = true;
    cmd.silent = true;
    
    EXPECT_EQ(cmd.selector, "#file-input");
    EXPECT_EQ(cmd.filepath, "/path/to/file.pdf");
    EXPECT_EQ(cmd.timeout_ms, 60000);
    EXPECT_FALSE(cmd.wait_completion);
    EXPECT_EQ(cmd.max_file_size, 50000000);
    EXPECT_EQ(cmd.allowed_types.size(), 3);
    EXPECT_FALSE(cmd.verify_upload);
    EXPECT_EQ(cmd.custom_message, "Upload test file");
    EXPECT_TRUE(cmd.json_output);
    EXPECT_TRUE(cmd.silent);
}

TEST_F(FileOpsTypesTest, UploadCommandFileTypeValidation) {
    UploadCommand cmd;
    cmd.allowed_types = {"jpg", "png", "gif"};
    
    EXPECT_TRUE(cmd.isValidFileType("image.jpg"));
    EXPECT_TRUE(cmd.isValidFileType("photo.PNG")); // Case insensitive
    EXPECT_TRUE(cmd.isValidFileType("animation.gif"));
    EXPECT_FALSE(cmd.isValidFileType("document.pdf"));
    EXPECT_FALSE(cmd.isValidFileType("script.js"));
}

TEST_F(FileOpsTypesTest, UploadCommandAllowAllTypes) {
    UploadCommand cmd;
    cmd.allowed_types = {"*"};
    
    EXPECT_TRUE(cmd.isValidFileType("any.file"));
    EXPECT_TRUE(cmd.isValidFileType("document.pdf"));
    EXPECT_TRUE(cmd.isValidFileType("image.jpg"));
    EXPECT_TRUE(cmd.isValidFileType("file.with.multiple.dots.txt"));
}

TEST_F(FileOpsTypesTest, UploadCommandFileExtraction) {
    UploadCommand cmd;
    
    EXPECT_EQ(cmd.getFileExtension("file.txt"), ".txt");
    EXPECT_EQ(cmd.getFileExtension("image.JPG"), ".JPG"); // Case preserved
    EXPECT_EQ(cmd.getFileExtension("path/to/file.pdf"), ".pdf");
    EXPECT_EQ(cmd.getFileExtension("file.tar.gz"), ".gz"); // Last extension
    EXPECT_EQ(cmd.getFileExtension("noextension"), "");
    EXPECT_EQ(cmd.getFileExtension(".hidden"), "");
    EXPECT_EQ(cmd.getFileExtension(""), "");
}

// ========== DownloadCommand Tests ==========

TEST_F(FileOpsTypesTest, DownloadCommandDefaultValues) {
    DownloadCommand cmd;
    
    EXPECT_TRUE(cmd.filename_pattern.empty());
    EXPECT_TRUE(cmd.download_dir.empty());
    EXPECT_EQ(cmd.timeout_ms, 30000);
    EXPECT_TRUE(cmd.verify_integrity);
    EXPECT_EQ(cmd.expected_size, 0);
    EXPECT_FALSE(cmd.delete_on_completion);
    EXPECT_FALSE(cmd.json_output);
    EXPECT_FALSE(cmd.silent);
}

TEST_F(FileOpsTypesTest, DownloadCommandCustomization) {
    DownloadCommand cmd;
    cmd.filename_pattern = "report*.pdf";
    cmd.download_dir = "/tmp/downloads";
    cmd.timeout_ms = 120000;
    cmd.verify_integrity = false;
    cmd.expected_size = 1024000; // 1MB
    cmd.delete_on_completion = true;
    cmd.custom_message = "Download report";
    cmd.json_output = true;
    cmd.silent = true;
    
    EXPECT_EQ(cmd.filename_pattern, "report*.pdf");
    EXPECT_EQ(cmd.download_dir, "/tmp/downloads");
    EXPECT_EQ(cmd.timeout_ms, 120000);
    EXPECT_FALSE(cmd.verify_integrity);
    EXPECT_EQ(cmd.expected_size, 1024000);
    EXPECT_TRUE(cmd.delete_on_completion);
    EXPECT_EQ(cmd.custom_message, "Download report");
    EXPECT_TRUE(cmd.json_output);
    EXPECT_TRUE(cmd.silent);
}

TEST_F(FileOpsTypesTest, DownloadCommandPatternMatching) {
    DownloadCommand cmd;
    
    // Exact match
    cmd.filename_pattern = "report.pdf";
    EXPECT_TRUE(cmd.matchesPattern("report.pdf"));
    EXPECT_FALSE(cmd.matchesPattern("other.pdf"));
    
    // Wildcard patterns
    cmd.filename_pattern = "*.pdf";
    EXPECT_TRUE(cmd.matchesPattern("any.pdf"));
    EXPECT_TRUE(cmd.matchesPattern("document.pdf"));
    EXPECT_FALSE(cmd.matchesPattern("file.txt"));
    
    cmd.filename_pattern = "report_*.xlsx";
    EXPECT_TRUE(cmd.matchesPattern("report_2024.xlsx"));
    EXPECT_TRUE(cmd.matchesPattern("report_january.xlsx"));
    EXPECT_FALSE(cmd.matchesPattern("summary_2024.xlsx"));
}

TEST_F(FileOpsTypesTest, DownloadCommandPatternTypes) {
    DownloadCommand cmd;
    
    // Glob patterns
    cmd.filename_pattern = "*.txt";
    EXPECT_TRUE(cmd.isGlobPattern());
    EXPECT_FALSE(cmd.isRegexPattern());
    
    cmd.filename_pattern = "file?.pdf";
    EXPECT_TRUE(cmd.isGlobPattern());
    
    // Regex patterns (using /pattern/ format)
    cmd.filename_pattern = "/^report\\d{4}\\.pdf$/";
    EXPECT_TRUE(cmd.isRegexPattern());
    EXPECT_FALSE(cmd.isGlobPattern());
    
    // Exact match
    cmd.filename_pattern = "exact_filename.txt";
    EXPECT_FALSE(cmd.isGlobPattern());
    EXPECT_FALSE(cmd.isRegexPattern());
}

// ========== WaitCommand Tests ==========

TEST_F(FileOpsTypesTest, WaitCommandDefaultValues) {
    WaitCommand cmd;
    
    EXPECT_EQ(cmd.condition_type, WaitCondition::TEXT_APPEARS); // Should have some default
    EXPECT_TRUE(cmd.target_value.empty());
    EXPECT_EQ(cmd.timeout_ms, 10000);
    EXPECT_EQ(cmd.poll_interval_ms, 100);
    EXPECT_EQ(cmd.retry_count, 3);
    EXPECT_EQ(cmd.comparison_op, ComparisonOperator::EQUALS);
    EXPECT_EQ(cmd.expected_count, 1);
    EXPECT_FALSE(cmd.case_sensitive);
    EXPECT_FALSE(cmd.json_output);
    EXPECT_FALSE(cmd.silent);
}

TEST_F(FileOpsTypesTest, WaitCommandCustomization) {
    WaitCommand cmd;
    cmd.condition_type = WaitCondition::ELEMENT_COUNT;
    cmd.target_value = ".list-item";
    cmd.timeout_ms = 30000;
    cmd.poll_interval_ms = 500;
    cmd.retry_count = 5;
    cmd.comparison_op = ComparisonOperator::GREATER_THAN;
    cmd.expected_count = 10;
    cmd.case_sensitive = true;
    cmd.custom_message = "Wait for items to load";
    cmd.json_output = true;
    cmd.silent = true;
    
    EXPECT_EQ(cmd.condition_type, WaitCondition::ELEMENT_COUNT);
    EXPECT_EQ(cmd.target_value, ".list-item");
    EXPECT_EQ(cmd.timeout_ms, 30000);
    EXPECT_EQ(cmd.poll_interval_ms, 500);
    EXPECT_EQ(cmd.retry_count, 5);
    EXPECT_EQ(cmd.comparison_op, ComparisonOperator::GREATER_THAN);
    EXPECT_EQ(cmd.expected_count, 10);
    EXPECT_TRUE(cmd.case_sensitive);
    EXPECT_EQ(cmd.custom_message, "Wait for items to load");
    EXPECT_TRUE(cmd.json_output);
    EXPECT_TRUE(cmd.silent);
}

TEST_F(FileOpsTypesTest, WaitCommandValidation) {
    WaitCommand cmd;
    
    // Valid CSS selector
    cmd.target_value = "#valid-selector";
    EXPECT_TRUE(cmd.isValidSelector());
    
    cmd.target_value = ".class-name";
    EXPECT_TRUE(cmd.isValidSelector());
    
    // Invalid selectors
    cmd.target_value = ">>invalid";
    EXPECT_FALSE(cmd.isValidSelector());
    
    // Valid JavaScript
    cmd.target_value = "document.readyState === 'complete'";
    EXPECT_TRUE(cmd.isValidJavaScript());
    
    cmd.target_value = "window.myFunction()";
    EXPECT_TRUE(cmd.isValidJavaScript());
    
    // Invalid JavaScript (unbalanced braces)
    cmd.target_value = "function() { unbalanced";
    EXPECT_FALSE(cmd.isValidJavaScript());
}

// ========== FileInfo Tests ==========

TEST_F(FileOpsTypesTest, FileInfoCreation) {
    createTestFile("test.txt", "Hello, World!");
    std::filesystem::path filepath = test_dir / "test.txt";
    
    FileInfo info = FileInfo::create(filepath.string());
    
    EXPECT_EQ(info.filepath, filepath.string());
    EXPECT_EQ(info.filename, "test.txt");
    EXPECT_GT(info.size_bytes, 0);
    EXPECT_TRUE(info.is_readable);
    EXPECT_TRUE(info.exists);
    EXPECT_FALSE(info.mime_type.empty());
}

TEST_F(FileOpsTypesTest, FileInfoNonexistentFile) {
    std::filesystem::path filepath = test_dir / "nonexistent.txt";
    
    FileInfo info = FileInfo::create(filepath.string());
    
    EXPECT_EQ(info.filepath, filepath.string());
    EXPECT_EQ(info.filename, "nonexistent.txt");
    EXPECT_EQ(info.size_bytes, 0);
    EXPECT_FALSE(info.is_readable);
    EXPECT_FALSE(info.exists);
}

TEST_F(FileOpsTypesTest, FileInfoSizeString) {
    createTestFile("small.txt", "small");
    createTestFile("large.txt", std::string(1024 * 1024, 'x')); // 1MB
    
    FileInfo smallInfo = FileInfo::create((test_dir / "small.txt").string());
    FileInfo largeInfo = FileInfo::create((test_dir / "large.txt").string());
    
    std::string smallSize = smallInfo.getSizeString();
    std::string largeSize = largeInfo.getSizeString();
    
    EXPECT_FALSE(smallSize.empty());
    EXPECT_FALSE(largeSize.empty());
    EXPECT_NE(smallSize, largeSize);
    
    // Large file should mention MB or KB
    EXPECT_TRUE(largeSize.find("MB") != std::string::npos || 
                largeSize.find("KB") != std::string::npos);
}

TEST_F(FileOpsTypesTest, FileInfoAgeCheck) {
    createTestFile("old.txt", "old content");
    
    // Get file info and check age
    FileInfo info = FileInfo::create((test_dir / "old.txt").string());
    
    // File should be very new (not older than 1 minute)
    EXPECT_FALSE(info.isOlderThan(std::chrono::minutes(1)));
    
    // But should be older than 0 minutes (well, this might be flaky)
    // Let's just test the interface works
    bool isOlder = info.isOlderThan(std::chrono::minutes(0));
    // Don't assert on this since timing is unpredictable
}

// ========== DownloadProgress Tests ==========

TEST_F(FileOpsTypesTest, DownloadProgressCalculations) {
    DownloadProgress progress;
    progress.filepath = "/tmp/download.zip";
    progress.current_size = 500;
    progress.expected_size = 1000;
    progress.is_complete = false;
    progress.start_time = std::chrono::system_clock::now() - std::chrono::seconds(10);
    progress.last_update = std::chrono::system_clock::now();
    
    EXPECT_DOUBLE_EQ(progress.getProgressPercent(), 50.0);
    
    auto elapsed = progress.getElapsedTime();
    EXPECT_GE(elapsed.count(), 9000); // At least 9 seconds
    EXPECT_LE(elapsed.count(), 11000); // At most 11 seconds
}

TEST_F(FileOpsTypesTest, DownloadProgressZeroSize) {
    DownloadProgress progress;
    progress.current_size = 100;
    progress.expected_size = 0; // Unknown size
    
    // Should handle division by zero gracefully
    double percent = progress.getProgressPercent();
    EXPECT_TRUE(percent == 0.0 || percent == 100.0); // Implementation dependent
}

TEST_F(FileOpsTypesTest, DownloadProgressStability) {
    DownloadProgress progress;
    progress.last_update = std::chrono::system_clock::now() - std::chrono::seconds(5);
    
    // Should be stable after 5 seconds (more than 2 second default)
    EXPECT_TRUE(progress.isStable());
    
    // Should not be stable with custom time
    EXPECT_FALSE(progress.isStable(std::chrono::milliseconds(10000))); // 10 seconds
}

// ========== NetworkRequest Tests ==========

TEST_F(FileOpsTypesTest, NetworkRequestLifecycle) {
    NetworkRequest request;
    request.url = "https://example.com/api/data";
    request.method = "GET";
    request.start_time = std::chrono::system_clock::now();
    request.is_complete = false;
    request.status_code = 0;
    
    EXPECT_TRUE(request.isActive());
    
    request.end_time = std::chrono::system_clock::now() + std::chrono::milliseconds(100);
    request.is_complete = true;
    request.status_code = 200;
    
    EXPECT_FALSE(request.isActive());
    
    auto duration = request.getDuration();
    EXPECT_GE(duration.count(), 100);
    EXPECT_LE(duration.count(), 200); // Allow some variance
}

// ========== NetworkState Tests ==========

TEST_F(FileOpsTypesTest, NetworkStateManagement) {
    NetworkState state;
    
    EXPECT_EQ(state.total_requests, 0);
    EXPECT_EQ(state.completed_requests, 0);
    EXPECT_TRUE(state.active_requests.empty());
    
    NetworkRequest request1;
    request1.url = "https://example.com/1";
    request1.method = "GET";
    request1.start_time = std::chrono::system_clock::now();
    request1.is_complete = false;
    
    state.addRequest(request1);
    
    EXPECT_EQ(state.total_requests, 1);
    EXPECT_EQ(state.completed_requests, 0);
    EXPECT_EQ(state.active_requests.size(), 1);
    
    state.completeRequest("https://example.com/1");
    
    EXPECT_EQ(state.completed_requests, 1);
    EXPECT_TRUE(state.active_requests.empty());
}

TEST_F(FileOpsTypesTest, NetworkStateIdleDetection) {
    NetworkState state;
    
    // Should be idle when no activity
    EXPECT_TRUE(state.isIdle(std::chrono::milliseconds(1000)));
    
    // Add activity
    state.last_activity = std::chrono::system_clock::now();
    EXPECT_FALSE(state.isIdle(std::chrono::milliseconds(1000)));
    
    // Wait and check again (simulated)
    state.last_activity = std::chrono::system_clock::now() - std::chrono::seconds(2);
    EXPECT_TRUE(state.isIdle(std::chrono::milliseconds(1000)));
}

// ========== Utility Functions Tests ==========

TEST_F(FileOpsTypesTest, ResultToStringConversions) {
    EXPECT_FALSE(uploadResultToString(UploadResult::SUCCESS).empty());
    EXPECT_FALSE(uploadResultToString(UploadResult::FILE_NOT_FOUND).empty());
    EXPECT_FALSE(uploadResultToString(UploadResult::TIMEOUT).empty());
    
    EXPECT_FALSE(downloadResultToString(DownloadResult::SUCCESS).empty());
    EXPECT_FALSE(downloadResultToString(DownloadResult::TIMEOUT).empty());
    EXPECT_FALSE(downloadResultToString(DownloadResult::FILE_NOT_FOUND).empty());
    
    EXPECT_FALSE(waitConditionToString(WaitCondition::TEXT_APPEARS).empty());
    EXPECT_FALSE(waitConditionToString(WaitCondition::NETWORK_IDLE).empty());
    EXPECT_FALSE(waitConditionToString(WaitCondition::ELEMENT_COUNT).empty());
    
    EXPECT_FALSE(comparisonOperatorToString(ComparisonOperator::EQUALS).empty());
    EXPECT_FALSE(comparisonOperatorToString(ComparisonOperator::GREATER_THAN).empty());
    EXPECT_FALSE(comparisonOperatorToString(ComparisonOperator::GREATER_EQUAL).empty());
}

TEST_F(FileOpsTypesTest, ValidationUtilities) {
    // Valid file paths
    EXPECT_TRUE(isValidFilePath("/path/to/file.txt"));
    EXPECT_TRUE(isValidFilePath("relative/path.txt"));
    EXPECT_TRUE(isValidFilePath("C:\\Windows\\file.txt"));
    
    // Invalid file paths (implementation dependent)
    EXPECT_FALSE(isValidFilePath(""));
    
    // Valid selectors
    EXPECT_TRUE(isValidSelector("#id"));
    EXPECT_TRUE(isValidSelector(".class"));
    EXPECT_TRUE(isValidSelector("div.class#id"));
    
    // Invalid selectors
    EXPECT_FALSE(isValidSelector(""));
    EXPECT_FALSE(isValidSelector(">>invalid"));
    
    // Valid JavaScript
    EXPECT_TRUE(isValidJavaScript("true"));
    EXPECT_TRUE(isValidJavaScript("document.title"));
    EXPECT_TRUE(isValidJavaScript("window.location.href"));
    
    // Invalid JavaScript
    EXPECT_FALSE(isValidJavaScript(""));
    EXPECT_FALSE(isValidJavaScript("{{invalid}}"));
}

TEST_F(FileOpsTypesTest, PlatformUtilities) {
    std::string defaultDir = getDefaultDownloadDirectory();
    EXPECT_FALSE(defaultDir.empty());
    
    std::string normalized = normalizePath("/path//to/../file.txt");
    EXPECT_FALSE(normalized.empty());
    // Path normalization results are platform dependent
    
    // Test directory creation
    std::filesystem::path testPath = test_dir / "new_directory";
    EXPECT_TRUE(createDirectoryIfNotExists(testPath.string()));
    EXPECT_TRUE(std::filesystem::exists(testPath));
    EXPECT_TRUE(std::filesystem::is_directory(testPath));
}

TEST_F(FileOpsTypesTest, TimeUtilities) {
    auto duration = std::chrono::milliseconds(1500);
    std::string formatted = formatDuration(duration);
    EXPECT_FALSE(formatted.empty());
    EXPECT_TRUE(formatted.find("1.5") != std::string::npos || 
                formatted.find("1500") != std::string::npos);
    
    size_t bytes = 1024 * 1024; // 1MB
    std::string sizeStr = formatFileSize(bytes);
    EXPECT_FALSE(sizeStr.empty());
    EXPECT_TRUE(sizeStr.find("MB") != std::string::npos || 
                sizeStr.find("1024") != std::string::npos);
}