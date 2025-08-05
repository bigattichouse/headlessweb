#include "TestWaitUtilities.h"
#include "../../src/Debug.h"
#include <thread>
#include <iostream>
#include <gtk/gtk.h>

// External debug flag
extern bool g_debug;

namespace TestUtils {

// ========== TestWaitUtilities Implementation ==========

bool TestWaitUtilities::waitForCondition(std::function<bool()> condition, int timeout_ms, int check_interval_ms) {
    if (!condition) {
        return false;
    }
    
    auto start_time = std::chrono::steady_clock::now();
    auto timeout = std::chrono::milliseconds(timeout_ms);
    
    while (std::chrono::steady_clock::now() - start_time < timeout) {
        try {
            if (condition()) {
                return true;
            }
        } catch (const std::exception& e) {
            debug_output("Condition check failed: " + std::string(e.what()));
        }
        
        // Process events instead of blocking sleep
        processEvents(check_interval_ms);
    }
    
    return false;
}

bool TestWaitUtilities::waitForBrowserReady(int timeout_ms) {
    return waitForCondition([]() {
        return checkBrowserState();
    }, timeout_ms);
}

bool TestWaitUtilities::waitForAssertion(std::function<bool()> assertion, int timeout_ms, const std::string& error_message) {
    bool result = waitForCondition(assertion, timeout_ms);
    
    if (!result && !error_message.empty()) {
        debug_output("Assertion timeout: " + error_message);
    }
    
    return result;
}

bool TestWaitUtilities::waitForElementExists(const std::string& selector, int timeout_ms) {
    // This would integrate with the Browser class in real implementation
    return waitForCondition([selector]() {
        // Placeholder - would check if element exists via Browser API
        return true;
    }, timeout_ms);
}

bool TestWaitUtilities::waitForElementVisible(const std::string& selector, int timeout_ms) {
    return waitForCondition([selector]() {
        // Placeholder - would check element visibility via Browser API
        return true;
    }, timeout_ms);
}

bool TestWaitUtilities::waitForElementClickable(const std::string& selector, int timeout_ms) {
    return waitForCondition([selector]() {
        // Placeholder - would check if element is clickable via Browser API
        return true;
    }, timeout_ms);
}

bool TestWaitUtilities::waitForDOMReady(int timeout_ms) {
    return waitForCondition([]() {
        // Placeholder - would check DOM ready state via Browser API
        return true;
    }, timeout_ms);
}

bool TestWaitUtilities::waitForPageLoadComplete(int timeout_ms) {
    return waitForCondition([]() {
        // Placeholder - would check page load completion via Browser API
        return true;
    }, timeout_ms);
}

bool TestWaitUtilities::waitForNetworkIdle(int idle_time_ms, int timeout_ms) {
    return waitForCondition([idle_time_ms]() {
        // Placeholder - would check network idle state via Browser API
        return true;
    }, timeout_ms);
}

bool TestWaitUtilities::waitForJavaScriptReady(int timeout_ms) {
    return waitForCondition([]() {
        // Placeholder - would check JavaScript ready state via Browser API
        return true;
    }, timeout_ms);
}

bool TestWaitUtilities::waitForJavaScriptCondition(const std::string& js_condition, int timeout_ms) {
    return waitForCondition([js_condition]() {
        // Placeholder - would execute JavaScript condition via Browser API
        return true;
    }, timeout_ms);
}

bool TestWaitUtilities::waitForFormReady(const std::string& form_selector, int timeout_ms) {
    return waitForCondition([form_selector]() {
        // Placeholder - would check form ready state via Browser API
        return true;
    }, timeout_ms);
}

bool TestWaitUtilities::waitForInputFilled(const std::string& input_selector, int timeout_ms) {
    return waitForCondition([input_selector]() {
        // Placeholder - would check input fill state via Browser API
        return true;
    }, timeout_ms);
}

bool TestWaitUtilities::waitForFormSubmitted(const std::string& form_selector, int timeout_ms) {
    return waitForCondition([form_selector]() {
        // Placeholder - would check form submission state via Browser API
        return true;
    }, timeout_ms);
}

bool TestWaitUtilities::waitForDownloadComplete(const std::string& filename_pattern, int timeout_ms) {
    return waitForCondition([filename_pattern]() {
        // Placeholder - would check download completion via FileOps API
        return true;
    }, timeout_ms);
}

void TestWaitUtilities::processEvents(int duration_ms) {
    auto start_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::milliseconds(duration_ms);
    
    while (std::chrono::steady_clock::now() - start_time < duration) {
        processPlatformEvents();
        
        // Yield to other threads with minimal sleep
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void TestWaitUtilities::synchronizeWithBrowser() {
    // Process all pending GTK/platform events
    processPlatformEvents();
    
    // Small yield to ensure browser has time to process
    yieldToSystem(50);
}

void TestWaitUtilities::yieldToSystem(int min_yield_ms) {
    processPlatformEvents();
    std::this_thread::sleep_for(std::chrono::milliseconds(min_yield_ms));
}

std::chrono::milliseconds TestWaitUtilities::measureOperationTime(std::function<void()> operation) {
    auto start_time = std::chrono::steady_clock::now();
    
    try {
        operation();
    } catch (const std::exception& e) {
        debug_output("Operation measurement failed: " + std::string(e.what()));
    }
    
    auto end_time = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
}

bool TestWaitUtilities::isHeadlessEnvironment() {
    // Check if running in headless mode
    return std::getenv("DISPLAY") == nullptr || std::getenv("HEADLESS") != nullptr;
}

bool TestWaitUtilities::isDebugMode() {
    return g_debug;
}

void TestWaitUtilities::logTestStep(const std::string& step_description) {
    if (isDebugMode()) {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        debug_output("[TEST STEP] " + step_description);
    }
}

void TestWaitUtilities::processPlatformEvents() {
    // Process GTK events if available
    if (gtk_init_check()) {
        GMainContext* context = g_main_context_default();
        while (g_main_context_pending(context)) {
            g_main_context_iteration(context, FALSE);
        }
    }
}

bool TestWaitUtilities::checkBrowserState() {
    // Placeholder - would integrate with Browser readiness checking
    // In real implementation, this would check:
    // - DOM ready state
    // - JavaScript execution state
    // - Network idle state
    // - Resource loading completion
    return true;
}

// ========== TestScope Implementation ==========

TestScope::TestScope(const std::string& test_name, bool auto_cleanup) 
    : test_name_(test_name), cleanup_required_(auto_cleanup) {
    
    start_time_ = std::chrono::steady_clock::now();
    TestWaitUtilities::logTestStep("Starting test: " + test_name_);
}

TestScope::~TestScope() {
    auto elapsed = getElapsedTime();
    TestWaitUtilities::logTestStep("Completed test: " + test_name_ + 
                                  " (elapsed: " + std::to_string(elapsed.count()) + "ms)");
    
    if (cleanup_required_) {
        // Perform cleanup - synchronize with browser state
        TestWaitUtilities::synchronizeWithBrowser();
    }
}

void TestScope::markCheckpoint(const std::string& checkpoint_name) {
    auto elapsed = getElapsedTime();
    TestWaitUtilities::logTestStep("Checkpoint '" + checkpoint_name + "' at " + 
                                  std::to_string(elapsed.count()) + "ms");
}

std::chrono::milliseconds TestScope::getElapsedTime() const {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time_);
}

} // namespace TestUtils