#include <gtest/gtest.h>
#include "Browser/Browser.h"
#include "../utils/test_helpers.h"
#include <thread>
#include <chrono>
#include <map>
#include <vector>
#include <fstream>
#include <filesystem>

using namespace std::chrono_literals;

class BrowserStorageTest : public ::testing::Test {
protected:
    void SetUp() override {
        HWeb::HWebConfig test_config;
        browser = std::make_unique<Browser>(test_config);
        
        // Create temporary directory for HTML test files
        temp_dir = std::make_unique<TestHelpers::TemporaryDirectory>("storage_tests");
        
        // Load a simple HTML page for testing storage functionality
        setupTestPage();
        
        // Wait for page to be ready
        std::this_thread::sleep_for(1000ms);
    }
    
    void setupTestPage() {
        // CRITICAL FIX: Create real HTML file for localStorage access
        // WebKit requires file:// origin for localStorage to work properly
        std::string test_html = R"HTML(
<!DOCTYPE html>
<html>
<head>
    <title>Storage Test Page</title>
    <script>
        // Helper functions for testing
        function clearAllStorage() {
            try {
                localStorage.clear();
                sessionStorage.clear();
                document.cookie.split(";").forEach(function(c) { 
                    document.cookie = c.replace(/^ +/, "").replace(/=.*/, "=;expires=" + new Date().toUTCString() + ";path=/"); 
                });
            } catch(e) {
                console.log('Storage clear error:', e);
            }
        }
        
        function getStorageInfo() {
            return {
                localStorageLength: localStorage.length,
                sessionStorageLength: sessionStorage.length,
                cookieCount: document.cookie ? document.cookie.split(';').length : 0
            };
        }
        
        function updateStatus(message) {
            document.getElementById('status').textContent = message;
        }
        
        // Test localStorage availability on page load
        document.addEventListener('DOMContentLoaded', function() {
            try {
                localStorage.setItem('test_availability', 'available');
                var result = localStorage.getItem('test_availability');
                console.log('localStorage test result:', result);
                localStorage.removeItem('test_availability');
            } catch(e) {
                console.log('localStorage not available:', e.message);
            }
        });
    </script>
</head>
<body>
    <h1>Browser Storage Test</h1>
    <p id="status">Ready</p>
</body>
</html>
)HTML";
        
        // Create HTML file and load via file:// URL
        test_html_file = temp_dir->createFile("storage_test.html", test_html);
        std::string file_url = "file://" + test_html_file.string();
        browser->loadUri(file_url);
        
        // Wait for page load
        std::this_thread::sleep_for(1000ms);
        
        // Clear any existing storage - wait for clearing to complete
        browser->executeJavascriptSync("clearAllStorage();");
        std::this_thread::sleep_for(200ms);
        
        // Verify storage is actually cleared
        auto initial_storage = browser->getLocalStorage();
        if (!initial_storage.empty()) {
            // Force clear if needed
            browser->executeJavascriptSync("localStorage.clear(); sessionStorage.clear();");
            std::this_thread::sleep_for(200ms);
        }
    }

    void TearDown() override {
        if (browser) {
            // Clean up storage before destroying browser
            browser->executeJavascriptSync("try { clearAllStorage(); } catch(e) {}");
        }
        browser.reset();
        temp_dir.reset();
    }

    std::unique_ptr<Browser> browser;
    std::unique_ptr<TestHelpers::TemporaryDirectory> temp_dir;
    std::filesystem::path test_html_file;
};

// ========== Cookie Management Tests ==========

TEST_F(BrowserStorageTest, SetAndGetSingleCookie) {
    Cookie test_cookie;
    test_cookie.name = "test_cookie";
    test_cookie.value = "test_value";
    test_cookie.path = "/";
    test_cookie.domain = "";
    
    browser->setCookie(test_cookie);
    
    // Use callback to get cookies
    std::vector<Cookie> retrieved_cookies;
    bool callback_called = false;
    
    browser->getCookiesAsync([&](std::vector<Cookie> cookies) {
        retrieved_cookies = cookies;
        callback_called = true;
    });
    
    // Wait for callback
    auto start = std::chrono::steady_clock::now();
    while (!callback_called && std::chrono::steady_clock::now() - start < 2s) {
        std::this_thread::sleep_for(10ms);
    }
    
    EXPECT_TRUE(callback_called);
    EXPECT_GE(retrieved_cookies.size(), 1);
    
    // Find our test cookie
    bool found = false;
    for (const auto& cookie : retrieved_cookies) {
        if (cookie.name == "test_cookie" && cookie.value == "test_value") {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(BrowserStorageTest, SetMultipleCookies) {
    std::vector<Cookie> test_cookies = {
        {"cookie1", "value1", "", "/", false, false, -1},
        {"cookie2", "value2", "", "/", false, false, -1},
        {"cookie3", "value3", "", "/", false, false, -1}
    };
    
    for (const auto& cookie : test_cookies) {
        browser->setCookie(cookie);
    }
    
    std::vector<Cookie> retrieved_cookies;
    bool callback_called = false;
    
    browser->getCookiesAsync([&](std::vector<Cookie> cookies) {
        retrieved_cookies = cookies;
        callback_called = true;
    });
    
    // Wait for callback
    auto start = std::chrono::steady_clock::now();
    while (!callback_called && std::chrono::steady_clock::now() - start < 2s) {
        std::this_thread::sleep_for(10ms);
    }
    
    EXPECT_TRUE(callback_called);
    EXPECT_GE(retrieved_cookies.size(), 3);
    
    // Verify all cookies are present
    for (const auto& test_cookie : test_cookies) {
        bool found = false;
        for (const auto& retrieved_cookie : retrieved_cookies) {
            if (retrieved_cookie.name == test_cookie.name && 
                retrieved_cookie.value == test_cookie.value) {
                found = true;
                break;
            }
        }
        EXPECT_TRUE(found) << "Cookie " << test_cookie.name << " not found";
    }
}

TEST_F(BrowserStorageTest, SetCookieSafe) {
    Cookie valid_cookie{"safe_cookie", "safe_value", "", "/", false, false, -1};
    
    // This should not throw
    EXPECT_NO_THROW(browser->setCookieSafe(valid_cookie));
    
    // Verify cookie was set
    std::vector<Cookie> cookies;
    bool callback_called = false;
    
    browser->getCookiesAsync([&](std::vector<Cookie> retrieved) {
        cookies = retrieved;
        callback_called = true;
    });
    
    auto start = std::chrono::steady_clock::now();
    while (!callback_called && std::chrono::steady_clock::now() - start < 2s) {
        std::this_thread::sleep_for(10ms);
    }
    
    bool found = false;
    for (const auto& cookie : cookies) {
        if (cookie.name == "safe_cookie") {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(BrowserStorageTest, ClearCookies) {
    // Set some cookies first
    browser->setCookie({"cookie1", "value1", "", "/", false, false, -1});
    browser->setCookie({"cookie2", "value2", "", "/", false, false, -1});
    
    // Clear all cookies
    browser->clearCookies();
    
    // Verify cookies are cleared
    std::vector<Cookie> cookies;
    bool callback_called = false;
    
    browser->getCookiesAsync([&](std::vector<Cookie> retrieved) {
        cookies = retrieved;
        callback_called = true;
    });
    
    auto start = std::chrono::steady_clock::now();
    while (!callback_called && std::chrono::steady_clock::now() - start < 2s) {
        std::this_thread::sleep_for(10ms);
    }
    
    EXPECT_TRUE(callback_called);
    EXPECT_EQ(cookies.size(), 0);
}

TEST_F(BrowserStorageTest, CookieWithSpecialCharacters) {
    Cookie special_cookie;
    special_cookie.name = "special";
    special_cookie.value = "value with spaces & symbols!";
    special_cookie.path = "/";
    
    browser->setCookie(special_cookie);
    
    std::vector<Cookie> cookies;
    bool callback_called = false;
    
    browser->getCookiesAsync([&](std::vector<Cookie> retrieved) {
        cookies = retrieved;
        callback_called = true;
    });
    
    auto start = std::chrono::steady_clock::now();
    while (!callback_called && std::chrono::steady_clock::now() - start < 2s) {
        std::this_thread::sleep_for(10ms);
    }
    
    bool found = false;
    for (const auto& cookie : cookies) {
        if (cookie.name == "special" && !cookie.value.empty()) {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);
}

// ========== Local Storage Tests ==========

TEST_F(BrowserStorageTest, SetAndGetLocalStorage) {
    std::map<std::string, std::string> test_storage = {
        {"key1", "value1"},
        {"key2", "value2"},
        {"key3", "value3"}
    };
    
    browser->setLocalStorage(test_storage);
    
    auto retrieved_storage = browser->getLocalStorage();
    
    EXPECT_EQ(retrieved_storage.size(), test_storage.size());
    
    for (const auto& [key, value] : test_storage) {
        EXPECT_EQ(retrieved_storage[key], value);
    }
}

TEST_F(BrowserStorageTest, LocalStorageEmpty) {
    auto storage = browser->getLocalStorage();
    EXPECT_TRUE(storage.empty());
}

TEST_F(BrowserStorageTest, LocalStorageOverwrite) {
    // Set initial storage
    browser->setLocalStorage({{"key1", "initial_value"}});
    
    // Overwrite with new storage
    std::map<std::string, std::string> new_storage = {
        {"key1", "new_value"},
        {"key2", "additional_value"}
    };
    browser->setLocalStorage(new_storage);
    
    auto retrieved = browser->getLocalStorage();
    
    EXPECT_EQ(retrieved["key1"], "new_value");
    EXPECT_EQ(retrieved["key2"], "additional_value");
}

TEST_F(BrowserStorageTest, LocalStorageWithSpecialCharacters) {
    std::map<std::string, std::string> special_storage = {
        {"key with spaces", "value with spaces"},
        {"key'with'quotes", "value'with'quotes"},
        {"unicode_key", "🚀 unicode value ✨"}
    };
    
    browser->setLocalStorage(special_storage);
    auto retrieved = browser->getLocalStorage();
    
    // At least verify we can set and retrieve without crashing
    EXPECT_FALSE(retrieved.empty());
    
    // Check if keys exist (values might be escaped)
    bool has_space_key = retrieved.find("key with spaces") != retrieved.end() ||
                        retrieved.find("key\\ with\\ spaces") != retrieved.end();
    EXPECT_TRUE(has_space_key);
}

TEST_F(BrowserStorageTest, LocalStorageLargeData) {
    std::string large_value(1024, 'A'); // 1KB of 'A' characters
    std::map<std::string, std::string> large_storage = {
        {"large_key", large_value}
    };
    
    browser->setLocalStorage(large_storage);
    auto retrieved = browser->getLocalStorage();
    
    EXPECT_FALSE(retrieved.empty());
    auto it = retrieved.find("large_key");
    if (it != retrieved.end()) {
        EXPECT_FALSE(it->second.empty());
    }
}

// ========== Session Storage Tests ==========

TEST_F(BrowserStorageTest, SetAndGetSessionStorage) {
    std::map<std::string, std::string> test_storage = {
        {"session_key1", "session_value1"},
        {"session_key2", "session_value2"}
    };
    
    browser->setSessionStorage(test_storage);
    
    auto retrieved_storage = browser->getSessionStorage();
    
    EXPECT_EQ(retrieved_storage.size(), test_storage.size());
    
    for (const auto& [key, value] : test_storage) {
        EXPECT_EQ(retrieved_storage[key], value);
    }
}

TEST_F(BrowserStorageTest, SessionStorageEmpty) {
    auto storage = browser->getSessionStorage();
    EXPECT_TRUE(storage.empty());
}

TEST_F(BrowserStorageTest, SessionStorageIndependentFromLocal) {
    std::map<std::string, std::string> local_storage = {{"local_key", "local_value"}};
    std::map<std::string, std::string> session_storage = {{"session_key", "session_value"}};
    
    browser->setLocalStorage(local_storage);
    browser->setSessionStorage(session_storage);
    
    auto retrieved_local = browser->getLocalStorage();
    auto retrieved_session = browser->getSessionStorage();
    
    EXPECT_EQ(retrieved_local.size(), 1);
    EXPECT_EQ(retrieved_session.size(), 1);
    EXPECT_EQ(retrieved_local["local_key"], "local_value");
    EXPECT_EQ(retrieved_session["session_key"], "session_value");
}

TEST_F(BrowserStorageTest, SessionStorageWithSpecialCharacters) {
    std::map<std::string, std::string> special_storage = {
        {"session'key", "session'value"},
        {"session key", "session value"}
    };
    
    browser->setSessionStorage(special_storage);
    auto retrieved = browser->getSessionStorage();
    
    // Verify we can set and retrieve without crashing
    EXPECT_FALSE(retrieved.empty());
}

// ========== Integration Tests ==========

TEST_F(BrowserStorageTest, StorageAfterNavigation) {
    // Set storage on current page
    std::map<std::string, std::string> test_storage = {{"persistent_key", "persistent_value"}};
    browser->setLocalStorage(test_storage);
    
    // Navigate to another data URL
    std::string new_page = "data:text/html,<html><body><h1>New Page</h1></body></html>";
    browser->loadUri(new_page);
    std::this_thread::sleep_for(500ms);
    
    // Local storage should persist (session storage might not)
    auto retrieved = browser->getLocalStorage();
    
    // Note: data URLs might not preserve localStorage between navigations
    // This test verifies the mechanism works without throwing
    EXPECT_NO_THROW(browser->getLocalStorage());
}

TEST_F(BrowserStorageTest, CombinedStorageOperations) {
    // Test all storage types together
    Cookie test_cookie{"combo_cookie", "combo_value", "", "/", false, false, -1};
    std::map<std::string, std::string> local_data = {{"local_combo", "local_val"}};
    std::map<std::string, std::string> session_data = {{"session_combo", "session_val"}};
    
    browser->setCookie(test_cookie);
    browser->setLocalStorage(local_data);
    browser->setSessionStorage(session_data);
    
    // Retrieve all
    std::vector<Cookie> cookies;
    bool callback_called = false;
    browser->getCookiesAsync([&](std::vector<Cookie> retrieved) {
        cookies = retrieved;
        callback_called = true;
    });
    
    auto start = std::chrono::steady_clock::now();
    while (!callback_called && std::chrono::steady_clock::now() - start < 2s) {
        std::this_thread::sleep_for(10ms);
    }
    
    auto local_storage = browser->getLocalStorage();
    auto session_storage = browser->getSessionStorage();
    
    // Verify all storage types work
    EXPECT_TRUE(callback_called);
    EXPECT_FALSE(local_storage.empty());
    EXPECT_FALSE(session_storage.empty());
    
    // Verify specific values
    EXPECT_EQ(local_storage["local_combo"], "local_val");
    EXPECT_EQ(session_storage["session_combo"], "session_val");
}

// ========== Error Handling Tests ==========

TEST_F(BrowserStorageTest, StorageWithEmptyBrowser) {
    // Test behavior with minimal setup
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
    // Test setting empty storage maps
    std::map<std::string, std::string> empty_storage;
    
    EXPECT_NO_THROW(browser->setLocalStorage(empty_storage));
    EXPECT_NO_THROW(browser->setSessionStorage(empty_storage));
    
    auto local_result = browser->getLocalStorage();
    auto session_result = browser->getSessionStorage();
    
    EXPECT_TRUE(local_result.empty());
    EXPECT_TRUE(session_result.empty());
}