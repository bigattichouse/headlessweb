#include <gtest/gtest.h>
#include "Browser/Browser.h"
#include "../utils/test_helpers.h"
#include <thread>
#include <chrono>
#include <filesystem>
#include <fstream>

using namespace std::chrono_literals;

class BrowserMainTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create fresh browser instance for each test
        HWeb::HWebConfig test_config;
        browser = std::make_unique<Browser>(test_config);
        
        // Create temporary directory for file:// URLs
        temp_dir = std::make_unique<TestHelpers::TemporaryDirectory>("browser_main_tests");
        
        // Allow browser initialization to complete
        std::this_thread::sleep_for(500ms);
    }
    
    void TearDown() override {
        browser.reset();
        temp_dir.reset();
    }
    
    // Helper method to create file:// URL from HTML content
    std::string createTestPage(const std::string& html_content, const std::string& filename = "test.html") {
        auto html_file = temp_dir->createFile(filename, html_content);
        return "file://" + html_file.string();
    }
    
    // Helper method to load page and wait for full readiness
    bool loadPageWithReadinessCheck(const std::string& url) {
        browser->loadUri(url);
        
        // Wait for navigation
        bool nav_success = browser->waitForNavigation(5000);
        if (!nav_success) return false;
        
        // Check basic JavaScript execution
        std::string js_test = browser->executeJavascriptSync("'test'");
        if (js_test != "test") {
            std::this_thread::sleep_for(1000ms);
        }
        
        // Verify DOM is complete
        std::string dom_ready = browser->executeJavascriptSync("document.readyState === 'complete'");
        if (dom_ready != "true") {
            std::this_thread::sleep_for(500ms);
        }
        
        return true;
    }

    std::unique_ptr<Browser> browser;
    std::unique_ptr<TestHelpers::TemporaryDirectory> temp_dir;
};

// ========== Constructor and Initialization Tests ==========

TEST_F(BrowserMainTest, ConstructorInitializesWebView) {
    // Browser should be properly initialized
    EXPECT_NE(browser->webView, nullptr);
    EXPECT_NE(browser->window, nullptr);
    EXPECT_NE(browser->main_loop, nullptr);
}

TEST_F(BrowserMainTest, ConstructorCreatesSessionDataPath) {
    // Should create session data directory
    std::string home = std::getenv("HOME");
    std::string expected_path = home + "/.hweb/webkit-data";
    
    EXPECT_TRUE(std::filesystem::exists(expected_path));
    EXPECT_TRUE(std::filesystem::is_directory(expected_path));
}

TEST_F(BrowserMainTest, WindowConfiguration) {
    // Window should be properly configured for headless operation
    // We don't test visibility directly due to GTK API limitations in tests
    EXPECT_NE(browser->window, nullptr);
}

TEST_F(BrowserMainTest, DefaultViewportSize) {
    // Should have reasonable default viewport
    auto [width, height] = browser->getViewport();
    
    EXPECT_GT(width, 0);
    EXPECT_GT(height, 0);
    EXPECT_GE(width, 1024);  // Minimum reasonable width
    EXPECT_GE(height, 768);  // Minimum reasonable height
}

// ========== Multiple Browser Instance Tests ==========

TEST_F(BrowserMainTest, MultipleBrowserInstances) {
    // Should be able to create multiple browser instances
    HWeb::HWebConfig test_config;
    auto browser2 = std::make_unique<Browser>(test_config);
    auto browser3 = std::make_unique<Browser>(test_config);
    
    EXPECT_NE(browser->webView, nullptr);
    EXPECT_NE(browser2->webView, nullptr);  
    EXPECT_NE(browser3->webView, nullptr);
    
    // Each should have unique webview instances
    EXPECT_NE(browser->webView, browser2->webView);
    EXPECT_NE(browser->webView, browser3->webView);
    EXPECT_NE(browser2->webView, browser3->webView);
    
    browser2.reset();
    browser3.reset();
}

TEST_F(BrowserMainTest, BrowserLifecycleRapidCreateDestroy) {
    // Test rapid creation and destruction
    for (int i = 0; i < 5; i++) {
        HWeb::HWebConfig test_config;
        auto temp_browser = std::make_unique<Browser>(test_config);
        EXPECT_NE(temp_browser->webView, nullptr);
        temp_browser.reset();
    }
}

// ========== Core Browser Interface Tests ==========

TEST_F(BrowserMainTest, LoadSimplePage) {
    std::string simple_html = R"HTML(
<!DOCTYPE html>
<html>
<head><title>Test Page</title></head>
<body><h1>Hello World</h1></body>
</html>
)HTML";
    
    // CRITICAL FIX: Use file:// URL instead of data: URL with enhanced readiness
    std::string file_url = createTestPage(simple_html);
    
    EXPECT_NO_THROW({
        bool load_success = loadPageWithReadinessCheck(file_url);
        EXPECT_TRUE(load_success);
    });
    
    // Verify page loaded
    std::string title = browser->getPageTitle();
    EXPECT_EQ(title, "Test Page");
    
    std::string current_url = browser->getCurrentUrl();
    EXPECT_NE(current_url.find("file://"), std::string::npos);
}

TEST_F(BrowserMainTest, GetCurrentUrlInitial) {
    // Initial URL should be empty or about:blank
    std::string url = browser->getCurrentUrl();
    EXPECT_TRUE(url.empty() || url == "about:blank");
}

TEST_F(BrowserMainTest, GetPageTitleInitial) {
    // Initial title should be empty
    std::string title = browser->getPageTitle();
    EXPECT_TRUE(title.empty());
}

TEST_F(BrowserMainTest, ViewportManagement) {
    // Test viewport setting and getting
    browser->setViewport(1280, 720);
    
    // Allow viewport change to take effect
    std::this_thread::sleep_for(200ms);
    
    auto [width, height] = browser->getViewport();
    
    // Should be close to requested size (allowing for some variance)
    EXPECT_NEAR(width, 1280, 50);
    EXPECT_NEAR(height, 720, 50);
}

TEST_F(BrowserMainTest, UserAgentSetting) {
    std::string custom_ua = "HeadlessWeb Test Agent 1.0";
    
    EXPECT_NO_THROW(browser->setUserAgent(custom_ua));
    
    // Load a page to test user agent
    std::string test_html = R"HTML(
<!DOCTYPE html>
<html>
<head><title>UA Test</title></head>
<body>
    <div id="ua-display"></div>
    <script>
        document.getElementById('ua-display').textContent = navigator.userAgent;
    </script>
</body>
</html>
)HTML";
    
    std::string data_url = "data:text/html;charset=utf-8," + test_html;
    browser->loadUri(data_url);
    std::this_thread::sleep_for(800ms);
    
    std::string displayed_ua = browser->getInnerText("#ua-display");
    EXPECT_NE(displayed_ua.find("HeadlessWeb Test Agent"), std::string::npos);
}

// ========== JavaScript Integration Tests ==========

TEST_F(BrowserMainTest, BasicJavaScriptExecution) {
    std::string test_html = R"HTML(
<!DOCTYPE html>
<html>
<head><title>JS Test</title></head>
<body><div id="target">Original</div></body>
</html>
)HTML";
    
    std::string data_url = "data:text/html;charset=utf-8," + test_html;
    browser->loadUri(data_url);
    std::this_thread::sleep_for(600ms);
    
    // Test JavaScript execution
    std::string result = browser->executeJavascriptSync("2 + 3");
    EXPECT_EQ(result, "5");
    
    // Test DOM manipulation via JavaScript
    browser->executeJavascript("document.getElementById('target').textContent = 'Modified';");
    std::this_thread::sleep_for(100ms);
    
    std::string content = browser->getInnerText("#target");
    EXPECT_EQ(content, "Modified");
}

TEST_F(BrowserMainTest, JavaScriptErrorHandling) {
    std::string test_html = "data:text/html,<html><body></body></html>";
    browser->loadUri(test_html);
    std::this_thread::sleep_for(500ms);
    
    // Test safe JavaScript execution with invalid syntax
    std::string result = browser->executeJavascriptSyncSafe("invalid.syntax.here");
    
    // Should not crash and should return empty or error indicator
    EXPECT_TRUE(result.empty() || result.find("error") != std::string::npos || result == "undefined");
}

// ========== DOM Interaction Tests ==========

TEST_F(BrowserMainTest, BasicDOMInteraction) {
    std::string form_html = R"HTML(
<!DOCTYPE html>
<html>
<head><title>DOM Test</title></head>
<body>
    <form id="test-form">
        <input type="text" id="name-input" placeholder="Name" />
        <input type="email" id="email-input" placeholder="Email" />
        <button type="button" id="test-btn">Click Me</button>
        <div id="result"></div>
    </form>
    <script>
        document.getElementById('test-btn').addEventListener('click', function() {
            document.getElementById('result').textContent = 'Button clicked!';
        });
    </script>
</body>
</html>
)HTML";
    
    std::string data_url = "data:text/html;charset=utf-8," + form_html;
    browser->loadUri(data_url);
    std::this_thread::sleep_for(800ms);
    
    // Test element existence
    EXPECT_TRUE(browser->elementExists("#name-input"));
    EXPECT_TRUE(browser->elementExists("#test-btn"));
    EXPECT_FALSE(browser->elementExists("#nonexistent"));
    
    // Test filling inputs
    EXPECT_TRUE(browser->fillInput("#name-input", "John Doe"));
    EXPECT_TRUE(browser->fillInput("#email-input", "john@example.com"));
    
    // Test clicking
    EXPECT_TRUE(browser->clickElement("#test-btn"));
    std::this_thread::sleep_for(200ms);
    
    // Verify click effect
    std::string result = browser->getInnerText("#result");
    EXPECT_EQ(result, "Button clicked!");
    
    // Test getting input values
    std::string name_value = browser->getAttribute("#name-input", "value");
    EXPECT_EQ(name_value, "John Doe");
}

TEST_F(BrowserMainTest, ElementCounting) {
    std::string list_html = R"HTML(
<!DOCTYPE html>
<html>
<head><title>Count Test</title></head>
<body>
    <ul>
        <li class="item">Item 1</li>
        <li class="item">Item 2</li>
        <li class="item">Item 3</li>
    </ul>
    <div class="item">Div item</div>
</body>
</html>
)HTML";
    
    std::string data_url = "data:text/html;charset=utf-8," + list_html;
    browser->loadUri(data_url);
    std::this_thread::sleep_for(600ms);
    
    // Test element counting
    EXPECT_EQ(browser->countElements(".item"), 4);
    EXPECT_EQ(browser->countElements("li"), 3);
    EXPECT_EQ(browser->countElements("ul"), 1);
    EXPECT_EQ(browser->countElements(".nonexistent"), 0);
}

// ========== Navigation Tests ==========

TEST_F(BrowserMainTest, BasicNavigation) {
    // Load first page using file:// URL
    std::string page1_html = "<html><head><title>Page 1</title></head><body><h1>First Page</h1></body></html>";
    std::string page1_url = createTestPage(page1_html, "page1.html");
    browser->loadUri(page1_url);
    
    // Enhanced page load waiting
    bool nav_success = browser->waitForNavigation(5000);
    EXPECT_TRUE(nav_success);
    
    EXPECT_EQ(browser->getPageTitle(), "Page 1");
    
    // Load second page using file:// URL  
    std::string page2_html = "<html><head><title>Page 2</title></head><body><h1>Second Page</h1></body></html>";
    std::string page2_url = createTestPage(page2_html, "page2.html");
    browser->loadUri(page2_url);
    std::this_thread::sleep_for(600ms);
    
    EXPECT_EQ(browser->getPageTitle(), "Page 2");
    
    // Test go back
    browser->goBack();
    std::this_thread::sleep_for(600ms);
    
    EXPECT_EQ(browser->getPageTitle(), "Page 1");
    
    // Test go forward
    browser->goForward();
    std::this_thread::sleep_for(600ms);
    
    EXPECT_EQ(browser->getPageTitle(), "Page 2");
}

TEST_F(BrowserMainTest, PageReload) {
    std::string dynamic_html = R"HTML(
<!DOCTYPE html>
<html>
<head><title>Reload Test</title></head>
<body>
    <div id="timestamp"></div>
    <script>
        document.getElementById('timestamp').textContent = Date.now();
    </script>
</body>
</html>
)HTML";
    
    std::string data_url = "data:text/html;charset=utf-8," + dynamic_html;
    browser->loadUri(data_url);
    std::this_thread::sleep_for(800ms);
    
    std::string first_timestamp = browser->getInnerText("#timestamp");
    EXPECT_FALSE(first_timestamp.empty());
    
    // Small delay to ensure different timestamp
    std::this_thread::sleep_for(100ms);
    
    // Reload page
    browser->reload();
    std::this_thread::sleep_for(800ms);
    
    std::string second_timestamp = browser->getInnerText("#timestamp");
    EXPECT_FALSE(second_timestamp.empty());
    
    // Timestamps should be different (page reloaded)
    EXPECT_NE(first_timestamp, second_timestamp);
}

// ========== URL Validation Tests ==========

TEST_F(BrowserMainTest, URLValidation) {
    // Valid URLs
    EXPECT_TRUE(browser->validateUrl("https://example.com"));
    EXPECT_TRUE(browser->validateUrl("http://localhost:8080"));
    EXPECT_TRUE(browser->validateUrl("file:///path/to/file.html"));
    EXPECT_TRUE(browser->validateUrl("data:text/html,<html></html>"));
    
    // Invalid URLs
    EXPECT_FALSE(browser->validateUrl(""));
    EXPECT_FALSE(browser->validateUrl("not-a-url"));
    EXPECT_FALSE(browser->validateUrl("javascript:alert('test')"));
}

TEST_F(BrowserMainTest, FileURLValidation) {
    EXPECT_TRUE(browser->isFileUrl("file:///path/to/file.html"));
    EXPECT_TRUE(browser->isFileUrl("file://localhost/path/to/file.html"));
    EXPECT_FALSE(browser->isFileUrl("https://example.com"));
    EXPECT_FALSE(browser->isFileUrl("http://localhost"));
    EXPECT_FALSE(browser->isFileUrl("data:text/html,test"));
}

// ========== Error Handling and Edge Cases ==========

TEST_F(BrowserMainTest, InvalidOperationsHandling) {
    // Test operations on empty/invalid page
    EXPECT_FALSE(browser->elementExists("#nonexistent"));
    EXPECT_FALSE(browser->clickElement("#nonexistent"));
    EXPECT_FALSE(browser->fillInput("#nonexistent", "test"));
    
    std::string empty_attr = browser->getAttribute("#nonexistent", "id");
    EXPECT_TRUE(empty_attr.empty());
    
    std::string empty_text = browser->getInnerText("#nonexistent");
    EXPECT_TRUE(empty_text.empty());
}

TEST_F(BrowserMainTest, EmptyPageOperations) {
    // Load minimal empty page
    browser->loadUri("data:text/html,<html><body></body></html>");
    std::this_thread::sleep_for(500ms);
    
    // These should not crash
    EXPECT_EQ(browser->countElements("div"), 0);
    EXPECT_FALSE(browser->elementExists("div"));
    EXPECT_TRUE(browser->getPageTitle().empty());
    
    // JavaScript should still work
    std::string result = browser->executeJavascriptSync("1 + 1");
    EXPECT_EQ(result, "2");
}

// ========== State Management Tests ==========

TEST_F(BrowserMainTest, BrowserStateConsistency) {
    std::string complex_html = R"HTML(
<!DOCTYPE html>
<html>
<head><title>State Test</title></head>
<body>
    <div id="state-info">
        <div id="ready-state"></div>
        <div id="location-info"></div>
    </div>
    <script>
        document.getElementById('ready-state').textContent = document.readyState;
        document.getElementById('location-info').textContent = window.location.href;
    </script>
</body>
</html>
)HTML";
    
    std::string data_url = "data:text/html;charset=utf-8," + complex_html;
    browser->loadUri(data_url);
    std::this_thread::sleep_for(800ms);
    
    // Verify page state
    std::string ready_state = browser->getInnerText("#ready-state");
    EXPECT_EQ(ready_state, "complete");
    
    std::string location = browser->getInnerText("#location-info");
    EXPECT_NE(location.find("data:text/html"), std::string::npos);
    
    // Browser state should be consistent
    std::string current_url = browser->getCurrentUrl();
    EXPECT_EQ(current_url, location);
}

// ========== Memory and Resource Management Tests ==========

TEST_F(BrowserMainTest, ResourceCleanupOnDestruction) {
    // Create and destroy browser to test cleanup
    {
        HWeb::HWebConfig test_config;
        auto temp_browser = std::make_unique<Browser>(test_config);
        EXPECT_NE(temp_browser->webView, nullptr);
        // Browser destructor should be called here
    }
    
    // Original browser should still work
    EXPECT_NE(browser->webView, nullptr);
    std::string result = browser->executeJavascriptSync("42");
    EXPECT_EQ(result, "42");
}