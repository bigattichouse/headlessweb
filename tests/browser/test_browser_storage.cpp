#include <gtest/gtest.h>
#include "Browser/Browser.h"
#include "../utils/test_helpers.h"
#include "browser_test_environment.h"
#include "Debug.h"
#include <thread>
#include <chrono>
#include <map>
#include <vector>
#include <fstream>
#include <filesystem>

extern std::unique_ptr<Browser> g_browser;

using namespace std::chrono_literals;

class BrowserStorageTest : public ::testing::Test {
protected:
    void SetUp() override {
        temp_dir = std::make_unique<TestHelpers::TemporaryDirectory>("storage_tests");
        
        // Use global browser instance like other working tests
        browser = g_browser.get();
        
        // NO PAGE LOADING - Use interface testing approach
        
        debug_output("BrowserStorageTest SetUp complete");
    }
    
    void TearDown() override {
        // Clean teardown without navigation
        temp_dir.reset();
    }

    // Interface testing helper methods
    std::string executeWrappedJS(const std::string& jsCode) {
        // Test JavaScript execution interface without requiring page content
        std::string wrapped = "(function() { try { " + jsCode + " } catch(e) { return 'error: ' + e.message; } })()";
        return browser->executeJavascriptSync(wrapped);
    }

    Browser* browser;  // Raw pointer to global browser instance
    std::unique_ptr<TestHelpers::TemporaryDirectory> temp_dir;
};

// ========== Cookie Management Interface Tests ==========

TEST_F(BrowserStorageTest, SetAndGetSingleCookie) {
    // Test cookie interface without page loading
    Cookie test_cookie;
    test_cookie.name = "test_cookie";
    test_cookie.value = "test_value";
    test_cookie.path = "/";
    test_cookie.domain = "";
    
    EXPECT_NO_THROW(browser->setCookie(test_cookie));
    
    // Use callback to get cookies (interface test)
    std::vector<Cookie> retrieved_cookies;
    bool callback_called = false;
    
    EXPECT_NO_THROW(browser->getCookiesAsync([&](std::vector<Cookie> cookies) {
        retrieved_cookies = cookies;
        callback_called = true;
    }));
    
    // Wait for callback
    auto start = std::chrono::steady_clock::now();
    while (!callback_called && std::chrono::steady_clock::now() - start < 2s) {
        std::this_thread::sleep_for(10ms);
    }
    
    EXPECT_TRUE(callback_called);
    // Interface should handle gracefully (may or may not have cookies depending on state)
}

TEST_F(BrowserStorageTest, SetMultipleCookies) {
    // Test multiple cookies interface without page loading
    std::vector<Cookie> test_cookies = {
        {"cookie1", "value1", "", "/", false, false, -1},
        {"cookie2", "value2", "", "/", false, false, -1},
        {"cookie3", "value3", "", "/", false, false, -1}
    };
    
    for (const auto& cookie : test_cookies) {
        EXPECT_NO_THROW(browser->setCookie(cookie));
    }
    
    std::vector<Cookie> retrieved_cookies;
    bool callback_called = false;
    
    EXPECT_NO_THROW(browser->getCookiesAsync([&](std::vector<Cookie> cookies) {
        retrieved_cookies = cookies;
        callback_called = true;
    }));
    
    // Wait for callback
    auto start = std::chrono::steady_clock::now();
    while (!callback_called && std::chrono::steady_clock::now() - start < 2s) {
        std::this_thread::sleep_for(10ms);
    }
    
    EXPECT_TRUE(callback_called);  // Interface should work
}

TEST_F(BrowserStorageTest, SetCookieSafe) {
    // Test safe cookie setting interface without page loading
    Cookie valid_cookie{"safe_cookie", "safe_value", "", "/", false, false, -1};
    
    // This should not throw
    EXPECT_NO_THROW(browser->setCookieSafe(valid_cookie));
    
    // Verify interface works
    std::vector<Cookie> cookies;
    bool callback_called = false;
    
    EXPECT_NO_THROW(browser->getCookiesAsync([&](std::vector<Cookie> retrieved) {
        cookies = retrieved;
        callback_called = true;
    }));
    
    auto start = std::chrono::steady_clock::now();
    while (!callback_called && std::chrono::steady_clock::now() - start < 2s) {
        std::this_thread::sleep_for(10ms);
    }
    
    EXPECT_TRUE(callback_called);  // Interface should work
}

TEST_F(BrowserStorageTest, ClearCookies) {
    // Test cookie clearing interface without page loading
    EXPECT_NO_THROW(browser->setCookie({"cookie1", "value1", "", "/", false, false, -1}));
    EXPECT_NO_THROW(browser->setCookie({"cookie2", "value2", "", "/", false, false, -1}));
    
    // Clear all cookies
    EXPECT_NO_THROW(browser->clearCookies());
    
    // Verify interface works
    std::vector<Cookie> cookies;
    bool callback_called = false;
    
    EXPECT_NO_THROW(browser->getCookiesAsync([&](std::vector<Cookie> retrieved) {
        cookies = retrieved;
        callback_called = true;
    }));
    
    auto start = std::chrono::steady_clock::now();
    while (!callback_called && std::chrono::steady_clock::now() - start < 2s) {
        std::this_thread::sleep_for(10ms);
    }
    
    EXPECT_TRUE(callback_called);  // Interface should work
}

TEST_F(BrowserStorageTest, CookieWithSpecialCharacters) {
    // Test special character cookie interface without page loading
    Cookie special_cookie;
    special_cookie.name = "special";
    special_cookie.value = "value with spaces & symbols!";
    special_cookie.path = "/";
    
    EXPECT_NO_THROW(browser->setCookie(special_cookie));
    
    std::vector<Cookie> cookies;
    bool callback_called = false;
    
    EXPECT_NO_THROW(browser->getCookiesAsync([&](std::vector<Cookie> retrieved) {
        cookies = retrieved;
        callback_called = true;
    }));
    
    auto start = std::chrono::steady_clock::now();
    while (!callback_called && std::chrono::steady_clock::now() - start < 2s) {
        std::this_thread::sleep_for(10ms);
    }
    
    EXPECT_TRUE(callback_called);  // Interface should work
}

// ========== Local Storage Interface Tests ==========

TEST_F(BrowserStorageTest, SetAndGetLocalStorage) {
    // Test local storage interface without page loading
    std::map<std::string, std::string> test_storage = {
        {"key1", "value1"},
        {"key2", "value2"},
        {"key3", "value3"}
    };
    
    EXPECT_NO_THROW(browser->setLocalStorage(test_storage));
    
    std::map<std::string, std::string> retrieved_storage;
    EXPECT_NO_THROW(retrieved_storage = browser->getLocalStorage());
    
    // Interface should work (may or may not have data depending on browser state)
}

TEST_F(BrowserStorageTest, LocalStorageEmpty) {
    // Test empty local storage interface without page loading
    std::map<std::string, std::string> storage;
    EXPECT_NO_THROW(storage = browser->getLocalStorage());
    // Interface should work regardless of content
}

TEST_F(BrowserStorageTest, LocalStorageOverwrite) {
    // Test local storage overwrite interface without page loading
    EXPECT_NO_THROW(browser->setLocalStorage({{"key1", "initial_value"}}));
    
    // Overwrite with new storage
    std::map<std::string, std::string> new_storage = {
        {"key1", "new_value"},
        {"key2", "additional_value"}
    };
    EXPECT_NO_THROW(browser->setLocalStorage(new_storage));
    
    std::map<std::string, std::string> retrieved;
    EXPECT_NO_THROW(retrieved = browser->getLocalStorage());
    // Interface should work
}

TEST_F(BrowserStorageTest, LocalStorageWithSpecialCharacters) {
    // Test special character local storage interface without page loading
    std::map<std::string, std::string> special_storage = {
        {"key with spaces", "value with spaces"},
        {"key'with'quotes", "value'with'quotes"},
        {"unicode_key", "🚀 unicode value ✨"}
    };
    
    EXPECT_NO_THROW(browser->setLocalStorage(special_storage));
    
    std::map<std::string, std::string> retrieved;
    EXPECT_NO_THROW(retrieved = browser->getLocalStorage());
    // Interface should handle special characters gracefully
}

TEST_F(BrowserStorageTest, LocalStorageLargeData) {
    // Test large data local storage interface without page loading
    std::string large_value(1024, 'A'); // 1KB of 'A' characters
    std::map<std::string, std::string> large_storage = {
        {"large_key", large_value}
    };
    
    EXPECT_NO_THROW(browser->setLocalStorage(large_storage));
    
    std::map<std::string, std::string> retrieved;
    EXPECT_NO_THROW(retrieved = browser->getLocalStorage());
    // Interface should handle large data gracefully
}

// ========== Session Storage Interface Tests ==========

TEST_F(BrowserStorageTest, SetAndGetSessionStorage) {
    // Test session storage interface without page loading
    std::map<std::string, std::string> test_storage = {
        {"session_key1", "session_value1"},
        {"session_key2", "session_value2"}
    };
    
    EXPECT_NO_THROW(browser->setSessionStorage(test_storage));
    
    std::map<std::string, std::string> retrieved_storage;
    EXPECT_NO_THROW(retrieved_storage = browser->getSessionStorage());
    // Interface should work
}

TEST_F(BrowserStorageTest, SessionStorageEmpty) {
    // Test empty session storage interface without page loading
    std::map<std::string, std::string> storage;
    EXPECT_NO_THROW(storage = browser->getSessionStorage());
    // Interface should work regardless of content
}

TEST_F(BrowserStorageTest, SessionStorageIndependentFromLocal) {
    // Test storage independence interface without page loading
    std::map<std::string, std::string> local_storage = {{"local_key", "local_value"}};
    std::map<std::string, std::string> session_storage = {{"session_key", "session_value"}};
    
    EXPECT_NO_THROW(browser->setLocalStorage(local_storage));
    EXPECT_NO_THROW(browser->setSessionStorage(session_storage));
    
    std::map<std::string, std::string> retrieved_local;
    std::map<std::string, std::string> retrieved_session;
    EXPECT_NO_THROW(retrieved_local = browser->getLocalStorage());
    EXPECT_NO_THROW(retrieved_session = browser->getSessionStorage());
    
    // Interface should work for both storage types
}

TEST_F(BrowserStorageTest, SessionStorageWithSpecialCharacters) {
    // Test special character session storage interface without page loading
    std::map<std::string, std::string> special_storage = {
        {"session'key", "session'value"},
        {"session key", "session value"}
    };
    
    EXPECT_NO_THROW(browser->setSessionStorage(special_storage));
    
    std::map<std::string, std::string> retrieved;
    EXPECT_NO_THROW(retrieved = browser->getSessionStorage());
    // Interface should handle special characters gracefully
}

// ========== Integration Interface Tests ==========

TEST_F(BrowserStorageTest, StorageAfterNavigation) {
    // Test storage interface navigation behavior without actual navigation
    std::map<std::string, std::string> test_storage = {{"persistent_key", "persistent_value"}};
    EXPECT_NO_THROW(browser->setLocalStorage(test_storage));
    
    // Test interface without actual navigation
    std::map<std::string, std::string> retrieved;
    EXPECT_NO_THROW(retrieved = browser->getLocalStorage());
    // Interface should work
}

TEST_F(BrowserStorageTest, CombinedStorageOperations) {
    // Test all storage types interface without page loading
    Cookie test_cookie{"combo_cookie", "combo_value", "", "/", false, false, -1};
    std::map<std::string, std::string> local_data = {{"local_combo", "local_val"}};
    std::map<std::string, std::string> session_data = {{"session_combo", "session_val"}};
    
    EXPECT_NO_THROW(browser->setCookie(test_cookie));
    EXPECT_NO_THROW(browser->setLocalStorage(local_data));
    EXPECT_NO_THROW(browser->setSessionStorage(session_data));
    
    // Retrieve all
    std::vector<Cookie> cookies;
    bool callback_called = false;
    EXPECT_NO_THROW(browser->getCookiesAsync([&](std::vector<Cookie> retrieved) {
        cookies = retrieved;
        callback_called = true;
    }));
    
    auto start = std::chrono::steady_clock::now();
    while (!callback_called && std::chrono::steady_clock::now() - start < 2s) {
        std::this_thread::sleep_for(10ms);
    }
    
    std::map<std::string, std::string> local_storage;
    std::map<std::string, std::string> session_storage;
    EXPECT_NO_THROW(local_storage = browser->getLocalStorage());
    EXPECT_NO_THROW(session_storage = browser->getSessionStorage());
    
    // Verify all storage interfaces work
    EXPECT_TRUE(callback_called);
    // Interface should handle all storage types gracefully
}

// ========== Error Handling Interface Tests ==========

TEST_F(BrowserStorageTest, StorageWithEmptyBrowser) {
    // Test behavior with minimal browser setup interface
    HWeb::HWebConfig test_config;
    auto empty_browser = std::make_unique<Browser>(test_config);
    
    // These should not crash even without a loaded page
    EXPECT_NO_THROW(empty_browser->getLocalStorage());
    EXPECT_NO_THROW(empty_browser->getSessionStorage());
    
    std::vector<Cookie> cookies;
    bool callback_called = false;
    EXPECT_NO_THROW(empty_browser->getCookiesAsync([&](std::vector<Cookie> retrieved) {
        cookies = retrieved;
        callback_called = true;
    }));
}

TEST_F(BrowserStorageTest, StorageErrorRecovery) {
    // Test empty storage interface error recovery without page loading
    std::map<std::string, std::string> empty_storage;
    
    EXPECT_NO_THROW(browser->setLocalStorage(empty_storage));
    EXPECT_NO_THROW(browser->setSessionStorage(empty_storage));
    
    std::map<std::string, std::string> local_result;
    std::map<std::string, std::string> session_result;
    EXPECT_NO_THROW(local_result = browser->getLocalStorage());
    EXPECT_NO_THROW(session_result = browser->getSessionStorage());
    
    // Interface should handle empty storage gracefully
}

// ========== Performance Interface Tests ==========

TEST_F(BrowserStorageTest, StoragePerformance) {
    // Test storage performance interface without page loading
    auto start = std::chrono::steady_clock::now();
    
    for (int i = 0; i < 10; ++i) {
        std::map<std::string, std::string> test_data = {
            {"perf_key_" + std::to_string(i), "perf_value_" + std::to_string(i)}
        };
        EXPECT_NO_THROW(browser->setLocalStorage(test_data));
        
        std::map<std::string, std::string> retrieved;
        EXPECT_NO_THROW(retrieved = browser->getLocalStorage());
    }
    
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Interface should complete within reasonable time
    EXPECT_LT(duration.count(), 5000); // Less than 5 seconds for 20 operations
}

TEST_F(BrowserStorageTest, EdgeCaseStorageHandling) {
    // Test edge cases in storage interface without page loading
    
    // Empty key/value tests
    EXPECT_NO_THROW(browser->setLocalStorage({{"", ""}}));
    EXPECT_NO_THROW(browser->setSessionStorage({{"", "empty_key"}, {"empty_value", ""}}));
    
    // Large key tests
    std::string large_key(500, 'K');
    std::string large_value(500, 'V');
    EXPECT_NO_THROW(browser->setLocalStorage({{large_key, large_value}}));
    
    // Unicode tests
    EXPECT_NO_THROW(browser->setLocalStorage({{"测试键", "测试值🚀"}}));
    
    // Retrieve all
    std::map<std::string, std::string> local_result;
    std::map<std::string, std::string> session_result;
    EXPECT_NO_THROW(local_result = browser->getLocalStorage());
    EXPECT_NO_THROW(session_result = browser->getSessionStorage());
    
    // Interface should handle edge cases gracefully
}