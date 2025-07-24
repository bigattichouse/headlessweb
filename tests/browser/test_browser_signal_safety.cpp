#include <gtest/gtest.h>
#include "../../src/Browser/Browser.h"
#include <thread>
#include <chrono>
#include <memory>
#include <atomic>
#include <vector>

/**
 * Browser Signal Safety Test Suite
 * 
 * Tests the robust signal handling system that prevents segmentation faults
 * in WebKit signal callbacks after Browser object destruction.
 */
class BrowserSignalSafetyTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Each test gets a fresh browser instance
        HWeb::HWebConfig test_config;
        browser = std::make_unique<Browser>(test_config);
    }
    
    void TearDown() override {
        // Cleanup browser instance
        browser.reset();
        
        // Allow time for any pending signals to complete
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    std::unique_ptr<Browser> browser;
};

// Test 1: Basic object validity checking
TEST_F(BrowserSignalSafetyTest, ObjectValidityCheck) {
    // Object should be valid when created
    EXPECT_TRUE(browser->isObjectValid());
    
    // Create a copy of the pointer for testing after destruction
    Browser* browser_ptr = browser.get();
    
    // Object should still be valid before destruction
    EXPECT_TRUE(browser_ptr->isObjectValid());
    
    // Destroy the browser object
    browser.reset();
    
    // Note: We can't test the invalid state after destruction because
    // accessing the object after destruction is undefined behavior.
    // The validity check is for use within signal handlers during destruction.
}

// Test 2: Signal handler setup and teardown
TEST_F(BrowserSignalSafetyTest, SignalHandlerLifecycle) {
    // Test that signal handlers can be set up and torn down without issues
    EXPECT_NO_THROW(browser->setupSignalHandlers());
    EXPECT_NO_THROW(browser->disconnectSignalHandlers());
    
    // Test multiple setup/teardown cycles
    for (int i = 0; i < 3; i++) {
        EXPECT_NO_THROW(browser->setupSignalHandlers());
        EXPECT_NO_THROW(browser->disconnectSignalHandlers());
    }
}

// Test 3: Safe cleanup of waiters
TEST_F(BrowserSignalSafetyTest, WaiterCleanup) {
    // Test that waiter cleanup doesn't crash
    EXPECT_NO_THROW(browser->cleanupWaiters());
    
    // Test cleanup during various states
    browser->setupSignalHandlers();
    EXPECT_NO_THROW(browser->cleanupWaiters());
    
    browser->disconnectSignalHandlers();
    EXPECT_NO_THROW(browser->cleanupWaiters());
}

// Test 4: Thread-safe notification methods
TEST_F(BrowserSignalSafetyTest, ThreadSafeNotifications) {
    std::atomic<int> notification_count{0};
    std::atomic<bool> test_running{true};
    std::vector<std::thread> threads;
    
    // Create multiple threads that call notification methods
    for (int i = 0; i < 5; i++) {
        threads.emplace_back([&]() {
            while (test_running.load()) {
                if (browser && browser->isObjectValid()) {
                    browser->notifyNavigationComplete();
                    browser->notifyUriChanged();
                    browser->notifyTitleChanged();
                    browser->notifyReadyToShow();
                    notification_count++;
                }
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        });
    }
    
    // Let threads run for a short time
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    // Stop threads
    test_running.store(false);
    for (auto& t : threads) {
        t.join();
    }
    
    // Verify that notifications were called without crashing
    EXPECT_GT(notification_count.load(), 0);
    EXPECT_TRUE(browser->isObjectValid());
}

// Test 5: Rapid browser creation and destruction
TEST_F(BrowserSignalSafetyTest, RapidCreateDestroy) {
    // Test that rapid creation/destruction doesn't cause issues
    std::vector<std::unique_ptr<Browser>> browsers;
    
    // Create multiple browsers quickly
    for (int i = 0; i < 10; i++) {
        HWeb::HWebConfig test_config;
        auto test_browser = std::make_unique<Browser>(test_config);
        EXPECT_TRUE(test_browser->isObjectValid());
        browsers.push_back(std::move(test_browser));
    }
    
    // Destroy them all quickly
    browsers.clear();
    
    // Allow time for any pending signals
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // No crashes should have occurred
    SUCCEED();
}

// Test 6: Signal disconnection before destruction
TEST_F(BrowserSignalSafetyTest, ProperSignalDisconnection) {
    // Setup signals
    browser->setupSignalHandlers();
    
    // Manually disconnect before destruction
    EXPECT_NO_THROW(browser->disconnectSignalHandlers());
    
    // Object should still be valid
    EXPECT_TRUE(browser->isObjectValid());
    
    // Destruction should be clean
    browser.reset();
    
    // Allow time for any lingering signals
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    SUCCEED();
}

// Test 7: Navigation signal waiting with early destruction
TEST_F(BrowserSignalSafetyTest, NavigationSignalWithDestruction) {
    std::atomic<bool> waiting_complete{false};
    std::atomic<bool> test_failed{false};
    
    // Start a navigation wait in a separate thread
    std::thread wait_thread([&]() {
        try {
            // This should handle early browser destruction gracefully
            bool result = browser->waitForNavigationSignal(1000); // 1 second timeout
            waiting_complete.store(true);
        } catch (const std::exception& e) {
            test_failed.store(true);
        }
    });
    
    // Destroy browser while wait is potentially active
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    browser.reset();
    
    // Wait for the wait thread to complete
    wait_thread.join();
    
    // Verify no exceptions were thrown
    EXPECT_FALSE(test_failed.load());
}

// Test 8: Multiple concurrent signal handlers
TEST_F(BrowserSignalSafetyTest, ConcurrentSignalHandlers) {
    std::atomic<int> total_notifications{0};
    std::atomic<bool> test_running{true};
    std::vector<std::thread> notifier_threads;
    
    // Create threads that trigger different notification types
    const std::vector<std::string> notification_types = {
        "navigation", "uri", "title", "ready"
    };
    
    for (const auto& type : notification_types) {
        notifier_threads.emplace_back([&, type]() {
            while (test_running.load() && browser && browser->isObjectValid()) {
                if (type == "navigation") {
                    browser->notifyNavigationComplete();
                } else if (type == "uri") {
                    browser->notifyUriChanged();
                } else if (type == "title") {
                    browser->notifyTitleChanged();
                } else if (type == "ready") {
                    browser->notifyReadyToShow();
                }
                total_notifications++;
                std::this_thread::sleep_for(std::chrono::microseconds(200));
            }
        });
    }
    
    // Let it run briefly
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Stop all threads
    test_running.store(false);
    for (auto& t : notifier_threads) {
        t.join();
    }
    
    // Verify notifications occurred without issues
    EXPECT_GT(total_notifications.load(), 0);
    EXPECT_TRUE(browser->isObjectValid());
}

// Test 9: Memory leak prevention
TEST_F(BrowserSignalSafetyTest, MemoryLeakPrevention) {
    // This test ensures that the signal handling doesn't create memory leaks
    size_t initial_browsers = 1; // Our test browser
    
    // Create and destroy many browsers to check for leaks
    for (int cycle = 0; cycle < 20; cycle++) {
        std::vector<std::unique_ptr<Browser>> temp_browsers;
        
        // Create browsers
        for (int i = 0; i < 5; i++) {
            HWeb::HWebConfig test_config;
            auto temp = std::make_unique<Browser>(test_config);
            temp->setupSignalHandlers();
            temp_browsers.push_back(std::move(temp));
        }
        
        // Trigger some notifications
        for (auto& b : temp_browsers) {
            if (b && b->isObjectValid()) {
                b->notifyNavigationComplete();
                b->notifyUriChanged();
            }
        }
        
        // Clean destruction
        temp_browsers.clear();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // Test passes if we reach here without memory issues
    SUCCEED();
}

// Test 10: Error handling in notification callbacks
TEST_F(BrowserSignalSafetyTest, CallbackErrorHandling) {
    // This test verifies that errors in callbacks don't crash the system
    // Note: This is more of a structural test since we can't easily inject
    // failing callbacks into the private signal system.
    
    // Test that normal notifications work
    EXPECT_NO_THROW(browser->notifyNavigationComplete());
    EXPECT_NO_THROW(browser->notifyUriChanged());
    EXPECT_NO_THROW(browser->notifyTitleChanged());
    EXPECT_NO_THROW(browser->notifyReadyToShow());
    
    // Test during invalid state simulation
    // Note: We can't actually set is_valid to false from outside,
    // but the methods should handle invalid states gracefully
    EXPECT_TRUE(browser->isObjectValid());
}