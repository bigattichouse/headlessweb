#pragma once

#include <functional>
#include <chrono>
#include <string>
#include <future>

namespace TestUtils {

// Test-specific waiting utilities to replace blocking patterns
class TestWaitUtilities {
public:
    // Condition-based waiting instead of fixed delays
    static bool waitForCondition(
        std::function<bool()> condition, 
        int timeout_ms = 5000, 
        int check_interval_ms = 100);
    
    // Browser readiness waiting
    static bool waitForBrowserReady(int timeout_ms = 10000);
    
    // Assertion-based waiting with automatic retries
    static bool waitForAssertion(
        std::function<bool()> assertion, 
        int timeout_ms = 5000,
        const std::string& error_message = "Assertion failed");
    
    // Element state waiting
    static bool waitForElementExists(
        const std::string& selector, 
        int timeout_ms = 5000);
    
    static bool waitForElementVisible(
        const std::string& selector, 
        int timeout_ms = 5000);
    
    static bool waitForElementClickable(
        const std::string& selector, 
        int timeout_ms = 5000);
    
    // DOM state waiting
    static bool waitForDOMReady(int timeout_ms = 10000);
    static bool waitForPageLoadComplete(int timeout_ms = 15000);
    
    // Network state waiting
    static bool waitForNetworkIdle(int idle_time_ms = 500, int timeout_ms = 10000);
    
    // JavaScript execution waiting
    static bool waitForJavaScriptReady(int timeout_ms = 5000);
    static bool waitForJavaScriptCondition(
        const std::string& js_condition, 
        int timeout_ms = 5000);
    
    // Form interaction waiting
    static bool waitForFormReady(const std::string& form_selector, int timeout_ms = 5000);
    static bool waitForInputFilled(const std::string& input_selector, int timeout_ms = 3000);
    static bool waitForFormSubmitted(const std::string& form_selector, int timeout_ms = 10000);
    
    // Download waiting
    static bool waitForDownloadComplete(const std::string& filename_pattern, int timeout_ms = 30000);
    
    // Generic utility methods
    static void processEvents(int duration_ms = 100);
    static void synchronizeWithBrowser();
    static void yieldToSystem(int min_yield_ms = 10);
    
    // Performance measurement
    static std::chrono::milliseconds measureOperationTime(std::function<void()> operation);
    
    // Test environment helpers
    static bool isHeadlessEnvironment();
    static bool isDebugMode();
    static void logTestStep(const std::string& step_description);
    
private:
    static void processPlatformEvents();
    static bool checkBrowserState();
};

// RAII class for test timing and cleanup
class TestScope {
private:
    std::string test_name_;
    std::chrono::steady_clock::time_point start_time_;
    bool cleanup_required_;
    
public:
    explicit TestScope(const std::string& test_name, bool auto_cleanup = true);
    ~TestScope();
    
    void markCheckpoint(const std::string& checkpoint_name);
    void requireCleanup() { cleanup_required_ = true; }
    void skipCleanup() { cleanup_required_ = false; }
    
    std::chrono::milliseconds getElapsedTime() const;
};

// Macros for common test patterns
#define WAIT_FOR(condition, timeout_ms) \
    TestUtils::TestWaitUtilities::waitForCondition([&]() { return (condition); }, timeout_ms)

#define WAIT_FOR_ELEMENT(selector, timeout_ms) \
    TestUtils::TestWaitUtilities::waitForElementExists(selector, timeout_ms)

#define WAIT_FOR_ASSERTION(assertion, timeout_ms, message) \
    TestUtils::TestWaitUtilities::waitForAssertion([&]() { return (assertion); }, timeout_ms, message)

#define TEST_SCOPE(name) \
    TestUtils::TestScope test_scope(name)

#define YIELD_TO_BROWSER() \
    TestUtils::TestWaitUtilities::synchronizeWithBrowser()

} // namespace TestUtils