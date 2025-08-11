#include <gtest/gtest.h>
#include "Browser/Browser.h"
#include "Session/Session.h"
#include "../utils/test_helpers.h"
#include "browser_test_environment.h"
#include "Debug.h"
#include <memory>
#include <chrono>
#include <thread>

extern std::unique_ptr<Browser> g_browser;

class MinimalSegfaultDebugTest : public ::testing::Test {
protected:
    void SetUp() override {
        debug_output("=== MinimalSegfaultDebugTest::SetUp START ===");
        
        // Create temporary directory for file:// URLs
        temp_dir = std::make_unique<TestHelpers::TemporaryDirectory>("minimal_segfault_tests");
        
        // Get browser instance
        browser_ = g_browser.get();
        ASSERT_NE(browser_, nullptr) << "Global browser should be initialized";
        
        // Basic browser state check
        debug_output("Browser pointer valid, checking basic operations...");
        
        // Test the absolute most basic operation
        EXPECT_NO_THROW({
            std::string url = browser_->getCurrentUrl();
            debug_output("getCurrentUrl succeeded: " + url);
        }) << "getCurrentUrl should not crash";
        
        debug_output("=== MinimalSegfaultDebugTest::SetUp END ===");
    }
    
    void TearDown() override {
        debug_output("=== MinimalSegfaultDebugTest::TearDown START ===");
        // Minimal cleanup
        temp_dir.reset();
        debug_output("=== MinimalSegfaultDebugTest::TearDown END ===");
    }

    // Generic JavaScript wrapper function for safe execution (copied from DOMEscapingFixesTest)
    std::string executeWrappedJS(const std::string& jsCode) {
        if (!browser_) return "";
        try {
            std::string wrapped = "(function() { try { " + jsCode + " } catch(e) { return ''; } })()";
            return browser_->executeJavascriptSync(wrapped);
        } catch (const std::exception& e) {
            debug_output("JavaScript execution error: " + std::string(e.what()));
            return "";
        }
    }

    std::unique_ptr<TestHelpers::TemporaryDirectory> temp_dir;
    Browser* browser_;
};

// Test 1: Absolute minimum - just check browser exists
TEST_F(MinimalSegfaultDebugTest, JustCheckBrowserExists) {
    debug_output("=== TEST: JustCheckBrowserExists START ===");
    
    EXPECT_NE(browser_, nullptr);
    debug_output("Browser pointer is valid");
    
    // Try the absolute most basic operation
    std::string current_url;
    EXPECT_NO_THROW({
        current_url = browser_->getCurrentUrl();
    }) << "getCurrentUrl should not crash";
    
    debug_output("getCurrentUrl result: '" + current_url + "'");
    debug_output("=== TEST: JustCheckBrowserExists SUCCESS ===");
}

// Test 1B: Mimic the failing DOMEscapingFixesTest setup exactly
TEST_F(MinimalSegfaultDebugTest, MimicDOMEscapingTest) {
    debug_output("=== TEST: MimicDOMEscapingTest START ===");
    
    // Try to replicate the exact sequence that DOMEscapingFixesTest does
    ASSERT_NE(browser_, nullptr) << "Browser should not be null";
    
    // Test basic browser operation first
    EXPECT_NO_THROW({
        std::string url = browser_->getCurrentUrl();
        debug_output("getCurrentUrl succeeded: " + url);
    }) << "getCurrentUrl should not crash";
    
    // Load a basic page first to provide JavaScript context
    debug_output("Loading about:blank for JavaScript context...");
    browser_->loadUri("about:blank");
    bool nav_ready = browser_->waitForNavigation(2000);
    debug_output("Basic navigation ready: " + std::string(nav_ready ? "true" : "false"));
    
    // Try basic JavaScript execution
    std::string basic_result = executeWrappedJS("'test_basic'");
    debug_output("Basic JS result: " + basic_result);
    EXPECT_EQ(basic_result, "test_basic") << "Basic JavaScript should work";
    
    // Create HTML file exactly like DOMEscapingFixesTest
    std::string test_html = R"(<!DOCTYPE html>
<html><body>
    <input type="text" id="text-input" placeholder="Enter text">
</body></html>)";
    
    auto html_file = temp_dir->createFile("mimic_test.html", test_html);
    std::string file_url = "file://" + html_file.string();
    debug_output("Created test file: " + file_url);
    
    // This is where the segfault might happen - loadUri
    debug_output("About to call loadUri...");
    EXPECT_NO_THROW({
        browser_->loadUri(file_url);
        debug_output("loadUri succeeded");
    }) << "loadUri should not crash";
    
    // This is another potential segfault point - waitForNavigation
    debug_output("About to call waitForNavigation...");
    bool nav_result = false;
    EXPECT_NO_THROW({
        nav_result = browser_->waitForNavigation(5000);
        debug_output("waitForNavigation result: " + std::string(nav_result ? "true" : "false"));
    }) << "waitForNavigation should not crash";
    
    if (nav_result) {
        // Try element operations
        debug_output("About to check element existence...");
        bool element_exists = browser_->elementExists("#text-input");
        debug_output("Element exists: " + std::string(element_exists ? "true" : "false"));
        
        if (element_exists) {
            debug_output("About to fill input...");
            bool fill_result = browser_->fillInput("#text-input", "test value");
            debug_output("Fill input result: " + std::string(fill_result ? "true" : "false"));
        }
    }
    
    debug_output("=== TEST: MimicDOMEscapingTest SUCCESS ===");
}

// Test 2: Load about:blank (should be safe) - using event-driven approach
TEST_F(MinimalSegfaultDebugTest, LoadAboutBlank) {
    debug_output("=== TEST: LoadAboutBlank START ===");
    
    EXPECT_NO_THROW({
        browser_->loadUri("about:blank");
        debug_output("loadUri('about:blank') called successfully");
    }) << "loadUri should not crash";
    
    // EVENT-DRIVEN FIX: Use browser event system instead of blocking wait
    if (browser_->event_bus_) {
        debug_output("Using event bus for navigation waiting");
        auto nav_future = browser_->event_bus_->waitForNavigation(5000);
        if (nav_future.wait_for(std::chrono::milliseconds(5000)) == std::future_status::ready) {
            auto nav_event = nav_future.get();
            debug_output("Navigation completed via event system");
        } else {
            debug_output("Navigation timeout via event system");
        }
    } else {
        debug_output("Event bus not available, using basic approach");
        // Fallback: basic signal-based wait with timeout
        bool nav_result = browser_->waitForNavigation(2000);
        debug_output("waitForNavigation result: " + std::string(nav_result ? "true" : "false"));
    }
    
    debug_output("=== TEST: LoadAboutBlank SUCCESS ===");
}

// Test 3: Try basic JavaScript execution (this is where segfaults often happen)
TEST_F(MinimalSegfaultDebugTest, BasicJavaScriptExecution) {
    debug_output("=== TEST: BasicJavaScriptExecution START ===");
    
    // First load a page using event-driven approach
    browser_->loadUri("about:blank");
    
    // EVENT-DRIVEN FIX: Wait for browser readiness instead of fixed navigation wait
    bool page_ready = false;
    if (browser_->readiness_tracker_) {
        debug_output("Using readiness tracker for page preparation");
        auto ready_future = browser_->readiness_tracker_->waitForBasicReadiness(5000);
        if (ready_future.wait_for(std::chrono::milliseconds(5000)) == std::future_status::ready) {
            page_ready = ready_future.get();
            debug_output("Page readiness: " + std::string(page_ready ? "true" : "false"));
        }
    } else {
        debug_output("Readiness tracker not available, using fallback");
        page_ready = browser_->waitForNavigation(2000);
    }
    
    if (page_ready) {
        debug_output("Page ready, attempting JavaScript execution...");
        
        std::string js_result;
        EXPECT_NO_THROW({
            js_result = browser_->executeJavascriptSync("return 'hello';");
            debug_output("JavaScript result: '" + js_result + "'");
        }) << "Basic JavaScript execution should not crash";
        
        EXPECT_EQ(js_result, "hello") << "JavaScript should return 'hello'";
    } else {
        debug_output("Page not ready, skipping JavaScript test");
    }
    
    debug_output("=== TEST: BasicJavaScriptExecution SUCCESS ===");
}

// Test 4: Try loading a simple HTML file using event-driven approach
TEST_F(MinimalSegfaultDebugTest, LoadSimpleHTMLFile) {
    debug_output("=== TEST: LoadSimpleHTMLFile START ===");
    
    // DIAGNOSTIC: Check event system initialization
    debug_output("Event system diagnostic:");
    debug_output("  event_bus_: " + std::string(browser_->event_bus_ ? "INITIALIZED" : "NULL"));
    debug_output("  readiness_tracker_: " + std::string(browser_->readiness_tracker_ ? "INITIALIZED" : "NULL"));
    debug_output("  state_manager_: " + std::string(browser_->state_manager_ ? "INITIALIZED" : "NULL"));
    debug_output("  async_dom_: " + std::string(browser_->async_dom_ ? "INITIALIZED" : "NULL"));
    
    // Create minimal HTML
    std::string simple_html = R"(<!DOCTYPE html>
<html><head><title>Test</title></head>
<body>
    <div id="test-element">Hello World</div>
</body></html>)";
    
    // Create file
    auto html_file = temp_dir->createFile("simple_test.html", simple_html);
    std::string file_url = "file://" + html_file.string();
    
    debug_output("Loading HTML file: " + file_url);
    
    EXPECT_NO_THROW({
        browser_->loadUri(file_url);
        debug_output("loadUri for HTML file called successfully");
    }) << "Loading HTML file should not crash";
    
    // EVENT-DRIVEN DIAGNOSTIC: Try different approaches to understand what works
    bool page_ready = false;
    
    if (browser_->readiness_tracker_) {
        debug_output("Attempting to use readiness tracker...");
        try {
            auto ready_future = browser_->readiness_tracker_->waitForFullReadiness(3000);
            debug_output("Future created, waiting for result...");
            
            auto status = ready_future.wait_for(std::chrono::milliseconds(3000));
            debug_output("Wait status: " + std::string(status == std::future_status::ready ? "ready" : 
                        status == std::future_status::timeout ? "timeout" : "deferred"));
            
            if (status == std::future_status::ready) {
                page_ready = ready_future.get();
                debug_output("Readiness tracker result: " + std::string(page_ready ? "true" : "false"));
            } else {
                debug_output("Readiness tracker timed out, falling back to navigation");
                page_ready = browser_->waitForNavigation(3000);
            }
        } catch (const std::exception& e) {
            debug_output("Readiness tracker exception: " + std::string(e.what()));
            page_ready = browser_->waitForNavigation(3000);
        }
    } else {
        debug_output("Readiness tracker not available, using navigation fallback");
        page_ready = browser_->waitForNavigation(3000);
        debug_output("Navigation result for HTML file: " + std::string(page_ready ? "true" : "false"));
    }
    
    if (page_ready) {
        // Use the most reliable approach - direct JavaScript checking
        debug_output("Page ready, checking for DOM elements with JavaScript validation");
        
        std::string element_check = browser_->executeJavascriptSync(
            "(function() { return document.querySelector('#test-element') !== null && document.readyState === 'complete'; })()");
        
        bool element_ready = (element_check == "true");
        debug_output("Element readiness via JavaScript: " + std::string(element_ready ? "true" : "false"));
        
        if (element_ready) {
            EXPECT_NO_THROW({
                bool element_exists = browser_->elementExists("#test-element");
                debug_output("Element exists check: " + std::string(element_exists ? "true" : "false"));
            }) << "Element existence check should not crash";
        } else {
            debug_output("Element not ready via JavaScript check");
        }
    }
    
    debug_output("=== TEST: LoadSimpleHTMLFile SUCCESS ===");
}

// Test 5: Try a fillInput operation using event-driven approach
TEST_F(MinimalSegfaultDebugTest, BasicFillInputOperation) {
    debug_output("=== TEST: BasicFillInputOperation START ===");
    
    // Create HTML with input field
    std::string input_html = R"(<!DOCTYPE html>
<html><head><title>Input Test</title></head>
<body>
    <input type="text" id="test-input" value="">
</body></html>)";
    
    auto html_file = temp_dir->createFile("input_test.html", input_html);
    std::string file_url = "file://" + html_file.string();
    
    debug_output("Loading input test page: " + file_url);
    
    // EVENT-DRIVEN FIX: Use full page readiness instead of basic navigation
    browser_->loadUri(file_url);
    
    bool page_ready = false;
    
    if (browser_->readiness_tracker_) {
        debug_output("Using readiness tracker for input page");
        auto ready_future = browser_->readiness_tracker_->waitForFullReadiness(8000);
        if (ready_future.wait_for(std::chrono::milliseconds(8000)) == std::future_status::ready) {
            page_ready = ready_future.get();
            debug_output("Input page readiness: " + std::string(page_ready ? "true" : "false"));
        }
    } else {
        debug_output("Readiness tracker not available, using navigation fallback");
        page_ready = browser_->waitForNavigation(3000);
    }
    
    ASSERT_TRUE(page_ready) << "Page should load successfully via event system";
    
    // EVENT-DRIVEN FIX: Replace sleep with JavaScript-based element readiness check
    std::string input_ready_check = browser_->executeJavascriptSync(
        "return document.querySelector('#test-input') !== null && document.readyState === 'complete';");
    bool input_ready = (input_ready_check == "true");
    ASSERT_TRUE(input_ready) << "Input element should be ready";
    
    // Check if element exists with event-driven verification
    bool input_exists = false;
    EXPECT_NO_THROW({
        input_exists = browser_->elementExists("#test-input");
        debug_output("Input element exists: " + std::string(input_exists ? "true" : "false"));
    }) << "Element existence check should not crash";
    
    if (input_exists) {
        // EVENT-DRIVEN FIX: Use async DOM operations if available
        bool fill_result = false;
        
        if (browser_->async_dom_) {
            debug_output("Using async DOM operations for fillInput");
            auto fill_future = browser_->async_dom_->fillInputAsync("#test-input", "test value", 5000);
            if (fill_future.wait_for(std::chrono::milliseconds(5000)) == std::future_status::ready) {
                fill_result = fill_future.get();
                debug_output("Async fillInput result: " + std::string(fill_result ? "true" : "false"));
            }
        } else {
            debug_output("Async DOM operations not available, using synchronous fillInput");
            EXPECT_NO_THROW({
                fill_result = browser_->fillInput("#test-input", "test value");
                debug_output("fillInput result: " + std::string(fill_result ? "true" : "false"));
            }) << "fillInput should not crash";
        }
        
        EXPECT_TRUE(fill_result) << "fillInput should succeed";
        
        // EVENT-DRIVEN FIX: Verify input value with JavaScript check
        std::string value_check = browser_->executeJavascriptSync(
            "return document.querySelector('#test-input').value === 'test value';");
        bool value_set = (value_check == "true");
        debug_output("Input value verification: " + std::string(value_set ? "true" : "false"));
        EXPECT_TRUE(value_set) << "Input value should be set correctly";
    }
    
    debug_output("=== TEST: BasicFillInputOperation SUCCESS ===");
}