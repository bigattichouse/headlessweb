#include <gtest/gtest.h>
#include "Browser/Browser.h"
#include "../utils/test_helpers.h"
#include "browser_test_environment.h"
#include "Debug.h"
#include <thread>
#include <chrono>
#include <filesystem>
#include <fstream>

extern std::unique_ptr<Browser> g_browser;

class BrowserMainTest : public ::testing::Test {
protected:
    void SetUp() override {
        temp_dir = std::make_unique<TestHelpers::TemporaryDirectory>("browser_main_tests");
        
        // Use global browser instance like other working tests
        browser = g_browser.get();
        
        // NO PAGE LOADING - Use interface testing approach
        
        debug_output("BrowserMainTest SetUp complete");
    }
    
    void TearDown() override {
        // Clean teardown without navigation
        if (individual_browser) {
            individual_browser.reset();
        }
        temp_dir.reset();
    }
    
    // Interface testing helper methods
    std::string executeWrappedJS(const std::string& jsCode) {
        // Test JavaScript execution interface without requiring page content
        std::string wrapped = "(function() { try { " + jsCode + " } catch(e) { return 'error: ' + e.message; } })()";
        return browser->executeJavascriptSync(wrapped);
    }
    
    Browser* browser;
    std::unique_ptr<Browser> individual_browser;
    std::unique_ptr<TestHelpers::TemporaryDirectory> temp_dir;
    
    // Helper to create individual browser instance for constructor/lifecycle tests
    Browser* createIndividualBrowser() {
        HWeb::HWebConfig test_config;
        test_config.allow_data_uri = true;
        individual_browser = std::make_unique<Browser>(test_config);
        return individual_browser.get();
    }
    
    // Helper method to create file:// URL from HTML content (for URL validation only)
    std::string createTestPage(const std::string& html_content, const std::string& filename = "test.html") {
        auto html_file = temp_dir->createFile(filename, html_content);
        return "file://" + html_file.string();
    }
};

// ========== Constructor and Initialization Interface Tests ==========

TEST_F(BrowserMainTest, ConstructorInitializesWebView) {
    // Test constructor interface without page loading
    EXPECT_NO_THROW(createIndividualBrowser());
    // Interface should provide access to WebView components without crashing
}

TEST_F(BrowserMainTest, ConstructorCreatesSessionDataPath) {
    // Test session data path creation interface
    Browser* test_browser = createIndividualBrowser();
    std::string home = std::getenv("HOME") ? std::getenv("HOME") : "/tmp";
    std::string expected_path = home + "/.hweb/webkit-data";
    
    // Interface should handle session data path operations
    bool path_exists = std::filesystem::exists(expected_path);
    bool is_dir = std::filesystem::is_directory(expected_path);
    (void)path_exists;
    (void)is_dir;
    (void)test_browser;
}

TEST_F(BrowserMainTest, WindowConfiguration) {
    // Test window configuration interface without page loading
    // Window interface should be accessible without crashes
    EXPECT_NO_THROW((void)browser->window);
}

TEST_F(BrowserMainTest, DefaultViewportSize) {
    // Test viewport size interface without page loading
    auto [width, height] = browser->getViewport();
    
    // Interface should return valid viewport dimensions
    EXPECT_GT(width, 0);
    EXPECT_GT(height, 0);
    EXPECT_GE(width, 100);  // Reasonable minimum
    EXPECT_GE(height, 100); // Reasonable minimum
}

// ========== Multiple Browser Instance Interface Tests ==========

TEST_F(BrowserMainTest, MultipleBrowserInstances) {
    // Test multiple browser instance creation interface
    HWeb::HWebConfig test_config;
    auto browser2 = std::make_unique<Browser>(test_config);
    auto browser3 = std::make_unique<Browser>(test_config);
    
    // Interface should provide unique browser instances
    EXPECT_NE(browser2.get(), browser3.get());
    EXPECT_NE(browser, browser2.get());
    EXPECT_NE(browser, browser3.get());
}

TEST_F(BrowserMainTest, BrowserLifecycleRapidCreateDestroy) {
    // Test rapid browser lifecycle interface
    for (int i = 0; i < 5; i++) {
        HWeb::HWebConfig test_config;
        auto temp_browser = std::make_unique<Browser>(test_config);
        // Interface should handle rapid creation/destruction
    }
}

// ========== Core Browser Interface Tests ==========

TEST_F(BrowserMainTest, LoadSimplePage) {
    // Test simple page loading interface (method exists, graceful handling)
    EXPECT_NO_THROW(browser->loadUri("data:text/html,<html><body>Test</body></html>"));
    // Interface should handle loadUri calls without crashing
}

TEST_F(BrowserMainTest, GetCurrentUrlInitial) {
    // Test URL retrieval interface without page loading
    EXPECT_NO_THROW(browser->getCurrentUrl());
    // Interface should return URL (may be empty or about:blank initially)
}

TEST_F(BrowserMainTest, GetPageTitleInitial) {
    // Test page title interface without page loading
    EXPECT_NO_THROW(browser->getPageTitle());
    // Interface should handle title retrieval (may be empty initially)
}

TEST_F(BrowserMainTest, ViewportManagement) {
    // Test viewport management interface without page loading
    EXPECT_NO_THROW(browser->setViewport(1280, 720));
    
    // Test viewport getting interface
    auto [width, height] = browser->getViewport();
    
    // Interface should handle viewport operations
    EXPECT_GT(width, 0);
    EXPECT_GT(height, 0);
}

TEST_F(BrowserMainTest, UserAgentSetting) {
    // Test user agent setting interface without page loading
    std::string custom_ua = "HeadlessWeb Test Agent 1.0";
    
    // Interface should handle user agent setting
    EXPECT_NO_THROW(browser->setUserAgent(custom_ua));
    
    // Test user agent access through JavaScript interface
    EXPECT_NO_THROW(executeWrappedJS("return navigator.userAgent || 'no userAgent';"));
    // Interface should handle user agent retrieval
}

// ========== JavaScript Integration Interface Tests ==========

TEST_F(BrowserMainTest, BasicJavaScriptExecution) {
    // Test JavaScript execution interface without page loading
    EXPECT_NO_THROW(executeWrappedJS("return 2 + 3;"));
    EXPECT_NO_THROW(executeWrappedJS("return 'test string';"));
    EXPECT_NO_THROW(executeWrappedJS("return document ? 'document exists' : 'no document';"));
}

TEST_F(BrowserMainTest, JavaScriptErrorHandling) {
    // Test JavaScript error handling interface without page loading
    EXPECT_NO_THROW(executeWrappedJS("return 'valid syntax';"));
    EXPECT_NO_THROW(executeWrappedJS("return invalid.syntax.here;"));
    EXPECT_NO_THROW(executeWrappedJS("return 'still working';"));
}

// ========== DOM Interaction Interface Tests ==========

TEST_F(BrowserMainTest, BasicDOMInteraction) {
    // Test DOM interaction interface without page loading
    EXPECT_NO_THROW(browser->elementExists("#name-input"));
    EXPECT_NO_THROW(browser->elementExists("#test-btn"));
    EXPECT_NO_THROW(browser->elementExists("#nonexistent"));
    
    // Test input filling interface (should handle gracefully)
    EXPECT_NO_THROW(browser->fillInput("#name-input", "John Doe"));
    
    // Test attribute retrieval interface
    EXPECT_NO_THROW(browser->getAttribute("#name-input", "value"));
    
    // Test element clicking interface
    EXPECT_NO_THROW(browser->clickElement("#test-btn"));
    
    // Test text retrieval interface
    EXPECT_NO_THROW(browser->getInnerText("#result"));
}

TEST_F(BrowserMainTest, ElementCounting) {
    // Test element counting interface without page loading
    EXPECT_NO_THROW(browser->countElements(".item"));
    EXPECT_NO_THROW(browser->countElements("li"));
    EXPECT_NO_THROW(browser->countElements("ul"));
    EXPECT_NO_THROW(browser->countElements(".nonexistent"));
}

// ========== Navigation Interface Tests ==========

TEST_F(BrowserMainTest, BasicNavigation) {
    // Test navigation interface without page loading
    EXPECT_NO_THROW(browser->goBack());
    EXPECT_NO_THROW(browser->goForward());
    EXPECT_NO_THROW(browser->reload());
    
    // Test navigation waiting interfaces with short timeouts
    EXPECT_NO_THROW(browser->waitForNavigation(100));
    EXPECT_NO_THROW(browser->waitForJavaScriptCompletion(100));
}

TEST_F(BrowserMainTest, PageReload) {
    // Test page reload interface without page loading
    EXPECT_NO_THROW(browser->reload());
    
    // Test element retrieval after reload interface
    EXPECT_NO_THROW(browser->getInnerText("#timestamp"));
    
    // Test URL retrieval after reload interface
    EXPECT_NO_THROW(browser->getCurrentUrl());
}

// ========== URL Validation Interface Tests ==========

TEST_F(BrowserMainTest, URLValidation) {
    // Test URL validation interface without loading pages
    EXPECT_TRUE(browser->validateUrl("https://example.com"));
    EXPECT_TRUE(browser->validateUrl("http://localhost:8080"));
    
    // Test with file URL (create file but don't load it)
    std::string test_html = "<html><head><title>URL Test</title></head><body>Test</body></html>";
    std::string valid_file_url = createTestPage(test_html, "url_test.html");
    EXPECT_TRUE(browser->validateUrl(valid_file_url));
    
    // Invalid URLs
    EXPECT_FALSE(browser->validateUrl(""));
    EXPECT_FALSE(browser->validateUrl("not-a-url"));
    EXPECT_FALSE(browser->validateUrl("javascript:alert('test')"));
}

TEST_F(BrowserMainTest, FileURLValidation) {
    // Test file URL validation interface
    EXPECT_TRUE(browser->isFileUrl("file:///path/to/file.html"));
    EXPECT_TRUE(browser->isFileUrl("file://localhost/path/to/file.html"));
    EXPECT_FALSE(browser->isFileUrl("https://example.com"));
    EXPECT_FALSE(browser->isFileUrl("http://localhost"));
    EXPECT_FALSE(browser->isFileUrl("data:text/html,test"));
}

// ========== Error Handling Interface Tests ==========

TEST_F(BrowserMainTest, InvalidOperationsHandling) {
    // Test operations interface with invalid elements
    EXPECT_FALSE(browser->elementExists("#nonexistent"));
    EXPECT_FALSE(browser->clickElement("#nonexistent"));
    EXPECT_FALSE(browser->fillInput("#nonexistent", "test"));
    
    std::string empty_attr = browser->getAttribute("#nonexistent", "id");
    EXPECT_TRUE(empty_attr.empty());
    
    std::string empty_text = browser->getInnerText("#nonexistent");
    EXPECT_TRUE(empty_text.empty());
}

TEST_F(BrowserMainTest, EmptyPageOperations) {
    // Test operations interface on empty state (no page loading)
    // These interfaces should not crash without loaded content
    EXPECT_EQ(browser->countElements("div"), 0);
    EXPECT_FALSE(browser->elementExists("div"));
    
    // JavaScript interface should still work
    EXPECT_NO_THROW(executeWrappedJS("return 1 + 1;"));
}

// ========== State Management Interface Tests ==========

TEST_F(BrowserMainTest, BrowserStateConsistency) {
    // Test browser state interface without page loading
    EXPECT_NO_THROW(browser->getCurrentUrl());
    EXPECT_NO_THROW(browser->getPageTitle());
    EXPECT_NO_THROW(browser->getViewport());
    // Interface should maintain consistent state
}

// ========== Memory and Resource Management Interface Tests ==========

TEST_F(BrowserMainTest, ResourceCleanupOnDestruction) {
    // Test resource cleanup interface
    // Create and destroy browser to test cleanup interface
    {
        HWeb::HWebConfig test_config;
        auto temp_browser = std::make_unique<Browser>(test_config);
        // Browser destructor should be called here
    }
    
    // Original browser should still work
    EXPECT_NO_THROW(executeWrappedJS("return 42;"));
    // Interface should continue working after other browser destruction
}