#include <gtest/gtest.h>
#include "Browser/Browser.h"
#include "../utils/test_helpers.h"
#include "browser_test_environment.h"
#include "Debug.h"
#include <thread>
#include <chrono>
#include <filesystem>
#include <fstream>

using namespace std::chrono_literals;

extern std::unique_ptr<Browser> g_browser;

class BrowserMainTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Use global browser instance (properly initialized) but reset state for each test
        browser = g_browser.get();
        
        // Create temporary directory for file:// URLs
        temp_dir = std::make_unique<TestHelpers::TemporaryDirectory>("browser_main_tests");
        
        // Reset browser to clean state before each test
        browser->loadUri("about:blank");
        browser->waitForNavigation(2000);
        
        debug_output("BrowserMainTest SetUp complete");
    }
    
    void TearDown() override {
        // Clean up individual browser instances if created
        if (individual_browser) {
            individual_browser.reset();
        }
        temp_dir.reset();
    }
    
    // Generic JavaScript wrapper function for safe execution
    std::string executeWrappedJS(const std::string& jsCode) {
        // Try direct execution first (for simple expressions)
        if (jsCode.find("return ") == 0) {
            // Remove "return " and execute as expression
            std::string expression = jsCode.substr(7); // Remove "return "
            std::string result = browser->executeJavascriptSync(expression);
            if (!result.empty()) return result;
        }
        
        // Fallback to wrapped function approach
        std::string wrapped = "(function() { " + jsCode + " })()";
        return browser->executeJavascriptSync(wrapped);
    }
    
    // Helper method to create file:// URL from HTML content
    std::string createTestPage(const std::string& html_content, const std::string& filename = "test.html") {
        auto html_file = temp_dir->createFile(filename, html_content);
        return "file://" + html_file.string();
    }
    
    // Helper method to load page and wait for full readiness
    bool loadPageWithReadinessCheck(const std::string& url, bool require_title = true) {
        browser->loadUri(url);
        
        // Wait for navigation
        bool nav_success = browser->waitForNavigation(5000);
        if (!nav_success) return false;
        
        // CRITICAL FIX: Enhanced readiness checking with WebKit-specific timing
        // Allow WebKit processing time
        std::this_thread::sleep_for(1000ms);
        
        // Check basic JavaScript execution with retry
        for (int i = 0; i < 5; i++) {
            std::string js_test = executeWrappedJS("return 'test';");
            if (js_test == "test") break;
            if (i == 4) return false; // Final attempt failed
            std::this_thread::sleep_for(200ms);
        }
        
        // Verify basic DOM availability with retry
        for (int i = 0; i < 5; i++) {
            std::string dom_check = executeWrappedJS("return document !== undefined && document.body !== null;");
            if (dom_check == "true") break;
            if (i == 4) return false; // Final attempt failed  
            std::this_thread::sleep_for(200ms);
        }
        
        // Optional title check (for pages that should have titles)
        if (require_title) {
            for (int i = 0; i < 10; i++) {
                std::string title_check = executeWrappedJS("return document.title !== undefined && document.title !== null && document.title !== '';");
                if (title_check == "true") break;
                if (i == 9) return false; // Final attempt failed
                std::this_thread::sleep_for(300ms);
            }
        }
        
        return true;
    }
    
    // Enhanced page title extraction with fallback
    std::string getPageTitleReliable() {
        // Try native method first
        std::string title = browser->getPageTitle();
        if (!title.empty()) return title;
        
        // Fallback to JavaScript with retry logic
        for (int i = 0; i < 5; i++) {
            std::string js_title = executeWrappedJS("return document.title;");
            if (!js_title.empty()) return js_title;
            std::this_thread::sleep_for(200ms);
        }
        
        return ""; // Both methods failed
    }

    Browser* browser;  // Pointer to browser instance (global or individual)
    std::unique_ptr<Browser> individual_browser;  // For tests that need individual instances
    std::unique_ptr<TestHelpers::TemporaryDirectory> temp_dir;
    
    // Helper to create individual browser instance for constructor/lifecycle tests
    Browser* createIndividualBrowser() {
        HWeb::HWebConfig test_config;
        test_config.allow_data_uri = true;  // Match global browser config
        individual_browser = std::make_unique<Browser>(test_config);
        // Allow initialization time
        std::this_thread::sleep_for(800ms);
        return individual_browser.get();
    }
};

// ========== Constructor and Initialization Tests ==========

TEST_F(BrowserMainTest, ConstructorInitializesWebView) {
    // Test constructor with individual browser instance
    Browser* test_browser = createIndividualBrowser();
    EXPECT_NE(test_browser->webView, nullptr);
    EXPECT_NE(test_browser->window, nullptr);
    EXPECT_NE(test_browser->main_loop, nullptr);
}

TEST_F(BrowserMainTest, ConstructorCreatesSessionDataPath) {
    // Test with individual browser (creates session data directory)
    Browser* test_browser = createIndividualBrowser();
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
    
    // CRITICAL FIX: Use file:// URL with enhanced readiness checking
    std::string file_url = createTestPage(simple_html);
    
    // Use enhanced page loading with comprehensive readiness check
    bool page_ready = loadPageWithReadinessCheck(file_url);
    EXPECT_TRUE(page_ready) << "Page failed to load and become ready";
    
    // Use enhanced title extraction method
    std::string title = getPageTitleReliable();
    EXPECT_EQ(title, "Test Page");
    
    // Verify URL is correct
    std::string current_url = browser->getCurrentUrl();
    EXPECT_NE(current_url.find("file://"), std::string::npos);
    
    // Verify basic DOM functionality
    std::string body_content = executeWrappedJS("return document.body ? document.body.textContent.trim() : '';");
    EXPECT_EQ(body_content, "Hello World");
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
    
    // Load a page to test user agent using file:// URL
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
    
    std::string file_url = createTestPage(test_html, "ua_test.html");
    bool page_ready = loadPageWithReadinessCheck(file_url);
    EXPECT_TRUE(page_ready);
    
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
    
    std::string file_url = createTestPage(test_html, "js_test.html");
    bool page_ready = loadPageWithReadinessCheck(file_url);
    EXPECT_TRUE(page_ready);
    
    // Test JavaScript execution
    std::string result = executeWrappedJS("return 2 + 3;");
    EXPECT_EQ(result, "5");
    
    // Test DOM manipulation via JavaScript
    executeWrappedJS("document.getElementById('target').textContent = 'Modified';");
    std::this_thread::sleep_for(100ms);
    
    std::string content = browser->getInnerText("#target");
    EXPECT_EQ(content, "Modified");
}

TEST_F(BrowserMainTest, JavaScriptErrorHandling) {
    std::string test_html = "<html><body></body></html>";
    std::string file_url = createTestPage(test_html, "js_error_test.html");
    bool page_ready = loadPageWithReadinessCheck(file_url, false); // Don't require title for empty page
    EXPECT_TRUE(page_ready);
    
    // Test safe JavaScript execution with invalid syntax
    std::string result = executeWrappedJS("return invalid.syntax.here;"); // This will still fail but safely
    
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
    
    std::string file_url = createTestPage(form_html, "dom_test.html");
    bool page_ready = loadPageWithReadinessCheck(file_url);
    EXPECT_TRUE(page_ready);
    
    // Test element existence
    EXPECT_TRUE(browser->elementExists("#name-input"));
    EXPECT_TRUE(browser->elementExists("#test-btn"));
    EXPECT_FALSE(browser->elementExists("#nonexistent"));
    
    // Test filling inputs
    EXPECT_TRUE(browser->fillInput("#name-input", "John Doe"));
    EXPECT_TRUE(browser->fillInput("#email-input", "john@example.com"));
    
    // Test getting input values BEFORE button click
    std::string name_value_before = browser->getAttribute("#name-input", "value");
    EXPECT_EQ(name_value_before, "John Doe");
    
    // Test clicking
    EXPECT_TRUE(browser->clickElement("#test-btn"));
    std::this_thread::sleep_for(200ms);
    
    // Verify click effect
    std::string result = browser->getInnerText("#result");
    EXPECT_EQ(result, "Button clicked!");
    
    // Test getting input values AFTER button click
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
    
    std::string file_url = createTestPage(list_html, "count_test.html");
    bool page_ready = loadPageWithReadinessCheck(file_url);
    EXPECT_TRUE(page_ready);
    
    // Test element counting
    EXPECT_EQ(browser->countElements(".item"), 4);
    EXPECT_EQ(browser->countElements("li"), 3);
    EXPECT_EQ(browser->countElements("ul"), 1);
    EXPECT_EQ(browser->countElements(".nonexistent"), 0);
}

// ========== Navigation Tests ==========

TEST_F(BrowserMainTest, BasicNavigation) {
    // Load first page using file:// URL with enhanced readiness
    std::string page1_html = "<html><head><title>Page 1</title></head><body><h1>First Page</h1></body></html>";
    std::string page1_url = createTestPage(page1_html, "page1.html");
    
    bool page1_ready = loadPageWithReadinessCheck(page1_url);
    EXPECT_TRUE(page1_ready);
    EXPECT_EQ(getPageTitleReliable(), "Page 1");
    
    // Load second page using file:// URL with enhanced readiness
    std::string page2_html = "<html><head><title>Page 2</title></head><body><h1>Second Page</h1></body></html>";
    std::string page2_url = createTestPage(page2_html, "page2.html");
    
    bool page2_ready = loadPageWithReadinessCheck(page2_url);
    EXPECT_TRUE(page2_ready);
    EXPECT_EQ(getPageTitleReliable(), "Page 2");
    
    // Test go back with enhanced title checking
    browser->goBack();
    browser->waitForNavigation(3000);
    std::this_thread::sleep_for(1000ms); // Allow time for navigation
    EXPECT_EQ(getPageTitleReliable(), "Page 1");
    
    // Test go forward with enhanced title checking
    browser->goForward();
    browser->waitForNavigation(3000);
    std::this_thread::sleep_for(1000ms); // Allow time for navigation
    EXPECT_EQ(getPageTitleReliable(), "Page 2");
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
    
    std::string file_url = createTestPage(dynamic_html, "reload_test.html");
    bool page_ready = loadPageWithReadinessCheck(file_url);
    EXPECT_TRUE(page_ready);
    
    std::string first_timestamp = browser->getInnerText("#timestamp");
    EXPECT_FALSE(first_timestamp.empty());
    
    // Small delay to ensure different timestamp
    std::this_thread::sleep_for(100ms);
    
    // Reload page with enhanced readiness checking
    browser->reload();
    bool reload_ready = loadPageWithReadinessCheck(browser->getCurrentUrl());
    EXPECT_TRUE(reload_ready);
    
    std::string second_timestamp = browser->getInnerText("#timestamp");
    EXPECT_FALSE(second_timestamp.empty());
    
    // Timestamps should be different (page reloaded)
    EXPECT_NE(first_timestamp, second_timestamp);
}

// ========== URL Validation Tests ==========

TEST_F(BrowserMainTest, URLValidation) {
    // Test with actual working page to verify URL validation works correctly
    std::string test_html = "<html><head><title>URL Test</title></head><body>Test</body></html>";
    std::string valid_file_url = createTestPage(test_html, "url_test.html");
    
    // Valid URLs
    EXPECT_TRUE(browser->validateUrl("https://example.com"));
    EXPECT_TRUE(browser->validateUrl("http://localhost:8080"));  
    EXPECT_TRUE(browser->validateUrl(valid_file_url)); // Test our actual existing file URL
    
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
    // Load minimal empty page using file:// URL
    std::string empty_html = "<html><body></body></html>";
    std::string file_url = createTestPage(empty_html, "empty_test.html");
    bool page_ready = loadPageWithReadinessCheck(file_url, false); // Don't require title for empty page
    EXPECT_TRUE(page_ready);
    
    // These should not crash
    EXPECT_EQ(browser->countElements("div"), 0);
    EXPECT_FALSE(browser->elementExists("div"));
    EXPECT_TRUE(browser->getPageTitle().empty()); // Empty page should have empty title
    
    // JavaScript should still work
    std::string result = executeWrappedJS("return 1 + 1;");
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
    
    std::string file_url = createTestPage(complex_html, "state_test.html");
    bool page_ready = loadPageWithReadinessCheck(file_url);
    EXPECT_TRUE(page_ready);
    
    // Verify page state
    std::string ready_state = browser->getInnerText("#ready-state");
    EXPECT_EQ(ready_state, "complete");
    
    std::string location = browser->getInnerText("#location-info");
    EXPECT_NE(location.find("file://"), std::string::npos);
    
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
    std::string result = executeWrappedJS("return 42;");
    EXPECT_EQ(result, "42");
}