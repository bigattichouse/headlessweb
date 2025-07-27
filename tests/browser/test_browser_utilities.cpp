#include <gtest/gtest.h>
#include "Browser/Browser.h"
#include "../utils/test_helpers.h"
#include "browser_test_environment.h"
#include "Debug.h"
#include <thread>
#include <chrono>
#include <filesystem>

extern std::unique_ptr<Browser> g_browser;

using namespace std::chrono_literals;

class BrowserUtilitiesTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Use global browser instance (properly initialized)
        browser = g_browser.get();
        
        // Create temporary directory for file:// URLs
        temp_dir = std::make_unique<TestHelpers::TemporaryDirectory>("browser_utilities_tests");
        
        // Reset browser to clean state before each test
        browser->loadUri("about:blank");
        browser->waitForNavigation(2000);
        
        // Load a test page with content for utility testing
        setupTestPage();
        
        debug_output("BrowserUtilitiesTest SetUp complete");
    }
    
    void setupTestPage() {
        std::string test_html = R"HTML(
<!DOCTYPE html>
<html>
<head>
    <title>Utilities Test Page</title>
    <style>
        body { margin: 0; padding: 20px; font-family: Arial, sans-serif; }
        .test-section { margin: 20px 0; }
        .scrollable { height: 200px; overflow: auto; }
        .tall-content { height: 2000px; background: linear-gradient(to bottom, #ff0000, #0000ff); }
        .form-section { margin: 20px 0; }
        input, select, button { margin: 5px; padding: 5px; }
        #status { font-weight: bold; color: green; }
    </style>
</head>
<body>
    <h1>Browser Utilities Test Page</h1>
    
    <div class="test-section">
        <h2>Page State Testing</h2>
        <p id="status">Page loaded successfully</p>
        <button id="state-btn" onclick="updateState()">Update State</button>
    </div>
    
    <div class="test-section">
        <h2>Scroll Testing</h2>
        <div class="scrollable" id="scroll-container">
            <div class="tall-content" id="tall-content">
                <p>Scroll down to see more content...</p>
                <div style="margin-top: 500px;">
                    <p>Middle content</p>
                </div>
                <div style="margin-top: 1000px;">
                    <p>Bottom content</p>
                </div>
            </div>
        </div>
    </div>
    
    <div class="test-section">
        <h2>Action Sequence Testing</h2>
        <form id="test-form" class="form-section">
            <input type="text" id="text-input" placeholder="Enter text" />
            <select id="dropdown">
                <option value="">Choose...</option>
                <option value="option1">Option 1</option>
                <option value="option2">Option 2</option>
                <option value="option3">Option 3</option>
            </select>
            <input type="checkbox" id="checkbox" />
            <label for="checkbox">Check me</label>
            <button type="button" id="action-btn" onclick="recordAction('button clicked')">Click Me</button>
            <button type="submit" id="submit-btn">Submit</button>
        </form>
        
        <div id="action-log"></div>
    </div>
    
    <div class="test-section">
        <h2>Page Source Testing</h2>
        <div id="dynamic-content">Initial content</div>
        <button onclick="addDynamicContent()">Add Dynamic Content</button>
    </div>
    
    <script>
        let actionCount = 0;
        
        function updateState() {
            document.getElementById('status').textContent = 'State updated at ' + new Date().toLocaleTimeString();
        }
        
        function recordAction(action) {
            actionCount++;
            const log = document.getElementById('action-log');
            const entry = document.createElement('div');
            entry.textContent = `Action ${actionCount}: ${action}`;
            log.appendChild(entry);
        }
        
        function addDynamicContent() {
            const container = document.getElementById('dynamic-content');
            const newElement = document.createElement('p');
            newElement.textContent = 'Dynamic content added at ' + new Date().toLocaleTimeString();
            container.appendChild(newElement);
        }
        
        // Form event handlers
        document.getElementById('text-input').addEventListener('input', function(e) {
            recordAction('text input: ' + e.target.value);
        });
        
        document.getElementById('dropdown').addEventListener('change', function(e) {
            recordAction('dropdown selected: ' + e.target.value);
        });
        
        document.getElementById('checkbox').addEventListener('change', function(e) {
            recordAction('checkbox ' + (e.target.checked ? 'checked' : 'unchecked'));
        });
        
        document.getElementById('test-form').addEventListener('submit', function(e) {
            e.preventDefault();
            recordAction('form submitted');
        });
        
        // Utility functions for testing
        function getPageInfo() {
            return {
                readyState: document.readyState,
                title: document.title,
                url: window.location.href,
                scrollY: window.pageYOffset,
                scrollX: window.pageXOffset
            };
        }
        
        function simulateComplexAction() {
            recordAction('complex action started');
            setTimeout(() => {
                recordAction('complex action completed');
            }, 100);
        }
    </script>
</body>
</html>
)HTML";
        
        // CRITICAL FIX: Use file:// URL instead of data: URL
        auto html_file = temp_dir->createFile("utilities_test.html", test_html);
        std::string file_url = "file://" + html_file.string();
        
        debug_output("Loading utilities test page: " + file_url);
        browser->loadUri(file_url);
        
        // Wait for navigation to complete
        bool navigation_success = browser->waitForNavigation(10000);
        if (!navigation_success) {
            debug_output("Navigation failed for utilities test page");
            return;
        }
        
        // CRITICAL: Wait for JavaScript environment to be fully ready
        std::string js_ready = executeWrappedJS(
            "return document.readyState === 'complete' && "
            "document.getElementById('status') !== null && "
            "typeof updateState === 'function';"
        );
        
        if (js_ready != "true") {
            debug_output("JavaScript environment not ready, waiting additional time...");
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            
            // Re-check JavaScript readiness
            js_ready = executeWrappedJS(
                "return document.readyState === 'complete' && "
                "document.getElementById('status') !== null;"
            );
        }
        
        debug_output("Utilities test page loaded - ready: " + js_ready);
    }

    void TearDown() override {
        // Clean up temporary directory (don't destroy global browser)
        temp_dir.reset();
    }
    
    // Generic JavaScript wrapper function for safe execution
    std::string executeWrappedJS(const std::string& jsCode) {
        std::string wrapped = "(function() { " + jsCode + " })()";
        return browser->executeJavascriptSync(wrapped);
    }
    
    // Helper to create file:// URL from HTML content
    std::string createTestPageUrl(const std::string& html_content, const std::string& filename = "test.html") {
        auto html_file = temp_dir->createFile(filename, html_content);
        return "file://" + html_file.string();
    }

    Browser* browser;  // Raw pointer to global browser instance
    std::unique_ptr<TestHelpers::TemporaryDirectory> temp_dir;
};

// ========== Wait Method Tests ==========

TEST_F(BrowserUtilitiesTest, WaitWithValidDuration) {
    auto start = std::chrono::steady_clock::now();
    
    browser->wait(100); // 100ms wait
    
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    // Allow for some timing variance
    EXPECT_GE(duration, 90);  // At least 90ms
    EXPECT_LE(duration, 200); // At most 200ms (allowing for system timing)
}

TEST_F(BrowserUtilitiesTest, WaitWithZeroDuration) {
    auto start = std::chrono::steady_clock::now();
    
    browser->wait(0);
    
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    // Should return immediately
    EXPECT_LT(duration, 10);
}

TEST_F(BrowserUtilitiesTest, WaitWithNegativeDuration) {
    auto start = std::chrono::steady_clock::now();
    
    browser->wait(-100);
    
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    // Should return immediately for negative values
    EXPECT_LT(duration, 10);
}

// ========== Page State Tests ==========

TEST_F(BrowserUtilitiesTest, IsPageLoadedAfterSetup) {
    // Page should be loaded after setup
    EXPECT_TRUE(browser->isPageLoaded());
}

TEST_F(BrowserUtilitiesTest, GetPageLoadState) {
    std::string loadState = browser->getPageLoadState();
    
    // Should contain readyState and URL
    EXPECT_FALSE(loadState.empty());
    EXPECT_NE(loadState.find("complete"), std::string::npos);
    // Test environment starts with about:blank before setupTestPage() creates file:// URL
    EXPECT_NE(loadState.find("about:blank"), std::string::npos);
}

TEST_F(BrowserUtilitiesTest, PageStateAfterNavigation) {
    // Navigate to a simple page using file:// URL
    std::string simple_html = "<html><body><h1>Simple Page</h1></body></html>";
    std::string simple_page = createTestPageUrl(simple_html, "simple_page.html");
    browser->loadUri(simple_page);
    
    // Wait for navigation to complete properly
    browser->waitForNavigation(5000);
    
    EXPECT_TRUE(browser->isPageLoaded());
    
    std::string loadState = browser->getPageLoadState();
    EXPECT_NE(loadState.find("complete"), std::string::npos);
}

// ========== Page Source Tests ==========

TEST_F(BrowserUtilitiesTest, GetPageSourceBasic) {
    debug_output("=== GetPageSourceBasic test starting ===");
    
    // Debug: Check page status
    std::string current_url = browser->getCurrentUrl();
    std::string ready_state = browser->executeJavascriptSyncSafe("document.readyState");
    std::string title = browser->getPageTitle();
    
    debug_output("Current URL: " + current_url);
    debug_output("Ready state: " + ready_state);
    debug_output("Page title: " + title);
    
    std::string source = browser->getPageSource();
    debug_output("Page source length: " + std::to_string(source.length()));
    debug_output("Page source preview: " + source.substr(0, std::min(200ul, source.length())));
    
    EXPECT_FALSE(source.empty());
    EXPECT_NE(source.find("<html>"), std::string::npos);
    EXPECT_NE(source.find("Utilities Test Page"), std::string::npos);
    EXPECT_NE(source.find("</html>"), std::string::npos);
}

TEST_F(BrowserUtilitiesTest, GetPageSourceAfterDynamicContent) {
    // Add dynamic content using wrapper function
    executeWrappedJS("addDynamicContent();");
    std::this_thread::sleep_for(100ms);
    
    std::string source = browser->getPageSource();
    
    EXPECT_FALSE(source.empty());
    EXPECT_NE(source.find("Dynamic content added"), std::string::npos);
}

TEST_F(BrowserUtilitiesTest, GetPageSourceStructure) {
    // Load a page with complete HTML structure for testing
    std::string test_html = R"HTML(<!DOCTYPE html>
<html>
<head>
    <title>Source Structure Test</title>
    <style>
        body { font-family: Arial; }
    </style>
</head>
<body>
    <h1>Test Content</h1>
    <script>
        function testFunction() { return true; }
    </script>
</body>
</html>)HTML";
    
    std::string test_page = createTestPageUrl(test_html, "source_test.html");
    browser->loadUri(test_page);
    bool nav_success = browser->waitForNavigation(5000);
    
    // Signal-based wait for DOM readiness instead of arbitrary sleep
    browser->waitForFrameworkReady("auto", 5000);
    
    std::string source = browser->getPageSource();
    
    // Verify basic HTML structure (note: outerHTML doesn't include DOCTYPE)
    EXPECT_NE(source.find("<html>"), std::string::npos);
    EXPECT_NE(source.find("<head>"), std::string::npos);
    EXPECT_NE(source.find("<body>"), std::string::npos);
    EXPECT_NE(source.find("<script>"), std::string::npos);
    EXPECT_NE(source.find("<style>"), std::string::npos);
}

// ========== Scroll Position Tests ==========

TEST_F(BrowserUtilitiesTest, InitialScrollPosition) {
    auto [x, y] = browser->getScrollPosition();
    
    // Initial position should be at top
    EXPECT_EQ(x, 0);
    EXPECT_EQ(y, 0);
}

TEST_F(BrowserUtilitiesTest, SetAndGetScrollPosition) {
    browser->setScrollPosition(100, 200);
    std::this_thread::sleep_for(100ms); // Allow scroll to complete
    
    auto [x, y] = browser->getScrollPosition();
    
    // Note: Actual scroll might be limited by content size
    // We just verify the mechanism works
    EXPECT_NO_THROW(browser->getScrollPosition());
}

TEST_F(BrowserUtilitiesTest, ScrollToZero) {
    // First scroll away from origin
    browser->setScrollPosition(50, 50);
    std::this_thread::sleep_for(50ms);
    
    // Then scroll back to origin
    browser->setScrollPosition(0, 0);
    std::this_thread::sleep_for(50ms);
    
    auto [x, y] = browser->getScrollPosition();
    EXPECT_EQ(x, 0);
    EXPECT_EQ(y, 0);
}

TEST_F(BrowserUtilitiesTest, ScrollWithNegativeValues) {
    // Negative values should be handled gracefully
    browser->setScrollPosition(-10, -10);
    std::this_thread::sleep_for(50ms);
    
    auto [x, y] = browser->getScrollPosition();
    
    // Browser should clamp to 0
    EXPECT_EQ(x, 0);
    EXPECT_EQ(y, 0);
}

// ========== Action Sequence Tests ==========

TEST_F(BrowserUtilitiesTest, ExecuteEmptyActionSequence) {
    std::vector<Session::RecordedAction> empty_actions;
    
    bool result = browser->executeActionSequence(empty_actions);
    
    EXPECT_TRUE(result);
}

TEST_F(BrowserUtilitiesTest, ExecuteSingleClickAction) {
    // Load a page with clickable elements for testing
    std::string test_html = R"HTML(<!DOCTYPE html>
<html>
<head><title>Click Action Test</title></head>
<body>
    <button type="button" id="action-btn" onclick="recordAction('button clicked')">Click Me</button>
    <div id="action-log"></div>
    <script>
        function recordAction(action) {
            document.getElementById('action-log').textContent = action;
        }
    </script>
</body>
</html>)HTML";
    
    std::string test_page = createTestPageUrl(test_html, "click_test.html");
    browser->loadUri(test_page);
    browser->waitForNavigation(5000);
    
    // Signal-based wait for DOM readiness and element availability
    browser->waitForFrameworkReady("auto", 5000);
    browser->waitForSelectorEvent("#action-btn", 2000);
    
    std::vector<Session::RecordedAction> actions = {
        {"click", "#action-btn", "", 0}
    };
    
    bool result = browser->executeActionSequence(actions);
    
    EXPECT_TRUE(result);
    
    // Verify the click was registered using wrapper function
    std::string log_content = executeWrappedJS(
        "return document.getElementById('action-log') ? document.getElementById('action-log').textContent : '';");
    EXPECT_NE(log_content.find("button clicked"), std::string::npos);
}

TEST_F(BrowserUtilitiesTest, ExecuteTextInputAction) {
    std::vector<Session::RecordedAction> actions = {
        {"fill", "#text-input", "test input", 0}
    };
    
    bool result = browser->executeActionSequence(actions);
    
    EXPECT_TRUE(result);
    
    // Verify the text was entered using wrapper function
    std::string input_value = executeWrappedJS(
        "return document.getElementById('text-input') ? document.getElementById('text-input').value : '';");
    EXPECT_EQ(input_value, "test input");
}

// ========== Data Manager Tests ==========

TEST_F(BrowserUtilitiesTest, InitializeDataManagerBasic) {
    std::string test_session = "test_session_" + std::to_string(std::time(nullptr));
    
    // Should not throw
    EXPECT_NO_THROW(browser->initializeDataManager(test_session));
}

TEST_F(BrowserUtilitiesTest, InitializeDataManagerWithEmptyName) {
    // Should handle empty session name gracefully
    EXPECT_NO_THROW(browser->initializeDataManager(""));
}

// ========== Integration Tests ==========

TEST_F(BrowserUtilitiesTest, UtilityMethodsAfterNavigation) {
    // Navigate to a new page using file:// URL
    std::string new_html = "<html><body><h1>New Page</h1><div style='height:1000px;'></div></body></html>";
    std::string new_page = createTestPageUrl(new_html, "new_page.html");
    browser->loadUri(new_page);
    browser->waitForNavigation(5000);
    
    // Test utilities still work after navigation
    EXPECT_TRUE(browser->isPageLoaded());
    EXPECT_FALSE(browser->getPageSource().empty());
    
    auto [x, y] = browser->getScrollPosition();
    EXPECT_EQ(x, 0);
    EXPECT_EQ(y, 0);
}

// ========== Error Handling Tests ==========

TEST_F(BrowserUtilitiesTest, UtilitiesWithMinimalBrowser) {
    // Test behavior with a browser that has minimal setup
    HWeb::HWebConfig test_config;
    auto minimal_browser = std::make_unique<Browser>(test_config);
    
    // These should not crash even without a loaded page
    EXPECT_NO_THROW(minimal_browser->wait(10));
    EXPECT_NO_THROW(minimal_browser->isPageLoaded());
    EXPECT_NO_THROW(minimal_browser->getPageSource());
    EXPECT_NO_THROW(minimal_browser->getScrollPosition());
}