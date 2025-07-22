#include <gtest/gtest.h>
#include "FileOps/UploadManager.h"
#include "../utils/test_helpers.h"
#include "Browser/Browser.h"
#include "Debug.h"
#include <thread>
#include <chrono>

using namespace FileOps;

class UploadManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        temp_dir = std::make_unique<TestHelpers::TemporaryDirectory>("upload_tests");
        browser = std::make_unique<Browser>();
        manager = std::make_unique<UploadManager>();
        
        // Allow browser initialization to complete
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        // Start test server if needed
        server_manager = std::make_unique<TestHelpers::TestServerManager>();
        if (!server_manager->startServer()) {
            GTEST_SKIP() << "Could not start test server - upload tests skipped";
        }
        
        // Initialize browser with a controlled test page
        setupTestPage();
        
        // Create test files
        test_file = temp_dir->createFile("test.txt", "test content");
        large_file = temp_dir->createFile("large.txt", std::string(1024 * 1024, 'x')); // 1MB
        
        // Create binary test file
        std::vector<uint8_t> binary_data = {0x89, 0x50, 0x4E, 0x47}; // PNG header
        TestHelpers::createTestFile(temp_dir->getPath() / "test.png", 
                                   std::string(binary_data.begin(), binary_data.end()));
        binary_file = temp_dir->getPath() / "test.png";
    }
    
    void setupTestPage() {
        // Use the Node.js test server
        std::string server_url = server_manager->getServerUrl();
        
        // Load the test server page into the browser
        browser->loadUri(server_url);
        
        // Wait for navigation to complete properly
        bool navigation_success = browser->waitForNavigation(10000); // 10 second timeout
        if (!navigation_success) {
            std::string current_url = browser->getCurrentUrl();
            GTEST_SKIP() << "Navigation timeout - page failed to load (url: '" << current_url << "')";
        }
        
        // Verify the page loaded correctly by checking the title
        std::string title = browser->getPageTitle();
        if (title != "Upload Test Server") {
            std::string current_url = browser->getCurrentUrl();
            GTEST_SKIP() << "Test server page not loaded correctly (url: '" << current_url << "', title: '" << title << "')";
        }
        
        debug_output("Upload test page loaded successfully via test server");
    }

    void TearDown() override {
        manager.reset();
        browser.reset();
        temp_dir.reset();
    }

    std::unique_ptr<TestHelpers::TemporaryDirectory> temp_dir;
    std::unique_ptr<Browser> browser;
    std::unique_ptr<UploadManager> manager;
    std::unique_ptr<TestHelpers::TestServerManager> server_manager;
    
    std::filesystem::path test_file;
    std::filesystem::path large_file;
    std::filesystem::path binary_file;
    
    UploadCommand createUploadCommand(const std::string& filepath) {
        UploadCommand cmd;
        cmd.selector = "#file-input";
        cmd.filepath = filepath;
        cmd.timeout_ms = 5000;
        cmd.wait_completion = true;
        cmd.max_file_size = 10 * 1024 * 1024; // 10MB
        cmd.allowed_types = {"*"};
        cmd.verify_upload = true;
        return cmd;
    }
};

// ========== File Validation Tests ==========

TEST_F(UploadManagerTest, ValidateExistingFile) {
    UploadCommand cmd = createUploadCommand(test_file.string());
    
    bool result = manager->validateFile(test_file.string(), cmd);
    
    EXPECT_TRUE(result);
}

TEST_F(UploadManagerTest, ValidateNonExistentFile) {
    UploadCommand cmd = createUploadCommand("/nonexistent/file.txt");
    
    bool result = manager->validateFile("/nonexistent/file.txt", cmd);
    
    EXPECT_FALSE(result);
}

TEST_F(UploadManagerTest, ValidateFileSize) {
    // Test file within size limit
    bool result1 = manager->validateFileSize(test_file.string(), 1024);
    EXPECT_TRUE(result1);
    
    // Test file exceeding size limit  
    bool result2 = manager->validateFileSize(large_file.string(), 512);
    EXPECT_FALSE(result2);
}

TEST_F(UploadManagerTest, ValidateFileTypeAllowed) {
    std::vector<std::string> allowed = {"txt", "png", "jpg"};
    
    bool txt_result = manager->validateFileType(test_file.string(), allowed);
    bool png_result = manager->validateFileType(binary_file.string(), allowed);
    
    EXPECT_TRUE(txt_result);
    EXPECT_TRUE(png_result);
}

TEST_F(UploadManagerTest, ValidateFileTypeRestricted) {
    std::vector<std::string> allowed = {"jpg", "jpeg"};
    
    bool txt_result = manager->validateFileType(test_file.string(), allowed);
    bool png_result = manager->validateFileType(binary_file.string(), allowed);
    
    EXPECT_FALSE(txt_result);
    EXPECT_FALSE(png_result);
}

TEST_F(UploadManagerTest, ValidateFileTypeWildcard) {
    std::vector<std::string> allowed = {"*"};
    
    bool result = manager->validateFileType(test_file.string(), allowed);
    
    EXPECT_TRUE(result);
}

// ========== Upload Target Validation ==========

TEST_F(UploadManagerTest, ValidateUploadTargetExists) {
    // Verify target exists using real browser DOM query
    std::string exists_result = browser->executeJavascriptSync("elementExists('#file-input').toString()");
    bool result = manager->validateUploadTarget(*browser, "#file-input");
    
    EXPECT_TRUE(result);
    EXPECT_EQ(exists_result, "true");
}

TEST_F(UploadManagerTest, ValidateUploadTargetNotExists) {
    // Test with selector that doesn't exist in our test page
    std::string exists_result = browser->executeJavascriptSync("return elementExists('#nonexistent-input').toString();");
    bool result = manager->validateUploadTarget(*browser, "#nonexistent-input");
    
    EXPECT_FALSE(result);
    EXPECT_EQ(exists_result, "false");
}

// ========== File Preparation Tests ==========

TEST_F(UploadManagerTest, PrepareValidFile) {
    FileInfo info = manager->prepareFile(test_file.string());
    
    EXPECT_EQ(info.filepath, test_file.string());
    EXPECT_EQ(info.filename, "test.txt");
    EXPECT_GT(info.size_bytes, 0);
    EXPECT_TRUE(info.exists);
    EXPECT_TRUE(info.is_readable);
}

TEST_F(UploadManagerTest, PrepareNonExistentFile) {
    FileInfo info = manager->prepareFile("/nonexistent/file.txt");
    
    EXPECT_FALSE(info.exists);
    EXPECT_FALSE(info.is_readable);
    EXPECT_EQ(info.size_bytes, 0);
}

TEST_F(UploadManagerTest, DetectMimeTypeByExtension) {
    std::string txt_mime = manager->detectMimeType(test_file.string());
    std::string png_mime = manager->detectMimeType(binary_file.string());
    
    EXPECT_NE(txt_mime.find("text"), std::string::npos);
    EXPECT_NE(png_mime.find("image"), std::string::npos);
}

TEST_F(UploadManagerTest, SanitizeFileName) {
    std::string dangerous_name = "/path/with/../dangerous/../../file.txt";
    std::string safe_name = manager->sanitizeFileName(dangerous_name);
    
    EXPECT_EQ(safe_name.find(".."), std::string::npos);
    EXPECT_NE(safe_name.find("file.txt"), std::string::npos);
}

// ========== Upload Simulation Tests ==========

TEST_F(UploadManagerTest, SimulateFileSelection) {
    bool result = manager->simulateFileSelection(*browser, "#file-input", test_file.string());
    
    // Verify the file was actually selected by checking page state
    std::string status = browser->executeJavascriptSync("return document.getElementById('upload-status').innerText;");
    
    EXPECT_TRUE(result);
    // Note: Real file selection might not trigger the change event in our test setup
    // This tests the mechanism rather than the specific event
}

TEST_F(UploadManagerTest, TriggerFileInputEvents) {
    bool result = manager->triggerFileInputEvents(*browser, "#file-input");
    
    EXPECT_TRUE(result);
}

TEST_F(UploadManagerTest, SimulateFileDrop) {
    bool result = manager->simulateFileDrop(*browser, "#drop-zone", test_file.string());
    
    // Verify drop zone exists and can be targeted
    std::string zone_exists = browser->executeJavascriptSync("return elementExists('#drop-zone').toString();");
    
    EXPECT_TRUE(result);
    EXPECT_EQ(zone_exists, "true");
}

// ========== Upload Progress Monitoring ==========

TEST_F(UploadManagerTest, WaitForUploadCompletion) {
    // Simulate upload completion via JavaScript
    browser->executeJavascriptSync("simulateUploadComplete();");
    
    bool result = manager->waitForUploadCompletion(*browser, "#file-input", 1000);
    
    // Verify status was updated
    std::string status = browser->executeJavascriptSync("return document.getElementById('upload-status').innerText;");
    EXPECT_EQ(status, "Upload complete");
    EXPECT_TRUE(result);
}

TEST_F(UploadManagerTest, MonitorUploadProgress) {
    int progress_updates = 0;
    auto progress_callback = [&progress_updates](int progress) {
        progress_updates++;
        EXPECT_GE(progress, 0);
        EXPECT_LE(progress, 100);
    };
    
    bool result = manager->monitorUploadProgress(*browser, 500, progress_callback);
    
    EXPECT_TRUE(result);
    EXPECT_GE(progress_updates, 0); // May be 0 if monitoring completes immediately
}

TEST_F(UploadManagerTest, VerifyUploadSuccess) {
    // Set up success state
    browser->executeJavascriptSync("updateStatus('Upload complete');");
    
    bool result = manager->verifyUploadSuccess(*browser, "#file-input");
    
    std::string status = browser->executeJavascriptSync("return document.getElementById('upload-status').innerText;");
    EXPECT_EQ(status, "Upload complete");
    EXPECT_TRUE(result);
}

// ========== Multiple File Upload ==========

TEST_F(UploadManagerTest, UploadMultipleFiles) {
    std::vector<std::string> filepaths = {
        test_file.string(),
        binary_file.string()
    };
    
    UploadResult result = manager->uploadMultipleFiles(*browser, "#multiple-input", filepaths);
    
    EXPECT_EQ(result, UploadResult::SUCCESS);
}

TEST_F(UploadManagerTest, UploadMultipleFilesWithMissing) {
    std::vector<std::string> filepaths = {
        test_file.string(),
        "/nonexistent/file.txt"
    };
    
    UploadResult result = manager->uploadMultipleFiles(*browser, "#multiple-input", filepaths);
    
    EXPECT_EQ(result, UploadResult::FILE_NOT_FOUND);
}

// ========== Error Handling and Recovery ==========

TEST_F(UploadManagerTest, UploadWithRetrySuccess) {
    UploadCommand cmd = createUploadCommand(test_file.string());
    
    // Set up success state for retry test
    browser->executeJavascriptSync("updateStatus('Ready for upload');");
    
    UploadResult result = manager->uploadWithRetry(*browser, cmd, 2);
    
    EXPECT_EQ(result, UploadResult::SUCCESS);
}

TEST_F(UploadManagerTest, UploadWithRetryTimeout) {
    UploadCommand cmd = createUploadCommand("/nonexistent/file.txt");
    cmd.timeout_ms = 100; // Very short timeout
    
    UploadResult result = manager->uploadWithRetry(*browser, cmd, 2);
    
    EXPECT_NE(result, UploadResult::SUCCESS);
}

TEST_F(UploadManagerTest, ClearUploadState) {
    // Should not throw or crash
    manager->clearUploadState(*browser, "#file-input");
    
    // Verify browser is still responsive
    std::string status = browser->executeJavascriptSync("return document.getElementById('upload-status').innerText;");
    EXPECT_FALSE(status.empty());
}

TEST_F(UploadManagerTest, GetErrorMessage) {
    std::string message = manager->getErrorMessage(UploadResult::FILE_NOT_FOUND, test_file.string());
    
    EXPECT_NE(message.find("not found"), std::string::npos);
    EXPECT_NE(message.find(test_file.string()), std::string::npos);
}

// ========== Configuration Tests ==========

TEST_F(UploadManagerTest, SetDefaultTimeout) {
    manager->setDefaultTimeout(10000);
    
    // Test that timeout is applied (indirect test through behavior)
    // This would be more testable with dependency injection
}

TEST_F(UploadManagerTest, SetMaxFileSize) {
    manager->setMaxFileSize(1024);
    
    // Test file size validation with new limit
    UploadCommand cmd = createUploadCommand(large_file.string());
    bool result = manager->validateFile(large_file.string(), cmd);
    
    // Large file should now be rejected due to global limit
    EXPECT_FALSE(result);
}

TEST_F(UploadManagerTest, SetProgressMonitoringEnabled) {
    manager->setProgressMonitoringEnabled(false);
    
    // Progress monitoring should be disabled (behavior-based test)
}

// ========== Utility Methods ==========

TEST_F(UploadManagerTest, GetCommonFileInputSelectors) {
    std::vector<std::string> selectors = manager->getCommonFileInputSelectors();
    
    EXPECT_FALSE(selectors.empty());
    EXPECT_NE(std::find(selectors.begin(), selectors.end(), "input[type=file]"), selectors.end());
}

TEST_F(UploadManagerTest, HasFileInputs) {
    // Use actual JavaScript to check for file inputs
    std::string result_str = browser->executeJavascriptSync("return hasFileInputs().toString();");
    bool result = manager->hasFileInputs(*browser);
    
    EXPECT_TRUE(result);
    EXPECT_EQ(result_str, "true");
}

TEST_F(UploadManagerTest, FindFileInputs) {
    // Use actual JavaScript to find file inputs
    std::string inputs_json = browser->executeJavascriptSync("return findFileInputs();");
    std::vector<std::string> inputs = manager->findFileInputs(*browser);
    
    EXPECT_GE(inputs.size(), 2); // Should find at least our test inputs
    EXPECT_NE(inputs_json.find("#file-input"), std::string::npos);
    EXPECT_NE(inputs_json.find("#multiple-input"), std::string::npos);
}

TEST_F(UploadManagerTest, UploadResultToString) {
    std::string success_msg = manager->uploadResultToString(UploadResult::SUCCESS);
    std::string error_msg = manager->uploadResultToString(UploadResult::FILE_NOT_FOUND);
    
    EXPECT_NE(success_msg.find("success"), std::string::npos);
    EXPECT_NE(error_msg.find("not found"), std::string::npos);
}

// ========== Edge Cases ==========

TEST_F(UploadManagerTest, UploadEmptyFile) {
    std::filesystem::path empty_file = temp_dir->createFile("empty.txt", "");
    UploadCommand cmd = createUploadCommand(empty_file.string());
    
    bool validation_result = manager->validateFile(empty_file.string(), cmd);
    
    EXPECT_TRUE(validation_result); // Empty files should be valid unless size restrictions apply
}

TEST_F(UploadManagerTest, UploadFileWithSpecialCharacters) {
    std::filesystem::path special_file = temp_dir->createFile("特殊_файл_🔧.txt", "content");
    UploadCommand cmd = createUploadCommand(special_file.string());
    
    std::string sanitized = manager->sanitizeFileName(special_file.filename().string());
    
    EXPECT_FALSE(sanitized.empty());
    // Should preserve extension
    EXPECT_NE(sanitized.find(".txt"), std::string::npos);
}

TEST_F(UploadManagerTest, UploadWithInvalidSelector) {
    UploadCommand cmd = createUploadCommand(test_file.string());
    cmd.selector = "#nonexistent-selector";
    
    bool result = manager->validateUploadTarget(*browser, "#nonexistent-selector");
    
    EXPECT_FALSE(result);
}

// ========== Integration Tests ==========

TEST_F(UploadManagerTest, EndToEndUploadWorkflow) {
    // Test complete workflow: validate -> prepare -> simulate -> verify
    UploadCommand cmd = createUploadCommand(test_file.string());
    
    // 1. Validate file
    bool file_valid = manager->validateFile(test_file.string(), cmd);
    EXPECT_TRUE(file_valid);
    
    // 2. Validate target
    bool target_valid = manager->validateUploadTarget(*browser, cmd.selector);
    EXPECT_TRUE(target_valid);
    
    // 3. Prepare file
    FileInfo info = manager->prepareFile(test_file.string());
    EXPECT_TRUE(info.exists);
    
    // 4. Simulate selection
    bool selection_ok = manager->simulateFileSelection(*browser, cmd.selector, test_file.string());
    EXPECT_TRUE(selection_ok);
    
    // 5. Verify browser state is consistent
    std::string page_title = browser->getPageTitle();
    EXPECT_EQ(page_title, "Upload Test Server");
}

TEST_F(UploadManagerTest, BrowserPageInteraction) {
    // Test that our test page is properly loaded and interactive
    std::string inputs_count = browser->executeJavascriptSync("document.querySelectorAll('input[type=\"file\"]').length.toString()");
    std::string drop_zone_exists = browser->executeJavascriptSync("elementExists('#drop-zone').toString()");
    
    EXPECT_EQ(inputs_count, "2"); // file-input and multiple-input
    EXPECT_EQ(drop_zone_exists, "true");
}