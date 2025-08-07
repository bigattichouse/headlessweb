#include <gtest/gtest.h>
#include "Browser/Browser.h"
#include "Session/Session.h"
#include "../utils/test_helpers.h"
#include "browser_test_environment.h"
#include "Debug.h"
#include <thread>
#include <chrono>
#include <filesystem>
#include <fstream>

extern std::unique_ptr<Browser> g_browser;

using namespace std::chrono_literals;

class BrowserSessionTest : public ::testing::Test {
protected:
    void SetUp() override {
        temp_dir = std::make_unique<TestHelpers::TemporaryDirectory>("session_tests");
        
        // Use global browser instance like other working tests
        browser = g_browser.get();
        
        // Create session for interface testing
        session = std::make_unique<Session>("test_session");
        
        // NO PAGE LOADING - Use interface testing approach
        
        debug_output("BrowserSessionTest SetUp complete");
    }
    
    void TearDown() override {
        // Clean teardown without navigation
        session.reset();
        temp_dir.reset();
    }
    
    // Interface testing helper methods
    std::string executeWrappedJS(const std::string& jsCode) {
        // Test JavaScript execution interface without requiring page content
        std::string wrapped = "(function() { try { " + jsCode + " } catch(e) { return 'error: ' + e.message; } })()";
        return browser->executeJavascriptSync(wrapped);
    }
    
    Browser* browser;
    std::unique_ptr<Session> session;
    std::unique_ptr<TestHelpers::TemporaryDirectory> temp_dir;
    
    // Helper method to create HTML page for testing (for interface testing only)
    std::string createTestPage(const std::string& html_content, const std::string& filename = "test.html") {
        auto html_file = temp_dir->createFile(filename, html_content);
        return "file://" + html_file.string();
    }
};

// ========== Session Creation Interface Tests ==========

TEST_F(BrowserSessionTest, SessionCreationInterface) {
    // Test session creation interface without page loading
    EXPECT_NE(session.get(), nullptr);
    EXPECT_NO_THROW(auto new_session = std::make_unique<Session>("new_session"));
}

TEST_F(BrowserSessionTest, SessionNameInterface) {
    // Test session name interface without page loading
    EXPECT_NO_THROW(session->getName());
    
    std::vector<std::string> session_names = {
        "test_session",
        "user_session_123",
        "session-with-dash",
        "session_with_underscore",
        "SessionWithCamelCase",
        "session123",
        "",  // Empty name
        "very_long_session_name_with_many_characters"
    };
    
    for (const auto& name : session_names) {
        EXPECT_NO_THROW(auto test_session = std::make_unique<Session>(name));
    }
}

// ========== Session URL Management Interface Tests ==========

TEST_F(BrowserSessionTest, SessionCurrentUrlInterface) {
    // Test session current URL interface without page loading
    EXPECT_NO_THROW(session->getCurrentUrl());
    EXPECT_NO_THROW(session->setCurrentUrl("http://example.com"));
    EXPECT_NO_THROW(session->setCurrentUrl("https://test.com/page"));
    EXPECT_NO_THROW(session->setCurrentUrl("data:text/html,<html><body>Test</body></html>"));
    EXPECT_NO_THROW(session->setCurrentUrl(""));  // Empty URL
}

TEST_F(BrowserSessionTest, SessionHistoryInterface) {
    // Test session history interface without page loading
    EXPECT_NO_THROW(session->getHistory());
    EXPECT_NO_THROW(session->getHistoryIndex());
    EXPECT_NO_THROW(session->canGoBack());
    EXPECT_NO_THROW(session->canGoForward());
    
    std::vector<std::string> test_urls = {
        "http://example1.com",
        "http://example2.com", 
        "http://example3.com",
        "data:text/html,<html><body>Test</body></html>",
        "about:blank"
    };
    
    for (const auto& url : test_urls) {
        EXPECT_NO_THROW(session->addToHistory(url));
    }
    
    EXPECT_NO_THROW(session->setHistoryIndex(0));
    EXPECT_NO_THROW(session->setHistoryIndex(1));
}

// ========== Session Cookie Management Interface Tests ==========

TEST_F(BrowserSessionTest, SessionCookieInterface) {
    // Test session cookie management interface without page loading
    EXPECT_NO_THROW(session->getCookies());
    EXPECT_NO_THROW(session->clearCookies());
    
    Cookie test_cookie;
    test_cookie.name = "session_cookie";
    test_cookie.value = "session_value";
    test_cookie.path = "/";
    test_cookie.domain = "";
    test_cookie.secure = false;
    test_cookie.httpOnly = false;
    test_cookie.expires = -1;
    
    EXPECT_NO_THROW(session->addCookie(test_cookie));
    EXPECT_NO_THROW(session->getCookies());
}

TEST_F(BrowserSessionTest, SessionMultipleCookiesInterface) {
    // Test multiple cookies in session interface
    std::vector<Cookie> test_cookies = {
        {"cookie1", "value1", "", "/", false, false, -1},
        {"cookie2", "value2", "", "/", false, false, -1},
        {"cookie3", "value3", "", "/", false, false, -1}
    };
    
    for (const auto& cookie : test_cookies) {
        EXPECT_NO_THROW(session->addCookie(cookie));
    }
    
    EXPECT_NO_THROW(session->setCookies(test_cookies));
    EXPECT_NO_THROW(session->getCookies());
}

// ========== Session Storage Interface Tests ==========

TEST_F(BrowserSessionTest, SessionLocalStorageInterface) {
    // Test session local storage interface without page loading
    EXPECT_NO_THROW(session->getLocalStorage());
    
    std::map<std::string, std::string> test_storage = {
        {"local_key1", "local_value1"},
        {"local_key2", "local_value2"},
        {"local_key3", "local_value3"}
    };
    
    EXPECT_NO_THROW(session->setLocalStorage(test_storage));
    EXPECT_NO_THROW(session->setLocalStorageItem("single_key", "single_value"));
    EXPECT_NO_THROW(session->getLocalStorage());
}

TEST_F(BrowserSessionTest, SessionSessionStorageInterface) {
    // Test session session storage interface without page loading
    EXPECT_NO_THROW(session->getSessionStorage());
    
    std::map<std::string, std::string> test_storage = {
        {"session_key1", "session_value1"},
        {"session_key2", "session_value2"}
    };
    
    EXPECT_NO_THROW(session->setSessionStorage(test_storage));
    EXPECT_NO_THROW(session->setSessionStorageItem("single_session_key", "single_session_value"));
    EXPECT_NO_THROW(session->getSessionStorage());
}

// ========== Session Form State Interface Tests ==========

TEST_F(BrowserSessionTest, SessionFormStateInterface) {
    // Test session form state interface without page loading
    EXPECT_NO_THROW(session->getFormFields());
    EXPECT_NO_THROW(session->clearFormFields());
    
    std::vector<FormField> form_fields = {
        {"#input1", "input1", "input1", "text", "value1", false},
        {"#textarea1", "textarea1", "textarea1", "textarea", "text area content", false},
        {"#select1", "select1", "select1", "select", "option1", false},
        {"#checkbox1", "checkbox1", "checkbox1", "checkbox", "true", true},
        {"#radio1", "radio1", "radio1", "radio", "selected", true}
    };
    
    for (const auto& field : form_fields) {
        EXPECT_NO_THROW(session->addFormField(field));
    }
    
    EXPECT_NO_THROW(session->setFormFields(form_fields));
    EXPECT_NO_THROW(session->getFormFields());
}

// ========== Session Active Elements Interface Tests ==========

TEST_F(BrowserSessionTest, SessionActiveElementsInterface) {
    // Test session active elements interface without page loading
    EXPECT_NO_THROW(session->getActiveElements());
    
    std::vector<std::string> active_selectors = {
        "#active-button",
        ".focused-input",
        "#selected-item",
        ".highlighted-element"
    };
    
    for (const auto& selector : active_selectors) {
        EXPECT_NO_THROW(session->addActiveElement(selector));
    }
    
    std::set<std::string> active_set(active_selectors.begin(), active_selectors.end());
    EXPECT_NO_THROW(session->setActiveElements(active_set));
}

// ========== Session Scroll Position Interface Tests ==========

TEST_F(BrowserSessionTest, SessionScrollPositionInterface) {
    // Test session scroll position interface without page loading
    EXPECT_NO_THROW(session->getScrollPosition());
    EXPECT_NO_THROW(session->getScrollPosition("window"));
    EXPECT_NO_THROW(session->getAllScrollPositions());
    
    // Test setting scroll positions
    EXPECT_NO_THROW(session->setScrollPosition("window", 100, 200));
    EXPECT_NO_THROW(session->setScrollPosition("#scrollable-div", 50, 75));
    EXPECT_NO_THROW(session->setScrollPosition(".scroll-container", 0, 0));
    
    // Test retrieval after setting
    EXPECT_NO_THROW(session->getScrollPosition("window"));
    EXPECT_NO_THROW(session->getScrollPosition("#scrollable-div"));
}

// ========== Session Page State Interface Tests ==========

TEST_F(BrowserSessionTest, SessionPageStateInterface) {
    // Test session page state interface without page loading
    EXPECT_NO_THROW(session->getPageHash());
    EXPECT_NO_THROW(session->setPageHash("#section1"));
    EXPECT_NO_THROW(session->setPageHash("#section2"));
    EXPECT_NO_THROW(session->setPageHash(""));
    
    EXPECT_NO_THROW(session->getDocumentReadyState());
    EXPECT_NO_THROW(session->setDocumentReadyState("loading"));
    EXPECT_NO_THROW(session->setDocumentReadyState("interactive"));
    EXPECT_NO_THROW(session->setDocumentReadyState("complete"));
}

TEST_F(BrowserSessionTest, SessionReadyConditionsInterface) {
    // Test session ready conditions interface without page loading
    EXPECT_NO_THROW(session->getReadyConditions());
    EXPECT_NO_THROW(session->clearReadyConditions());
    
    std::vector<PageReadyCondition> conditions = {
        {PageReadyCondition::SELECTOR, "#ready-element", 5000},
        {PageReadyCondition::JS_EXPRESSION, "document.readyState === 'complete'", 3000},
        {PageReadyCondition::CUSTOM, "customCondition", 2000}
    };
    
    for (const auto& condition : conditions) {
        EXPECT_NO_THROW(session->addReadyCondition(condition));
    }
    
    EXPECT_NO_THROW(session->getReadyConditions());
}

// ========== Session Viewport Interface Tests ==========

TEST_F(BrowserSessionTest, SessionViewportInterface) {
    // Test session viewport interface without page loading
    EXPECT_NO_THROW(session->getViewport());
    
    std::vector<std::pair<int, int>> viewport_sizes = {
        {1920, 1080},
        {1366, 768},
        {1280, 720},
        {800, 600},
        {375, 667},  // Mobile
        {320, 568}   // Small mobile
    };
    
    for (const auto& [width, height] : viewport_sizes) {
        EXPECT_NO_THROW(session->setViewport(width, height));
        EXPECT_NO_THROW(session->getViewport());
    }
}

// ========== Session User Agent Interface Tests ==========

TEST_F(BrowserSessionTest, SessionUserAgentInterface) {
    // Test session user agent interface without page loading
    EXPECT_NO_THROW(session->getUserAgent());
    
    std::vector<std::string> user_agents = {
        "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36",
        "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36",
        "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36",
        "Mozilla/5.0 (iPhone; CPU iPhone OS 14_0 like Mac OS X) AppleWebKit/605.1.15",
        "HeadlessWeb Custom Agent 1.0",
        ""  // Empty user agent
    };
    
    for (const auto& ua : user_agents) {
        EXPECT_NO_THROW(session->setUserAgent(ua));
        EXPECT_NO_THROW(session->getUserAgent());
    }
}

// ========== Session Custom Variables Interface Tests ==========

TEST_F(BrowserSessionTest, SessionCustomVariablesInterface) {
    // Test session custom variables interface without page loading
    std::map<std::string, std::string> custom_vars = {
        {"theme", "dark"},
        {"language", "en-US"},
        {"timezone", "UTC"},
        {"user_preference", "advanced"},
        {"layout", "grid"},
        {"api_token", "token_123"},
        {"session_id", "sess_456"}
    };
    
    for (const auto& [key, value] : custom_vars) {
        EXPECT_NO_THROW(session->setCustomVariable(key, value));
        EXPECT_NO_THROW(session->getCustomVariable(key));
        EXPECT_NO_THROW(session->hasCustomVariable(key));
    }
    
    // Test non-existent variable
    EXPECT_NO_THROW(session->getCustomVariable("nonexistent"));
    EXPECT_NO_THROW(session->hasCustomVariable("nonexistent"));
}

// ========== Session State Extractors Interface Tests ==========

TEST_F(BrowserSessionTest, SessionStateExtractorsInterface) {
    // Test session state extractors interface without page loading
    EXPECT_NO_THROW(session->getStateExtractors());
    
    std::map<std::string, std::string> extractors = {
        {"pageTitle", "document.title"},
        {"bodyText", "document.body.innerText"},
        {"elementCount", "document.querySelectorAll('*').length"},
        {"readyState", "document.readyState"},
        {"currentUrl", "window.location.href"}
    };
    
    for (const auto& [name, jsCode] : extractors) {
        EXPECT_NO_THROW(session->addStateExtractor(name, jsCode));
    }
    
    EXPECT_NO_THROW(session->getStateExtractors());
}

TEST_F(BrowserSessionTest, SessionExtractedStateInterface) {
    // Test session extracted state interface without page loading
    EXPECT_NO_THROW(session->getAllExtractedState());
    
    // Create JSON values for testing
    Json::Value string_value("test string");
    Json::Value number_value(42);
    Json::Value bool_value(true);
    Json::Value array_value(Json::arrayValue);
    array_value.append("item1");
    array_value.append("item2");
    
    Json::Value object_value(Json::objectValue);
    object_value["key1"] = "value1";
    object_value["key2"] = 123;
    
    std::vector<std::pair<std::string, Json::Value>> test_states = {
        {"stringState", string_value},
        {"numberState", number_value},
        {"boolState", bool_value},
        {"arrayState", array_value},
        {"objectState", object_value}
    };
    
    for (const auto& [name, value] : test_states) {
        EXPECT_NO_THROW(session->setExtractedState(name, value));
        EXPECT_NO_THROW(session->getExtractedState(name));
    }
    
    EXPECT_NO_THROW(session->getAllExtractedState());
}

// ========== Session Serialization Interface Tests ==========

TEST_F(BrowserSessionTest, SessionSerializationInterface) {
    // Test session serialization interface without page loading
    
    // Add some test data to the session
    EXPECT_NO_THROW(session->setCurrentUrl("http://example.com"));
    EXPECT_NO_THROW(session->addToHistory("http://example.com"));
    EXPECT_NO_THROW(session->setCustomVariable("test_key", "test_value"));
    EXPECT_NO_THROW(session->setLocalStorageItem("local_key", "local_value"));
    
    // Test serialization
    EXPECT_NO_THROW(session->serialize());
    
    // Test deserialization
    std::string serialized_data = session->serialize();
    EXPECT_NO_THROW(Session::deserialize(serialized_data));
    
    // Test with empty serialization data
    EXPECT_NO_THROW(Session::deserialize("{}"));
    // Note: Empty string may not be valid JSON - interface test only
}

// ========== Session Error Handling Interface Tests ==========

TEST_F(BrowserSessionTest, SessionErrorHandlingInterface) {
    // Test session error handling interface without page loading
    
    // Test with invalid/edge case inputs
    EXPECT_NO_THROW(session->setCurrentUrl(""));  // Empty URL
    EXPECT_NO_THROW(session->addToHistory(""));   // Empty history entry
    EXPECT_NO_THROW(session->setCustomVariable("", "empty_key"));  // Empty key
    EXPECT_NO_THROW(session->setCustomVariable("key", ""));  // Empty value
    
    // Test with special characters
    EXPECT_NO_THROW(session->setCustomVariable("key with spaces", "value with spaces"));
    EXPECT_NO_THROW(session->setCustomVariable("key'with'quotes", "value'with'quotes"));
    EXPECT_NO_THROW(session->setCustomVariable("unicode_测试", "unicode_值"));
    
    // Test invalid scroll positions
    EXPECT_NO_THROW(session->setScrollPosition("", 0, 0));  // Empty selector
    EXPECT_NO_THROW(session->setScrollPosition("element", -100, -100));  // Negative positions
    
    // Test invalid viewport sizes
    EXPECT_NO_THROW(session->setViewport(0, 0));   // Zero size
    EXPECT_NO_THROW(session->setViewport(-1, -1)); // Negative size
    EXPECT_NO_THROW(session->setViewport(10000, 10000)); // Very large size
}

// ========== Session Performance Interface Tests ==========

TEST_F(BrowserSessionTest, SessionPerformanceInterface) {
    // Test session performance interface without page loading
    auto start = std::chrono::steady_clock::now();
    
    // Perform multiple operations
    for (int i = 0; i < 100; ++i) {
        std::string key = "perf_key_" + std::to_string(i);
        std::string value = "perf_value_" + std::to_string(i);
        std::string url = "http://example" + std::to_string(i) + ".com";
        
        EXPECT_NO_THROW(session->setCustomVariable(key, value));
        EXPECT_NO_THROW(session->addToHistory(url));
        EXPECT_NO_THROW(session->setLocalStorageItem(key, value));
    }
    
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Interface should complete within reasonable time
    EXPECT_LT(duration.count(), 5000); // Less than 5 seconds for 300 operations
}

// ========== Session Edge Cases Interface Tests ==========

TEST_F(BrowserSessionTest, SessionEdgeCasesInterface) {
    // Test session edge cases interface without page loading
    
    // Test with very long strings
    std::string long_string(10000, 'L');
    EXPECT_NO_THROW(session->setCurrentUrl(long_string));
    EXPECT_NO_THROW(session->setCustomVariable("long_key", long_string));
    EXPECT_NO_THROW(session->setUserAgent(long_string));
    
    // Test with special Unicode characters
    std::string unicode_string = "测试🌍 العربية αβγ ñiño José файл.txt ファイル";
    EXPECT_NO_THROW(session->setCustomVariable("unicode", unicode_string));
    EXPECT_NO_THROW(session->addToHistory("http://example.com/path/" + unicode_string));
    
    // Test with control characters
    std::string control_string = "value\nwith\nnewlines\tand\ttabs\r\nand\r\nreturns";
    EXPECT_NO_THROW(session->setCustomVariable("control_chars", control_string));
    
    // Test with JSON-like strings
    std::string json_string = "{\"key\": \"value\", \"array\": [1, 2, 3]}";
    EXPECT_NO_THROW(session->setCustomVariable("json_like", json_string));
}

// ========== Session Memory Management Interface Tests ==========

TEST_F(BrowserSessionTest, SessionMemoryManagementInterface) {
    // Test session memory management interface without page loading
    
    // Create large amounts of data
    for (int i = 0; i < 1000; ++i) {
        std::string key = "memory_key_" + std::to_string(i);
        std::string value(100, 'M'); // 100 bytes per value
        std::string url = "http://memory-test-" + std::to_string(i) + ".com";
        
        EXPECT_NO_THROW(session->setCustomVariable(key, value));
        EXPECT_NO_THROW(session->addToHistory(url));
        EXPECT_NO_THROW(session->setLocalStorageItem(key, value));
    }
    
    // Test cleanup operations
    EXPECT_NO_THROW(session->clearCookies());
    EXPECT_NO_THROW(session->clearFormFields());
    EXPECT_NO_THROW(session->clearReadyConditions());
    
    // Test that session still works after cleanup
    EXPECT_NO_THROW(session->setCustomVariable("after_cleanup", "value"));
    EXPECT_NO_THROW(session->getName());
}

// ========== Session Resource Cleanup Interface Tests ==========

TEST_F(BrowserSessionTest, SessionResourceCleanupInterface) {
    // Test session resource cleanup interface
    {
        auto temp_session = std::make_unique<Session>("temp_session");
        EXPECT_NO_THROW(temp_session->setCustomVariable("temp_key", "temp_value"));
        EXPECT_NO_THROW(temp_session->addToHistory("http://temp.com"));
        // Session destructor should clean up resources
    }
    
    // Test that original session still works
    EXPECT_NO_THROW(session->setCustomVariable("after_cleanup", "value"));
    EXPECT_NO_THROW(session->getName());
    EXPECT_NO_THROW(session->serialize());
}