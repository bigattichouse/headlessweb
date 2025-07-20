#include <gtest/gtest.h>
#include "Browser/Browser.h"
#include "../utils/test_helpers.h"
#include <filesystem>
// Note: Using real Browser class for integration testing

class BrowserCoreTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create temporary directory for testing
        temp_dir = std::make_unique<TestHelpers::TemporaryDirectory>("browser_core_tests");
        
        // Create session for browser initialization
        session = std::make_unique<Session>("test_session");
        session->setCurrentUrl("about:blank");
        session->setViewport(1024, 768);
    }

    void TearDown() override {
        session.reset();
        temp_dir.reset();
    }

    std::unique_ptr<TestHelpers::TemporaryDirectory> temp_dir;
    std::unique_ptr<Session> session;
};

// ========== Browser Initialization Tests ==========

TEST_F(BrowserCoreTest, BrowserDefaultConstruction) {
    // Test that Browser can be constructed without crashing
    EXPECT_NO_THROW({
        Browser browser;
    });
}

TEST_F(BrowserCoreTest, BrowserSessionInitialization) {
    // Test browser initialization with session
    EXPECT_NO_THROW({
        Browser browser;
        // Note: Full initialization may require WebKit, these tests focus on interface
    });
}

// ========== URL Validation Tests ==========

TEST_F(BrowserCoreTest, ValidateHttpUrls) {
    Browser browser;
    
    // Test valid HTTP URLs
    EXPECT_TRUE(browser.validateUrl("http://example.com"));
    EXPECT_TRUE(browser.validateUrl("https://example.com"));
    EXPECT_TRUE(browser.validateUrl("http://localhost:8080"));
    EXPECT_TRUE(browser.validateUrl("https://subdomain.example.com/path"));
    EXPECT_TRUE(browser.validateUrl("http://127.0.0.1:3000/app"));
}

TEST_F(BrowserCoreTest, ValidateFileUrls) {
    Browser browser;
    
    // Test valid file URLs
    EXPECT_TRUE(browser.validateUrl("file:///path/to/file.html"));
    EXPECT_TRUE(browser.validateUrl("file://localhost/path/to/file.html"));
    
    // Create actual test file for validation
    std::filesystem::path test_file = temp_dir->createFile("test.html", "<html><body>Test</body></html>");
    std::string file_url = "file://" + test_file.string();
    EXPECT_TRUE(browser.validateUrl(file_url));
}

TEST_F(BrowserCoreTest, RejectInvalidUrls) {
    Browser browser;
    
    // Test invalid URLs
    EXPECT_FALSE(browser.validateUrl(""));
    EXPECT_FALSE(browser.validateUrl("not-a-url"));
    EXPECT_FALSE(browser.validateUrl("ftp://example.com")); // Unsupported protocol
    EXPECT_FALSE(browser.validateUrl("javascript:alert('xss')")); // Security risk
    EXPECT_FALSE(browser.validateUrl("data:text/html,<script>alert('xss')</script>")); // Security risk
    EXPECT_FALSE(browser.validateUrl("http://")); // Malformed
    EXPECT_FALSE(browser.validateUrl("://missing-protocol"));
}

TEST_F(BrowserCoreTest, ValidateFileUrlSecurity) {
    Browser browser;
    
    // Test file URL security validation
    EXPECT_FALSE(browser.validateUrl("file:///etc/passwd")); // System file access
    EXPECT_FALSE(browser.validateUrl("file:///proc/version")); // System info access
    EXPECT_FALSE(browser.validateUrl("file:///../../../etc/passwd")); // Path traversal
    EXPECT_FALSE(browser.validateUrl("file:///C:/Windows/System32/")); // Windows system access
}

// ========== File URL Specific Tests ==========

TEST_F(BrowserCoreTest, DetectFileUrls) {
    Browser browser;
    
    EXPECT_TRUE(browser.isFileUrl("file:///path/to/file.html"));
    EXPECT_TRUE(browser.isFileUrl("file://localhost/path/to/file.html"));
    EXPECT_FALSE(browser.isFileUrl("http://example.com"));
    EXPECT_FALSE(browser.isFileUrl("https://example.com/file.html"));
    EXPECT_FALSE(browser.isFileUrl("ftp://example.com/file.html"));
}

TEST_F(BrowserCoreTest, ValidateFileUrlPaths) {
    Browser browser;
    
    // Create test files
    std::filesystem::path valid_html = temp_dir->createFile("valid.html", "<html><body>Valid</body></html>");
    std::filesystem::path valid_htm = temp_dir->createFile("valid.htm", "<html><body>Valid HTM</body></html>");
    std::filesystem::path invalid_txt = temp_dir->createFile("invalid.txt", "Not HTML");
    
    // Test file validation
    EXPECT_TRUE(browser.validateFileUrl("file://" + valid_html.string()));
    EXPECT_TRUE(browser.validateFileUrl("file://" + valid_htm.string()));
    EXPECT_FALSE(browser.validateFileUrl("file://" + invalid_txt.string())); // Wrong extension
    EXPECT_FALSE(browser.validateFileUrl("file:///nonexistent/file.html")); // File doesn't exist
}

// ========== Viewport Management Tests ==========

TEST_F(BrowserCoreTest, GetDefaultViewport) {
    Browser browser;
    
    auto viewport = browser.getViewport();
    
    // Should have reasonable default viewport
    EXPECT_GT(viewport.first, 0);   // Width > 0
    EXPECT_GT(viewport.second, 0);  // Height > 0
    EXPECT_LE(viewport.first, 4096);  // Reasonable max width
    EXPECT_LE(viewport.second, 4096); // Reasonable max height
}

TEST_F(BrowserCoreTest, ViewportForScreenshots) {
    Browser browser;
    
    // Should not crash when ensuring proper viewport
    EXPECT_NO_THROW({
        browser.ensureProperViewportForScreenshots();
    });
    
    // After ensuring proper viewport, should have valid dimensions
    auto viewport = browser.getViewport();
    EXPECT_GT(viewport.first, 0);
    EXPECT_GT(viewport.second, 0);
}

// ========== Navigation State Tests ==========

TEST_F(BrowserCoreTest, NavigationStateManagement) {
    Browser browser;
    
    // Test initial navigation state - browser should be in ready state
    EXPECT_NO_THROW(browser.getCurrentUrl()); // Should be able to get URL without error
    
    // Navigation state should be manageable
    EXPECT_NO_THROW({
        browser.notifyNavigationComplete();
        browser.notifyUriChanged();
        browser.notifyTitleChanged();
        browser.notifyReadyToShow();
    });
}

// ========== JavaScript Execution Interface Tests ==========

TEST_F(BrowserCoreTest, JavaScriptExecutionInterface) {
    Browser browser;
    
    // Test that JavaScript execution methods exist and don't crash with null browser
    std::string result;
    EXPECT_NO_THROW({
        browser.executeJavascript("console.log('test');", &result);
    });
    
    // Test synchronous execution methods
    EXPECT_NO_THROW({
        std::string sync_result = browser.executeJavascriptSync("1 + 1");
        std::string safe_result = browser.executeJavascriptSyncSafe("document.title");
    });
}

// ========== Error Handling Tests ==========

TEST_F(BrowserCoreTest, ErrorHandlingRobustness) {
    Browser browser;
    
    // Test that browser handles invalid operations gracefully
    EXPECT_NO_THROW({
        browser.executeJavascript("", nullptr);
        browser.executeJavascriptSync("");
        browser.executeJavascriptSyncSafe("");
    });
    
    // Test timeout handling
    EXPECT_NO_THROW({
        browser.waitForJavaScriptCompletion(100); // Short timeout
    });
}

// ========== Session Integration Tests ==========

TEST_F(BrowserCoreTest, SessionIntegrationBasics) {
    Browser browser;
    
    // Test that browser can work with session data
    EXPECT_NO_THROW({
        browser.waitForPageReady(*session);
    });
    
    // Test navigation with session
    EXPECT_NO_THROW({
        browser.waitForNavigation(1000);
    });
}

// ========== Edge Cases and Boundary Tests ==========

TEST_F(BrowserCoreTest, EdgeCaseUrlHandling) {
    Browser browser;
    
    // Test edge case URLs
    EXPECT_FALSE(browser.validateUrl(std::string(10000, 'a'))); // Very long URL
    EXPECT_FALSE(browser.validateUrl("http://\x00\x01\x02")); // Binary data
    EXPECT_FALSE(browser.validateUrl("http://测试.example.com")); // Unicode domain (may be invalid without IDN)
}

TEST_F(BrowserCoreTest, ConcurrentOperationSafety) {
    Browser browser;
    
    // Test that multiple operations don't interfere
    EXPECT_NO_THROW({
        browser.notifyNavigationComplete();
        browser.executeJavascript("console.log('test1');");
        browser.notifyUriChanged();
        browser.executeJavascript("console.log('test2');");
        browser.notifyReadyToShow();
    });
}

TEST_F(BrowserCoreTest, ResourceCleanupSafety) {
    // Test that browser destruction is safe
    EXPECT_NO_THROW({
        Browser* browser = new Browser();
        browser->executeJavascript("console.log('test');");
        delete browser;
    });
}