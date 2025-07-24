#include <gtest/gtest.h>
#include "Browser/Browser.h"
#include "Session/Session.h"
#include "FileOps/UploadManager.h"
#include "FileOps/DownloadManager.h"
#include "../utils/test_helpers.h"
#include <thread>
#include <chrono>
#include <filesystem>
#include <fstream>

using namespace std::chrono_literals;
using namespace FileOps;

class BrowserFileOpsIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        HWeb::HWebConfig test_config;
        browser = std::make_unique<Browser>(test_config);
        session = std::make_unique<Session>("integration_test_session");
        
        // Create test files directory
        test_dir = std::filesystem::temp_directory_path() / "hweb_integration_tests";
        std::filesystem::create_directories(test_dir);
        
        // Create test files
        createTestFiles();
        
        // Load a test page with file operations
        setupFileOpsTestPage();
        
        // Wait for page to be ready
        std::this_thread::sleep_for(800ms);
    }
    
    void createTestFiles() {
        // Create a test text file
        test_file = test_dir / "test_file.txt";
        std::ofstream file(test_file);
        file << "This is a test file for upload testing.\n";
        file << "It contains multiple lines of text.\n";
        file << "Line 3 with some data: 123456\n";
        file.close();
        
        // Create a test image file (minimal PNG)
        test_image = test_dir / "test_image.png";
        std::ofstream img(test_image, std::ios::binary);
        // Minimal PNG header
        const unsigned char png_header[] = {
            0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A,
            0x00, 0x00, 0x00, 0x0D, 0x49, 0x48, 0x44, 0x52,
            0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01,
            0x08, 0x06, 0x00, 0x00, 0x00, 0x1F, 0x15, 0xC4,
            0x89, 0x00, 0x00, 0x00, 0x0A, 0x49, 0x44, 0x41,
            0x54, 0x78, 0x9C, 0x63, 0x00, 0x01, 0x00, 0x00,
            0x05, 0x00, 0x01, 0x0D, 0x0A, 0x2D, 0xB4, 0x00,
            0x00, 0x00, 0x00, 0x49, 0x45, 0x4E, 0x44, 0xAE,
            0x42, 0x60, 0x82
        };
        img.write(reinterpret_cast<const char*>(png_header), sizeof(png_header));
        img.close();
        
        // Create a test download directory
        download_dir = test_dir / "downloads";
        std::filesystem::create_directories(download_dir);
    }
    
    void setupFileOpsTestPage() {
        std::string test_html = R"HTML(
<!DOCTYPE html>
<html>
<head><title>FileOps Test</title></head>
<body>
    <h1>FileOps Integration</h1>
    <form>
        <input type="file" id="file-upload" />
        <input type="file" id="multiple-upload" multiple />
        <button type="button" id="upload-btn">Upload</button>
        <div id="upload-status">Ready</div>
    </form>
    <a href="#" id="download-link">Download</a>
    <div id="download-status">Ready</div>
    <div id="selected-files">None</div>
    <div id="upload-progress">0%</div>
    <div id="last-action">none</div>
    <script>
        function saveState() { window._hweb_fileops_state = '{"test":"state"}'; }
        function loadState() { /* load logic */ }
    </script>
</body>
</html>
)HTML";
        
        // Create data URL for the test page
        std::string data_url = "data:text/html;charset=utf-8," + test_html;
        browser->loadUri(data_url);
        
        // Wait for page load
        std::this_thread::sleep_for(1000ms);
    }

    void TearDown() override {
        // Clean up test files
        if (std::filesystem::exists(test_dir)) {
            std::filesystem::remove_all(test_dir);
        }
        
        browser.reset();
        session.reset();
    }

    std::unique_ptr<Browser> browser;
    std::unique_ptr<Session> session;
    std::filesystem::path test_dir;
    std::filesystem::path test_file;
    std::filesystem::path test_image;
    std::filesystem::path download_dir;
};

// ========== Browser-Session Integration with File Operations ==========

TEST_F(BrowserFileOpsIntegrationTest, SessionPersistsFileUploadState) {
    // Simulate file selection and upload
    browser->executeJavascriptSync("document.getElementById('file-upload').value = 'test_file.txt';");
    browser->executeJavascriptSync("simulateUpload();");
    std::this_thread::sleep_for(600ms); // Wait for upload simulation
    
    // Save state to session
    browser->executeJavascriptSync("saveState();");
    
    // Update session with current browser state
    browser->updateSessionState(*session);
    
    // Verify session captured the file operation state
    EXPECT_FALSE(session->getCurrentUrl().empty());
    EXPECT_EQ(session->getDocumentReadyState(), "complete");
    
    // Check if custom state was captured
    auto extractors = std::map<std::string, std::string>{
        {"fileopsState", "window._hweb_fileops_state"}
    };
    Json::Value customState = browser->extractCustomState(extractors);
    
    EXPECT_TRUE(customState.isMember("fileopsState"));
    
    // Parse the file ops state
    std::string stateStr = customState["fileopsState"].asString();
    EXPECT_FALSE(stateStr.empty());
    EXPECT_NE(stateStr.find("upload_complete"), std::string::npos);
}

TEST_F(BrowserFileOpsIntegrationTest, SessionRestoresFileOperationState) {
    // Set up initial file operation state
    browser->executeJavascriptSync(R"(
        uploadState = {
            selectedFiles: ['restored_file.txt'],
            uploadProgress: 75,
            lastAction: 'upload_in_progress'
        };
        saveState();
        updateDisplay();
    )");
    
    // Update session with this state
    browser->updateSessionState(*session);
    
    // Create new browser instance and restore session
            HWeb::HWebConfig test_config;
        auto newBrowser = std::make_unique<Browser>(test_config);
    std::this_thread::sleep_for(500ms);
    
    newBrowser->restoreSession(*session);
    std::this_thread::sleep_for(800ms);
    
    // Load the state and verify restoration
    newBrowser->executeJavascriptSync("loadState();");
    std::this_thread::sleep_for(200ms);
    
    std::string selectedFiles = newBrowser->getInnerText("#selected-files");
    std::string uploadProgress = newBrowser->getInnerText("#upload-progress");
    std::string lastAction = newBrowser->getInnerText("#last-action");
    
    EXPECT_EQ(selectedFiles, "restored_file.txt");
    EXPECT_EQ(uploadProgress, "75%");
    EXPECT_EQ(lastAction, "upload_in_progress");
}

TEST_F(BrowserFileOpsIntegrationTest, FileUploadFormStateIntegration) {
    // Interact with file upload form
    browser->clickElement("#upload-btn");
    browser->executeJavascriptSync("document.getElementById('file-upload').value = 'integration_test.txt';");
    browser->fillInput("#upload-status", "Manual status update");
    
    std::this_thread::sleep_for(200ms);
    
    // Extract form state through session
    auto formFields = browser->extractFormState();
    
    EXPECT_GT(formFields.size(), 0);
    
    // Find the file input field
    bool foundFileInput = false;
    for (const auto& field : formFields) {
        if (field.selector == "#file-upload") {
            foundFileInput = true;
            // File inputs may not preserve the value for security reasons
            EXPECT_EQ(field.type, "file");
            break;
        }
    }
    
    EXPECT_TRUE(foundFileInput);
}

TEST_F(BrowserFileOpsIntegrationTest, DownloadOperationWithSessionTracking) {
    // Simulate download operation
    browser->clickElement("#download-link");
    std::this_thread::sleep_for(600ms);
    
    // Check download status
    std::string downloadStatus = browser->getInnerText("#download-status");
    EXPECT_NE(downloadStatus.find("test_file.txt"), std::string::npos);
    
    // Save state after download
    browser->executeJavascriptSync("saveState();");
    
    // Update session
    browser->updateSessionState(*session);
    
    // Verify the download action was recorded in custom state
    auto extractors = std::map<std::string, std::string>{
        {"fileopsState", "window._hweb_fileops_state"},
        {"downloadStatus", "document.getElementById('download-status').textContent"}
    };
    Json::Value customState = browser->extractCustomState(extractors);
    
    EXPECT_TRUE(customState.isMember("downloadStatus"));
    std::string status = customState["downloadStatus"].asString();
    EXPECT_NE(status.find("test_file.txt"), std::string::npos);
}

// ========== Upload Manager Integration Tests ==========

TEST_F(BrowserFileOpsIntegrationTest, UploadManagerWithBrowserIntegration) {
    UploadManager uploadManager;
    
    // Create upload command
    UploadCommand cmd;
    cmd.filepath = test_file.string();
    cmd.selector = "#file-upload";
    cmd.max_file_size = 1024 * 1024; // 1MB
    cmd.allowed_types = {".txt", ".png", ".jpg"};
    cmd.timeout_ms = 5000;
    
    // Verify upload target exists in browser
    EXPECT_TRUE(browser->elementExists("#file-upload"));
    
    // Test file validation
    EXPECT_TRUE(std::filesystem::exists(test_file));
    EXPECT_TRUE(uploadManager.validateFile(test_file.string(), cmd));
    EXPECT_TRUE(uploadManager.validateUploadTarget(*browser, "#file-upload"));
    
    // Test upload result (this tests integration but won't actually upload since it's a data URL)
    UploadResult result = uploadManager.uploadFile(*browser, cmd);
    
    // The result depends on the actual upload implementation
    // For a data URL page, we expect it to handle gracefully
    EXPECT_NE(result, UploadResult::FILE_NOT_FOUND);
    EXPECT_NE(result, UploadResult::ELEMENT_NOT_FOUND);
}

TEST_F(BrowserFileOpsIntegrationTest, UploadManagerValidationIntegration) {
    UploadManager uploadManager;
    
    // Test with non-existent file
    UploadCommand invalidCmd;
    invalidCmd.filepath = "/nonexistent/file.txt";
    invalidCmd.selector = "#file-upload";
    
    UploadResult result = uploadManager.uploadFile(*browser, invalidCmd);
    EXPECT_EQ(result, UploadResult::FILE_NOT_FOUND);
    
    // Test with non-existent element
    UploadCommand invalidElementCmd;
    invalidElementCmd.filepath = test_file.string();
    invalidElementCmd.selector = "#nonexistent-upload";
    
    result = uploadManager.uploadFile(*browser, invalidElementCmd);
    EXPECT_EQ(result, UploadResult::ELEMENT_NOT_FOUND);
    
    // Test file type validation
    UploadCommand restrictedCmd;
    restrictedCmd.filepath = test_file.string();
    restrictedCmd.selector = "#file-upload";
    restrictedCmd.allowed_types = {".pdf", ".doc"}; // Restrict to types our test file doesn't match
    
    EXPECT_FALSE(uploadManager.validateFileType(test_file.string(), restrictedCmd.allowed_types));
}

TEST_F(BrowserFileOpsIntegrationTest, MultipleFileUploadIntegration) {
    UploadManager uploadManager;
    
    // Create multiple upload command
    std::vector<std::string> files = {test_file.string(), test_image.string()};
    
    UploadCommand cmd;
    cmd.selector = "#multiple-upload";
    cmd.max_file_size = 1024 * 1024;
    cmd.allowed_types = {".txt", ".png", ".jpg"};
    cmd.timeout_ms = 10000;
    
    // Verify upload target exists
    EXPECT_TRUE(browser->elementExists("#multiple-upload"));
    
    // Test multiple file validation
    for (const auto& file : files) {
        EXPECT_TRUE(std::filesystem::exists(file));
        EXPECT_TRUE(uploadManager.validateFile(file, cmd));
    }
    
    // Test multiple file upload
    UploadResult result = uploadManager.uploadMultipleFiles(*browser, cmd.selector, files, cmd.timeout_ms);
    
    // Verify no critical failures
    EXPECT_NE(result, UploadResult::FILE_NOT_FOUND);
    EXPECT_NE(result, UploadResult::ELEMENT_NOT_FOUND);
}

// ========== Download Manager Integration Tests ==========

TEST_F(BrowserFileOpsIntegrationTest, DownloadManagerBasicIntegration) {
    DownloadManager downloadManager;
    
    // Create download command
    DownloadCommand cmd;
    cmd.download_dir = download_dir.string();
    cmd.filename_pattern = "test_file.*";
    cmd.timeout_ms = 5000;
    cmd.expected_size = 0; // Any size
    
    // Verify download directory exists
    EXPECT_TRUE(std::filesystem::exists(download_dir));
    EXPECT_TRUE(std::filesystem::is_directory(download_dir));
    
    // Test download directory setup
    EXPECT_TRUE(downloadManager.setDownloadDirectory(download_dir.string()));
    
    // Test basic download manager functionality
    std::string defaultDir = downloadManager.getDownloadDirectory();
    EXPECT_FALSE(defaultDir.empty());
}

TEST_F(BrowserFileOpsIntegrationTest, DownloadWithBrowserTrigger) {
    // Simulate triggering a download in the browser
    browser->clickElement("#download-link");
    std::this_thread::sleep_for(300ms);
    
    // Verify download was triggered (check UI state)
    std::string downloadStatus = browser->getInnerText("#download-status");
    EXPECT_NE(downloadStatus.find("Downloading"), std::string::npos);
    
    // Wait for simulated download completion
    std::this_thread::sleep_for(600ms);
    
    downloadStatus = browser->getInnerText("#download-status");
    EXPECT_NE(downloadStatus.find("complete"), std::string::npos);
}

// ========== Session and FileOps State Integration ==========

TEST_F(BrowserFileOpsIntegrationTest, FileOperationStateInSession) {
    // Perform multiple file operations
    browser->executeJavascriptSync(R"(
        // Simulate complex file operations
        uploadState.selectedFiles = ['file1.txt', 'file2.png', 'file3.pdf'];
        uploadState.uploadProgress = 60;
        uploadState.lastAction = 'batch_upload_in_progress';
        
        // Add download history
        window._hweb_download_history = [
            {filename: 'download1.txt', timestamp: Date.now() - 5000, status: 'complete'},
            {filename: 'download2.png', timestamp: Date.now() - 3000, status: 'complete'},
            {filename: 'current_download.pdf', timestamp: Date.now(), status: 'in_progress'}
        ];
        
        saveState();
        updateDisplay();
    )");
    
    // Update session with complex file operation state
    browser->updateSessionState(*session);
    
    // Create extractors for file operation state
    session->addStateExtractor("fileops", "window._hweb_fileops_state");
    session->addStateExtractor("downloads", "window._hweb_download_history");
    
    // Extract custom state
    auto customState = browser->extractCustomState(session->getStateExtractors());
    
    EXPECT_TRUE(customState.isMember("fileops"));
    EXPECT_TRUE(customState.isMember("downloads"));
    
    // Store in session
    auto memberNames = customState.getMemberNames();
    for (const auto& key : memberNames) {
        session->setExtractedState(key, customState[key]);
    }
    
    // Verify session contains file operation data
    Json::Value fileopsState = session->getExtractedState("fileops");
    EXPECT_FALSE(fileopsState.isNull());
    
    Json::Value downloadsState = session->getExtractedState("downloads");
    EXPECT_FALSE(downloadsState.isNull());
}

TEST_F(BrowserFileOpsIntegrationTest, CompleteFileOpsSessionWorkflow) {
    // Step 1: Setup complex file operations state
    browser->executeJavascriptSync(R"(
        uploadState = {
            selectedFiles: ['workflow_test.txt'],
            uploadProgress: 100,
            lastAction: 'upload_complete'
        };
        
        window._hweb_upload_history = [
            {filename: 'workflow_test.txt', size: 1024, timestamp: Date.now()}
        ];
        
        saveState();
    )");
    
    // Step 2: Fill form fields related to file operations
    browser->fillInput("#upload-status", "Workflow test completed");
    browser->focusElement("#upload-btn");
    browser->setScrollPosition(100, 150);
    
    std::this_thread::sleep_for(200ms);
    
    // Step 3: Capture complete session state
    browser->updateSessionState(*session);
    
    // Step 4: Create new browser and restore complete state
            HWeb::HWebConfig test_config;
        auto newBrowser = std::make_unique<Browser>(test_config);
    std::this_thread::sleep_for(500ms);
    
    newBrowser->restoreSession(*session);
    std::this_thread::sleep_for(800ms);
    
    // Step 5: Verify complete restoration
    // Check form state
    std::string uploadStatus = newBrowser->getAttribute("#upload-status", "value");
    EXPECT_EQ(uploadStatus, "Workflow test completed");
    
    // Check scroll position
    auto [x, y] = newBrowser->getScrollPosition();
    EXPECT_EQ(x, 100);
    EXPECT_EQ(y, 150);
    
    // Check custom state restoration
    newBrowser->executeJavascriptSync("loadState();");
    std::this_thread::sleep_for(200ms);
    
    std::string lastAction = newBrowser->getInnerText("#last-action");
    EXPECT_EQ(lastAction, "upload_complete");
    
    std::string selectedFiles = newBrowser->getInnerText("#selected-files");
    EXPECT_EQ(selectedFiles, "workflow_test.txt");
    
    std::string progress = newBrowser->getInnerText("#upload-progress");
    EXPECT_EQ(progress, "100%");
}

// ========== Error Handling and Edge Cases ==========

TEST_F(BrowserFileOpsIntegrationTest, FileOpsErrorHandlingIntegration) {
    UploadManager uploadManager;
    
    // Test upload with browser in invalid state
    browser->loadUri("data:text/html,<html><body>Minimal page</body></html>");
    std::this_thread::sleep_for(500ms);
    
    UploadCommand cmd;
    cmd.filepath = test_file.string();
    cmd.selector = "#nonexistent-upload";
    
    UploadResult result = uploadManager.uploadFile(*browser, cmd);
    EXPECT_EQ(result, UploadResult::ELEMENT_NOT_FOUND);
    
    // Test session handling with minimal page
    browser->updateSessionState(*session);
    
    // Should not crash and should update basic state
    EXPECT_FALSE(session->getCurrentUrl().empty());
    EXPECT_EQ(session->getDocumentReadyState(), "complete");
}

TEST_F(BrowserFileOpsIntegrationTest, SessionHandlingWithFileOpsErrors) {
    // Simulate file operation errors in JavaScript
    browser->executeJavascriptSync(R"(
        uploadState = {
            selectedFiles: [],
            uploadProgress: 0,
            lastAction: 'upload_error',
            errorMessage: 'File too large'
        };
        
        window._hweb_error_log = [
            {type: 'upload_error', message: 'File too large', timestamp: Date.now()},
            {type: 'download_error', message: 'Network timeout', timestamp: Date.now() - 1000}
        ];
        
        saveState();
    )");
    
    // Update session with error state
    browser->updateSessionState(*session);
    
    // Add error state extractors
    session->addStateExtractor("errorLog", "window._hweb_error_log");
    
    auto customState = browser->extractCustomState(session->getStateExtractors());
    
    // Verify error state was captured
    EXPECT_TRUE(customState.isMember("errorLog"));
    
    // Store error state in session
    session->setExtractedState("errorLog", customState["errorLog"]);
    
    // Verify error log was preserved
    Json::Value errorLog = session->getExtractedState("errorLog");
    EXPECT_FALSE(errorLog.isNull());
    EXPECT_TRUE(errorLog.isArray());
}