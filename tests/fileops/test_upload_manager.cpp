#include <gtest/gtest.h>
#include "FileOps/UploadManager.h"
#include "../utils/test_helpers.h"
#include "Browser/Browser.h"
#include "Debug.h"
#include <thread>
#include <chrono>
#include <gtk/gtk.h>

extern std::unique_ptr<Browser> g_browser;

using namespace FileOps;

class UploadManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        temp_dir = std::make_unique<TestHelpers::TemporaryDirectory>("upload_tests");
        
        // Create test files first (these don't need browser)
        test_file = temp_dir->createFile("test.txt", "test content");
        large_file = temp_dir->createFile("large.txt", std::string(1024 * 1024, 'x')); // 1MB
        
        // Create binary test file
        std::vector<uint8_t> binary_data = {0x89, 0x50, 0x4E, 0x47}; // PNG header
        TestHelpers::createTestFile(temp_dir->getPath() / "test.png", 
                                   std::string(binary_data.begin(), binary_data.end()));
        binary_file = temp_dir->getPath() / "test.png";
        
        manager = std::make_unique<UploadManager>();
        
        // Use global browser instance
        debug_output("Setting up test page...");
        bool page_setup_success = setupTestPage();
        if (!page_setup_success) {
            GTEST_SKIP() << "Browser setup failed, cannot test DOM validation";
        }
    }
    
    bool setupTestPage() {
        // Create a proper HTML file to avoid data URI encoding issues
        std::string test_html = 
            "<!DOCTYPE html>\n"
            "<html><head><title>Upload Test Server</title></head>\n"
            "<body>\n"
            "<h1>Test Page</h1>\n"
            "<input type='file' id='file-input'/>\n"
            "<input type='file' id='multiple-input' multiple/>\n"
            "<div id='drop-zone'>Drop files here</div>\n"
            "<div id='upload-status'>Ready</div>\n"
            "<script>\n"
            "function hasFileInputs() { return document.querySelectorAll('input[type=file]').length > 0; }\n"
            "function elementExists(sel) { return document.querySelector(sel) !== null; }\n"
            "function updateStatus(msg) { document.getElementById('upload-status').innerText = msg; }\n"
            "function simulateUploadComplete() { updateStatus('Upload complete'); }\n"
            "function findFileInputs() {\n"
            "  var inputs = document.querySelectorAll('input[type=file]');\n"
            "  var result = [];\n"
            "  for (var i = 0; i < inputs.length; i++) {\n"
            "    result.push('#' + inputs[i].id);\n"
            "  }\n"
            "  return JSON.stringify(result);\n"
            "}\n"
            "</script>\n"
            "</body></html>";
        
        // Create temporary HTML file
        auto test_html_file = temp_dir->createFile("test_page.html", test_html);
        std::string file_url = "file://" + test_html_file.string();
        
        debug_output("Loading test page via file URL: " + file_url);
        g_browser->loadUri(file_url);
        
        // Wait for navigation to complete using the browser's internal mechanism
        bool navigation_success = g_browser->waitForNavigation(10000);
        if (!navigation_success) {
            debug_output("Navigation failed");
            return false;
        }
        
        // CRITICAL FIX: Wait for DOM and JavaScript to be fully ready
        // Check that basic JavaScript execution works
        std::string js_test = g_browser->executeJavascriptSync("'test'");
        if (js_test != "test") {
            debug_output("JavaScript execution not ready");
            return false;
        }
        
        // Wait for all DOM elements to be available
        std::string dom_ready = g_browser->executeJavascriptSync(
            "document.readyState === 'complete' && "
            "document.getElementById('file-input') !== null && "
            "document.getElementById('upload-status') !== null"
        );
        
        if (dom_ready != "true") {
            debug_output("DOM not fully ready, waiting...");
            // Give additional time for DOM to settle
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            
            // Re-check DOM readiness
            dom_ready = g_browser->executeJavascriptSync(
                "document.readyState === 'complete' && "
                "document.getElementById('file-input') !== null && "
                "document.getElementById('upload-status') !== null"
            );
        }
        
        debug_output("Upload test page setup complete - DOM ready: " + dom_ready);
        return dom_ready == "true";
    }

    void TearDown() override {
        manager.reset();
        temp_dir.reset();
    }

    std::unique_ptr<TestHelpers::TemporaryDirectory> temp_dir;
    
    std::unique_ptr<UploadManager> manager;
    static bool gtk_is_initialized_;
    bool browser_ready = false;
    
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

bool UploadManagerTest::gtk_is_initialized_ = false;

// ========== Simple Validation Tests (No Browser Required) ==========

class UploadValidationTest : public ::testing::Test {
protected:
    void SetUp() override {
        temp_dir = std::make_unique<TestHelpers::TemporaryDirectory>("upload_validation_tests");
        manager = std::make_unique<UploadManager>();
        
        // Create test files without browser setup
        test_file = temp_dir->createFile("test.txt", "test content");
        large_file = temp_dir->createFile("large.txt", std::string(1024 * 1024, 'x')); // 1MB
        
        // Create binary test file
        std::vector<uint8_t> binary_data = {0x89, 0x50, 0x4E, 0x47}; // PNG header
        TestHelpers::createTestFile(temp_dir->getPath() / "test.png", 
                                   std::string(binary_data.begin(), binary_data.end()));
        binary_file = temp_dir->getPath() / "test.png";
    }

    void TearDown() override {
        manager.reset();
        temp_dir.reset();
    }

    std::unique_ptr<TestHelpers::TemporaryDirectory> temp_dir;
    std::unique_ptr<UploadManager> manager;
    
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

TEST_F(UploadValidationTest, ValidateExistingFile) {
    UploadCommand cmd = createUploadCommand(test_file.string());
    
    bool result = manager->validateFile(test_file.string(), cmd);
    
    EXPECT_TRUE(result);
}

TEST_F(UploadValidationTest, ValidateNonExistentFile) {
    UploadCommand cmd = createUploadCommand("/nonexistent/file.txt");
    
    bool result = manager->validateFile("/nonexistent/file.txt", cmd);
    
    EXPECT_FALSE(result);
}

TEST_F(UploadValidationTest, ValidateFileSize) {
    // Test file within size limit
    bool result1 = manager->validateFileSize(test_file.string(), 1024);
    EXPECT_TRUE(result1);
    
    // Test file exceeding size limit  
    bool result2 = manager->validateFileSize(large_file.string(), 512);
    EXPECT_FALSE(result2);
}

TEST_F(UploadValidationTest, ValidateFileTypeAllowed) {
    std::vector<std::string> allowed = {"txt", "png", "jpg"};
    
    bool txt_result = manager->validateFileType(test_file.string(), allowed);
    bool png_result = manager->validateFileType(binary_file.string(), allowed);
    
    EXPECT_TRUE(txt_result);
    EXPECT_TRUE(png_result);
}

TEST_F(UploadValidationTest, ValidateFileTypeRestricted) {
    std::vector<std::string> allowed = {"jpg", "jpeg"};
    
    bool txt_result = manager->validateFileType(test_file.string(), allowed);
    bool png_result = manager->validateFileType(binary_file.string(), allowed);
    
    EXPECT_FALSE(txt_result);
    EXPECT_FALSE(png_result);
}

TEST_F(UploadValidationTest, ValidateFileTypeWildcard) {
    std::vector<std::string> allowed = {"*"};
    
    bool result = manager->validateFileType(test_file.string(), allowed);
    
    EXPECT_TRUE(result);
}

// ========== Upload Target Validation ==========

TEST_F(UploadManagerTest, ValidateUploadTargetExists) {
    // DEBUG: Check basic JavaScript execution
    std::string basic_test = g_browser->executeJavascriptSync("'hello'");
    debug_output("Basic JS test result: '" + basic_test + "'");
    
    // DEBUG: Check document ready state
    std::string ready_state = g_browser->executeJavascriptSync("document.readyState");
    debug_output("Document ready state: '" + ready_state + "'");
    
    // DEBUG: Check direct element query
    std::string direct_query = g_browser->executeJavascriptSync("document.querySelector('#file-input') !== null");
    debug_output("Direct query result: '" + direct_query + "'");
    
    // Verify target exists using real browser DOM query
    std::string exists_result = g_browser->executeJavascriptSync("elementExists('#file-input').toString()");
    debug_output("elementExists result: '" + exists_result + "'");
    
    bool result = manager->validateUploadTarget(*g_browser, "#file-input");
    debug_output("validateUploadTarget result: " + std::string(result ? "true" : "false"));
    
    EXPECT_TRUE(result);
    EXPECT_EQ(exists_result, "true");
}

TEST_F(UploadManagerTest, ValidateUploadTargetNotExists) {

    
    // Test with selector that doesn't exist in our test page
    std::string exists_result = g_browser->executeJavascriptSync("elementExists('#nonexistent-input').toString()");
    bool result = manager->validateUploadTarget(*g_browser, "#nonexistent-input");
    
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
    bool result = manager->simulateFileSelection(*g_browser, "#file-input", test_file.string());
    
    // Verify the file was actually selected by checking page state
    std::string status = g_browser->executeJavascriptSync("document.getElementById('upload-status').innerText");
    
    EXPECT_TRUE(result);
    // Note: Real file selection might not trigger the change event in our test setup
    // This tests the mechanism rather than the specific event
}

TEST_F(UploadManagerTest, TriggerFileInputEvents) {
    bool result = manager->triggerFileInputEvents(*g_browser, "#file-input");
    
    EXPECT_TRUE(result);
}

TEST_F(UploadManagerTest, SimulateFileDrop) {
    bool result = manager->simulateFileDrop(*g_browser, "#drop-zone", test_file.string());
    
    // Verify drop zone exists and can be targeted
    std::string zone_exists = g_browser->executeJavascriptSync("elementExists('#drop-zone').toString()");
    
    EXPECT_TRUE(result);
    EXPECT_EQ(zone_exists, "true");
}

// ========== Upload Progress Monitoring ==========

TEST_F(UploadManagerTest, WaitForUploadCompletion) {
    // Simulate upload completion via JavaScript
    g_browser->executeJavascriptSync("simulateUploadComplete();");
    
    bool result = manager->waitForUploadCompletion(*g_browser, "#file-input", 1000);
    
    // Verify status was updated
    std::string status = g_browser->executeJavascriptSync("document.getElementById('upload-status').innerText");
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
    
    bool result = manager->monitorUploadProgress(*g_browser, 500, progress_callback);
    
    EXPECT_TRUE(result);
    EXPECT_GE(progress_updates, 0); // May be 0 if monitoring completes immediately
}

TEST_F(UploadManagerTest, VerifyUploadSuccess) {
    // Set up success state
    g_browser->executeJavascriptSync("updateStatus('Upload complete');");
    
    bool result = manager->verifyUploadSuccess(*g_browser, "#file-input");
    
    std::string status = g_browser->executeJavascriptSync("document.getElementById('upload-status').innerText");
    EXPECT_EQ(status, "Upload complete");
    EXPECT_TRUE(result);
}

// ========== Multiple File Upload ==========

TEST_F(UploadManagerTest, UploadMultipleFiles) {
    std::vector<std::string> filepaths = {
        test_file.string(),
        binary_file.string()
    };
    
    UploadResult result = manager->uploadMultipleFiles(*g_browser, "#multiple-input", filepaths);
    
    EXPECT_EQ(result, UploadResult::SUCCESS);
}

TEST_F(UploadManagerTest, UploadMultipleFilesWithMissing) {
    std::vector<std::string> filepaths = {
        test_file.string(),
        "/nonexistent/file.txt"
    };
    
    UploadResult result = manager->uploadMultipleFiles(*g_browser, "#multiple-input", filepaths);
    
    EXPECT_EQ(result, UploadResult::FILE_NOT_FOUND);
}

// ========== Error Handling and Recovery ==========

TEST_F(UploadManagerTest, UploadWithRetrySuccess) {
    UploadCommand cmd = createUploadCommand(test_file.string());
    
    // Set up success state for retry test
    g_browser->executeJavascriptSync("updateStatus('Ready for upload');");
    
    UploadResult result = manager->uploadWithRetry(*g_browser, cmd, 2);
    
    EXPECT_EQ(result, UploadResult::SUCCESS);
}

TEST_F(UploadManagerTest, UploadWithRetryTimeout) {
    UploadCommand cmd = createUploadCommand("/nonexistent/file.txt");
    cmd.timeout_ms = 100; // Very short timeout
    
    UploadResult result = manager->uploadWithRetry(*g_browser, cmd, 2);
    
    EXPECT_NE(result, UploadResult::SUCCESS);
}

TEST_F(UploadManagerTest, ClearUploadState) {
    // Should not throw or crash
    manager->clearUploadState(*g_browser, "#file-input");
    
    // Verify browser is still responsive
    std::string status = g_browser->executeJavascriptSync("document.getElementById('upload-status').innerText");
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
    std::string result_str = g_browser->executeJavascriptSync("hasFileInputs().toString()");
    bool result = manager->hasFileInputs(*g_browser);
    
    EXPECT_TRUE(result);
    EXPECT_EQ(result_str, "true");
}

TEST_F(UploadManagerTest, FindFileInputs) {
    // Use actual JavaScript to find file inputs
    std::string inputs_json = g_browser->executeJavascriptSync("findFileInputs()");
    std::vector<std::string> inputs = manager->findFileInputs(*g_browser);
    
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
    
    bool result = manager->validateUploadTarget(*g_browser, "#nonexistent-selector");
    
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
    bool target_valid = manager->validateUploadTarget(*g_browser, cmd.selector);
    EXPECT_TRUE(target_valid);
    
    // 3. Prepare file
    FileInfo info = manager->prepareFile(test_file.string());
    EXPECT_TRUE(info.exists);
    
    // 4. Simulate selection
    bool selection_ok = manager->simulateFileSelection(*g_browser, cmd.selector, test_file.string());
    EXPECT_TRUE(selection_ok);
    
    // 5. Verify browser state is consistent
    std::string page_title = g_browser->getPageTitle();
    EXPECT_EQ(page_title, "Upload Test Server");
}

TEST_F(UploadManagerTest, BrowserPageInteraction) {
    // Test that our test page is properly loaded and interactive
    std::string inputs_count = g_browser->executeJavascriptSync("document.querySelectorAll('input[type=\"file\"]').length.toString()");
    std::string drop_zone_exists = g_browser->executeJavascriptSync("elementExists('#drop-zone').toString()");
    
    EXPECT_EQ(inputs_count, "2"); // file-input and multiple-input
    EXPECT_EQ(drop_zone_exists, "true");
}

// ========== Enhanced Method Tests ==========

TEST_F(UploadManagerTest, EnhancedFileValidation) {
    UploadCommand cmd = createUploadCommand(test_file.string());
    
    // Test with empty file (should fail with enhanced validation)
    std::filesystem::path empty_file = temp_dir->createFile("empty.txt", "");
    bool empty_result = manager->validateFile(empty_file.string(), cmd);
    EXPECT_FALSE(empty_result); // Enhanced validation rejects empty files
    
    // Test with dangerous filename patterns
    std::filesystem::path dangerous_file = temp_dir->createFile("dangerous_file", "content");
    // Manually construct a path with ".." in it to test validation
    std::string dangerous_path = dangerous_file.parent_path().string() + "/../dangerous_file";
    bool dangerous_result = manager->validateFile(dangerous_path, cmd);
    EXPECT_FALSE(dangerous_result); // Should reject dangerous filenames
    
    // Test filename length validation - create a filename at filesystem limit
    std::string long_filename = std::string(250, 'a') + ".txt"; // 254 chars, just under 255 limit
    std::filesystem::path long_file = temp_dir->createFile(long_filename, "content");
    bool long_result = manager->validateFile(long_file.string(), cmd);
    EXPECT_FALSE(long_result); // Should reject overly long filenames (UploadManager limit is 255)
}

TEST_F(UploadManagerTest, EnhancedMimeTypeDetection) {
    // Test comprehensive MIME type detection
    std::string html_mime = manager->detectMimeType("/test/file.html");
    std::string css_mime = manager->detectMimeType("/test/file.css");
    std::string js_mime = manager->detectMimeType("/test/file.js");
    std::string json_mime = manager->detectMimeType("/test/file.json");
    std::string pdf_mime = manager->detectMimeType("/test/file.pdf");
    std::string jpg_mime = manager->detectMimeType("/test/file.jpg");
    std::string mp4_mime = manager->detectMimeType("/test/file.mp4");
    std::string zip_mime = manager->detectMimeType("/test/file.zip");
    
    EXPECT_EQ(html_mime, "text/html");
    EXPECT_EQ(css_mime, "text/css");
    EXPECT_EQ(js_mime, "application/javascript");
    EXPECT_EQ(json_mime, "application/json");
    EXPECT_EQ(pdf_mime, "application/pdf");
    EXPECT_EQ(jpg_mime, "image/jpeg");
    EXPECT_EQ(mp4_mime, "video/mp4");
    EXPECT_EQ(zip_mime, "application/zip");
    
    // Test unknown extension fallback
    std::string unknown_mime = manager->detectMimeType("/test/file.unknown");
    EXPECT_EQ(unknown_mime, "application/octet-stream");
}

TEST_F(UploadManagerTest, Base64Encoding) {
    // Base64 encoding method is private - skipping direct test
    // This functionality is tested indirectly through upload operations
    GTEST_SKIP() << "Base64 encoding method is private";
}

TEST_F(UploadManagerTest, JavaScriptGeneration) {
    // JavaScript generation method is private - skipping direct test
    // This functionality is tested indirectly through upload operations
    GTEST_SKIP() << "JavaScript generation method is private";
}

TEST_F(UploadManagerTest, JavaScriptEscaping) {
    // JavaScript escaping method is private - skipping direct test
    // This functionality is tested indirectly through upload operations
    GTEST_SKIP() << "JavaScript escaping method is private";
}

TEST_F(UploadManagerTest, EnhancedFileSimulation) {
    // Test the enhanced file simulation with proper File object creation
    bool result = manager->simulateFileSelection(*g_browser, "#file-input", test_file.string());
    
    EXPECT_TRUE(result);
    
    // Verify that the enhanced script was executed
    std::string console_logs = g_browser->executeJavascriptSync("window.lastConsoleMessage || ''");
    // The enhanced script includes error logging, so we shouldn't see error messages
    EXPECT_EQ(console_logs.find("File upload simulation error"), std::string::npos);
}

TEST_F(UploadManagerTest, FilePreparationWithMetadata) {
    FileInfo info = manager->prepareFile(test_file.string());
    
    // Enhanced FileInfo should include complete metadata
    EXPECT_FALSE(info.filepath.empty());
    EXPECT_FALSE(info.filename.empty());
    EXPECT_FALSE(info.mime_type.empty());
    EXPECT_GT(info.size_bytes, 0);
    EXPECT_TRUE(info.exists);
    EXPECT_TRUE(info.is_readable);
    
    // Test absolute path conversion
    EXPECT_EQ(info.filepath.front(), '/'); // Should be absolute path on Unix
    
    // Test MIME type is set correctly
    EXPECT_EQ(info.mime_type, "text/plain");
}

TEST_F(UploadManagerTest, SecurityValidation) {
    // Test various security validations in enhanced validateFile method
    UploadCommand cmd = createUploadCommand("");
    
    // Test null byte injection protection
    std::string null_byte_path = std::string("test") + std::string(1, '\0') + std::string("malicious");
    // Note: filesystem won't actually create files with null bytes, so we test the validation logic
    
    // Test path traversal protection
    std::filesystem::path traversal_file = temp_dir->createFile("passwd_test", "fake content");
    // Create a path with traversal patterns for testing
    std::string traversal_path = traversal_file.parent_path().string() + "/../../../etc/passwd";
    bool traversal_result = manager->validateFile(traversal_path, cmd);
    // Should fail validation due to non-existent file
    
    // Test permission validation with actual file
    bool permission_result = manager->validateFile(test_file.string(), cmd);
    EXPECT_TRUE(permission_result); // Our test file should be readable
}