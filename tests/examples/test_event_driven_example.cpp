// PHASE 6 EXAMPLE: Event-driven test patterns replacing blocking waits
// This file demonstrates how to replace blocking patterns in tests with condition-based waiting

#include <gtest/gtest.h>
#include "Browser/Browser.h"
#include "../utils/TestWaitUtilities.h"
#include "../utils/test_helpers.h"
#include "browser_test_environment.h"
#include "Debug.h"
#include <memory>

extern std::unique_ptr<Browser> g_browser;

class EventDrivenTestExample : public ::testing::Test {
protected:
    void SetUp() override {
        TEST_SCOPE("EventDrivenTestExample SetUp");
        
        browser_ = g_browser.get();
        temp_dir = std::make_unique<TestHelpers::TemporaryDirectory>("event_driven_tests");
        
        // OLD PATTERN: Fixed delay after loading
        // browser_->loadUri("about:blank");
        // std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        
        // NEW PATTERN: Condition-based waiting
        browser_->loadUri("about:blank");
        ASSERT_TRUE(WAIT_FOR(
            browser_->waitForNavigation(100), // Quick check
            5000 // Overall timeout
        )) << "Failed to load blank page";
        
        // Wait for browser to be fully ready instead of fixed delay
        ASSERT_TRUE(TestUtils::TestWaitUtilities::waitForBrowserReady(10000))
            << "Browser not ready after setup";
    }
    
    void TearDown() override {
        temp_dir.reset();
        YIELD_TO_BROWSER(); // Replace fixed delays in cleanup
    }

    Browser* browser_;
    std::unique_ptr<TestHelpers::TemporaryDirectory> temp_dir;
    
    // OLD PATTERN: Multiple retry loops with fixed sleeps
    bool loadPageWithReadinessCheck_OLD(const std::string& url) {
        browser_->loadUri(url);
        
        // Navigation wait
        bool nav_success = browser_->waitForNavigation(5000);
        if (!nav_success) return false;
        
        // BLOCKING: Fixed processing time
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        
        // BLOCKING: JavaScript execution retry with fixed delays
        for (int i = 0; i < 5; i++) {
            std::string js_test = browser_->executeJavascriptSync("return 'test';");
            if (js_test == "test") break;
            if (i == 4) return false;
            std::this_thread::sleep_for(std::chrono::milliseconds(200)); // BLOCKING
        }
        
        // BLOCKING: DOM ready check with fixed delays
        for (int i = 0; i < 5; i++) {
            std::string dom_check = browser_->executeJavascriptSync("return document.readyState === 'complete';");
            if (dom_check == "true") break;
            if (i == 4) return false;
            std::this_thread::sleep_for(std::chrono::milliseconds(200)); // BLOCKING
        }
        
        return true;
    }
    
    // NEW PATTERN: Event-driven condition waiting
    bool loadPageWithReadinessCheck_NEW(const std::string& url) {
        browser_->loadUri(url);
        
        // Navigation wait (unchanged - already event-driven)
        if (!browser_->waitForNavigation(5000)) {
            return false;
        }
        
        // REPLACED: Condition-based JavaScript readiness check
        if (!TestUtils::TestWaitUtilities::waitForJavaScriptReady(5000)) {
            return false;
        }
        
        // REPLACED: Condition-based DOM ready check
        if (!TestUtils::TestWaitUtilities::waitForDOMReady(5000)) {
            return false;
        }
        
        return true;
    }
};

// Test demonstrating form interaction improvements
TEST_F(EventDrivenTestExample, FormInteractionReplacements) {
    TEST_SCOPE("FormInteractionReplacements");
    
    // Create a test form
    std::string html_content = R"(
        <!DOCTYPE html>
        <html>
        <body>
            <form id="test-form">
                <input type="text" id="name-input" name="name">
                <input type="checkbox" id="agree-checkbox" name="agree">
                <button type="submit" id="submit-btn">Submit</button>
            </form>
            <div id="result"></div>
        </body>
        </html>
    )";
    
    std::string filepath = temp_dir->createFileWithContent("form_test.html", html_content);
    std::string file_url = "file://" + filepath;
    
    // Load page with new pattern
    ASSERT_TRUE(loadPageWithReadinessCheck_NEW(file_url))
        << "Failed to load form test page";
    
    // OLD PATTERN: Fill input with fixed delays
    // browser_->fillInput("#name-input", "Test User");
    // std::this_thread::sleep_for(std::chrono::milliseconds(500)); // BLOCKING
    // browser_->checkElement("#agree-checkbox");
    // std::this_thread::sleep_for(std::chrono::milliseconds(300)); // BLOCKING
    
    // NEW PATTERN: Fill input with completion waiting
    browser_->fillInput("#name-input", "Test User");
    ASSERT_TRUE(TestUtils::TestWaitUtilities::waitForInputFilled("#name-input", 3000))
        << "Input fill not completed";
    
    browser_->checkElement("#agree-checkbox");
    ASSERT_TRUE(WAIT_FOR(
        browser_->executeJavascriptSync("return document.querySelector('#agree-checkbox').checked;") == "true",
        3000
    )) << "Checkbox not checked";
    
    // OLD PATTERN: Form submission with fixed delay
    // browser_->clickElement("#submit-btn");
    // std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // BLOCKING
    
    // NEW PATTERN: Form submission with completion detection
    browser_->clickElement("#submit-btn");
    ASSERT_TRUE(TestUtils::TestWaitUtilities::waitForFormSubmitted("#test-form", 10000))
        << "Form submission not completed";
}

// Test demonstrating download operation improvements
TEST_F(EventDrivenTestExample, DownloadOperationReplacements) {
    TEST_SCOPE("DownloadOperationReplacements");
    
    // Create a download test page
    std::string html_content = R"(
        <!DOCTYPE html>
        <html>
        <body>
            <a href="data:text/plain;base64,SGVsbG8gV29ybGQ=" download="test.txt" id="download-link">
                Download Test File
            </a>
        </body>
        </html>
    )";
    
    std::string filepath = temp_dir->createFileWithContent("download_test.html", html_content);
    std::string file_url = "file://" + filepath;
    
    ASSERT_TRUE(loadPageWithReadinessCheck_NEW(file_url))
        << "Failed to load download test page";
    
    // OLD PATTERN: Download with polling and fixed delays
    // browser_->clickElement("#download-link");
    // std::this_thread::sleep_for(std::chrono::milliseconds(2000)); // BLOCKING
    // 
    // // Polling for download completion
    // bool download_found = false;
    // for (int i = 0; i < 30; i++) {
    //     // Check for download file existence
    //     if (/* check file exists */) {
    //         download_found = true;
    //         break;
    //     }
    //     std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // BLOCKING
    // }
    
    // NEW PATTERN: Event-driven download detection
    browser_->clickElement("#download-link");
    
    // Wait for download initiation
    YIELD_TO_BROWSER(); // Brief synchronization instead of fixed delay
    
    // Event-driven download completion waiting
    ASSERT_TRUE(TestUtils::TestWaitUtilities::waitForDownloadComplete("test.txt", 30000))
        << "Download not completed within timeout";
}

// Test demonstrating network operation improvements  
TEST_F(EventDrivenTestExample, NetworkOperationReplacements) {
    TEST_SCOPE("NetworkOperationReplacements");
    
    // Create a page that makes network requests
    std::string html_content = R"(
        <!DOCTYPE html>
        <html>
        <body>
            <button id="fetch-btn" onclick="
                fetch('data:text/plain,response')
                    .then(r => r.text())
                    .then(data => document.getElementById('result').textContent = data);
            ">Fetch Data</button>
            <div id="result"></div>
        </body>
        </html>
    )";
    
    std::string filepath = temp_dir->createFileWithContent("network_test.html", html_content);
    std::string file_url = "file://" + filepath;
    
    ASSERT_TRUE(loadPageWithReadinessCheck_NEW(file_url))
        << "Failed to load network test page";
    
    // OLD PATTERN: Network request with fixed waiting
    // browser_->clickElement("#fetch-btn");
    // std::this_thread::sleep_for(std::chrono::milliseconds(3000)); // BLOCKING
    
    // NEW PATTERN: Event-driven network completion
    browser_->clickElement("#fetch-btn");
    
    // Wait for network idle instead of fixed delay
    ASSERT_TRUE(TestUtils::TestWaitUtilities::waitForNetworkIdle(500, 10000))
        << "Network requests not completed";
    
    // Verify result with condition-based waiting
    ASSERT_TRUE(WAIT_FOR(
        browser_->executeJavascriptSync("return document.getElementById('result').textContent;") == "response",
        5000
    )) << "Network response not received";
}

// Performance comparison test
TEST_F(EventDrivenTestExample, PerformanceComparison) {
    TEST_SCOPE("PerformanceComparison");
    
    std::string html_content = R"(
        <!DOCTYPE html>
        <html><body><div id="test">Ready</div></body></html>
    )";
    
    std::string filepath = temp_dir->createFileWithContent("perf_test.html", html_content);
    std::string file_url = "file://" + filepath;
    
    // Measure old blocking pattern
    auto old_time = TestUtils::TestWaitUtilities::measureOperationTime([&]() {
        browser_->loadUri(file_url);
        browser_->waitForNavigation(5000);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // BLOCKING
        
        // Multiple fixed delays
        for (int i = 0; i < 5; i++) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200)); // BLOCKING
        }
    });
    
    // Measure new event-driven pattern
    auto new_time = TestUtils::TestWaitUtilities::measureOperationTime([&]() {
        browser_->loadUri(file_url);
        browser_->waitForNavigation(5000);
        
        // Condition-based waiting
        WAIT_FOR(
            browser_->executeJavascriptSync("return document.getElementById('test').textContent;") == "Ready",
            5000
        );
    });
    
    // Log performance improvement
    TestUtils::TestWaitUtilities::logTestStep(
        "Performance comparison - Old: " + std::to_string(old_time.count()) + 
        "ms, New: " + std::to_string(new_time.count()) + "ms"
    );
    
    // New pattern should be faster (no unnecessary delays)
    EXPECT_LT(new_time, old_time) << "Event-driven pattern should be faster";
}