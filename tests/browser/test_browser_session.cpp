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
        // Use global browser instance (properly initialized)
        browser = g_browser.get();
        session = std::make_unique<Session>("test_session");
        
        // Create temporary directory for file:// URLs
        temp_dir = std::make_unique<TestHelpers::TemporaryDirectory>("session_tests");
        
        // Reset browser to clean state before each test
        browser->loadUri("about:blank");
        browser->waitForNavigation(2000);
        
        // Load a comprehensive test page for session testing
        setupTestPage();
        
        debug_output("BrowserSessionTest SetUp complete");
    }
    
    // Generic JavaScript wrapper function for safe execution
    std::string executeWrappedJS(const std::string& jsCode) {
        std::string wrapped = "(function() { " + jsCode + " })()";
        return browser->executeJavascriptSync(wrapped);
    }
    
    // Enhanced page loading method based on successful BrowserMainTest approach
    bool loadPageWithReadinessCheck(const std::string& url) {
        browser->loadUri(url);
        
        // Wait for navigation
        bool nav_success = browser->waitForNavigation(5000);
        if (!nav_success) return false;
        
        // Allow WebKit processing time
        std::this_thread::sleep_for(1000ms);
        
        // Check basic JavaScript execution with retry
        for (int i = 0; i < 5; i++) {
            std::string js_test = executeWrappedJS("return 'test';");
            if (js_test == "test") break;
            if (i == 4) return false;
            std::this_thread::sleep_for(200ms);
        }
        
        // Verify DOM is ready with specific element checks
        for (int i = 0; i < 5; i++) {
            std::string dom_check = executeWrappedJS(
                "return document.readyState === 'complete' && "
                "document.getElementById('text-input') !== null && "
                "document.getElementById('checkbox1') !== null && "
                "document.getElementById('select-single') !== null;"
            );
            if (dom_check == "true") break;
            if (i == 4) return false;
            std::this_thread::sleep_for(200ms);
        }
        
        return true;
    }

    void setupTestPage() {
        std::string simple_html = R"HTML(
<!DOCTYPE html>
<html>
<head><title>Session Test</title></head>
<body>
    <h1>Session Test</h1>
    <form>
        <input type="text" id="text-input" value="initial" />
        <input type="email" id="email-input" placeholder="Email" />
        <input type="number" id="number-input" placeholder="Number" />
        <textarea id="textarea-input" placeholder="Textarea content">Initial textarea</textarea>
        <input type="checkbox" id="checkbox1" checked />
        <input type="checkbox" id="checkbox2" />
        <select id="select-single">
            <option value="opt1" selected>Option 1</option>
            <option value="option2">Option 2</option>
            <option value="option3">Option 3</option>
        </select>
        <button type="button" id="focus-btn">Focus</button>
    </form>
    <div id="custom-widget" data-state="active">Widget</div>
</body>
</html>
)HTML";
        
        // CRITICAL FIX: Use proven approach from BrowserMainTest
        test_html_file = temp_dir->createFile("session_test.html", simple_html);
        std::string file_url = "file://" + test_html_file.string();
        
        // Use enhanced page loading with comprehensive readiness check
        bool page_ready = loadPageWithReadinessCheck(file_url);
        if (!page_ready) {
            std::cerr << "BrowserSessionTest: Page failed to load and become ready" << std::endl;
        }
    }

    void TearDown() override {
        // Clean up (don't destroy global browser)
        session.reset();
        temp_dir.reset();
    }

    Browser* browser;  // Raw pointer to global browser instance
    std::unique_ptr<Session> session;
    std::unique_ptr<TestHelpers::TemporaryDirectory> temp_dir;
    std::filesystem::path test_html_file;
};

// ========== Session State Update Tests ==========

TEST_F(BrowserSessionTest, UpdateSessionStateBasic) {
    // Update session state from current page
    browser->updateSessionState(*session);
    
    // Verify basic state was captured
    EXPECT_FALSE(session->getCurrentUrl().empty());
    EXPECT_NE(session->getCurrentUrl().find("file://"), std::string::npos);
    EXPECT_EQ(session->getDocumentReadyState(), "complete");
    EXPECT_GT(session->getLastAccessed(), 0);
}

TEST_F(BrowserSessionTest, UpdateSessionStateWithFormData) {
    // Modify form fields
    browser->fillInput("#text-input", "modified text");
    browser->fillInput("#email-input", "test@example.com");
    browser->fillInput("#number-input", "100");
    browser->checkElement("#checkbox2");
    browser->uncheckElement("#checkbox1");
    browser->selectOption("#select-single", "option2");
    browser->fillInput("#textarea-input", "Modified textarea content");
    
    std::this_thread::sleep_for(200ms);
    
    // Update session state
    browser->updateSessionState(*session);
    
    // Verify form fields were captured
    auto formFields = session->getFormFields();
    EXPECT_GT(formFields.size(), 0);
    
    // Find specific fields and verify their values
    bool foundTextInput = false;
    bool foundEmailInput = false;
    bool foundCheckbox2 = false;
    
    for (const auto& field : formFields) {
        if (field.selector == "#text-input") {
            EXPECT_EQ(field.value, "modified text");
            foundTextInput = true;
        } else if (field.selector == "#email-input") {
            EXPECT_EQ(field.value, "test@example.com");
            foundEmailInput = true;
        } else if (field.selector == "#checkbox2") {
            EXPECT_TRUE(field.checked);
            foundCheckbox2 = true;
        }
    }
    
    EXPECT_TRUE(foundTextInput);
    EXPECT_TRUE(foundEmailInput);
    EXPECT_TRUE(foundCheckbox2);
}

TEST_F(BrowserSessionTest, UpdateSessionStateWithStorage) {
    // Set storage data via JavaScript using wrapper function
    executeWrappedJS(
        "localStorage.setItem('testKey1', 'testValue1'); "
        "sessionStorage.setItem('sessionKey1', 'sessionValue1');"
    );
    std::this_thread::sleep_for(200ms);
    
    // Update session state
    browser->updateSessionState(*session);
    
    // Verify storage was captured (if not file:// URL)
    std::string currentUrl = browser->getCurrentUrl();
    if (currentUrl.find("file://") != 0) {
        auto localStorage = session->getLocalStorage();
        auto sessionStorage = session->getSessionStorage();
        
        EXPECT_GT(localStorage.size(), 0);
        EXPECT_GT(sessionStorage.size(), 0);
        
        EXPECT_EQ(localStorage["testKey1"], "testValue1");
        EXPECT_EQ(sessionStorage["sessionKey1"], "sessionValue1");
    }
}

TEST_F(BrowserSessionTest, UpdateSessionStateWithScrollPosition) {
    // Scroll to a specific position
    browser->setScrollPosition(50, 100);
    std::this_thread::sleep_for(200ms);
    
    // Update session state
    browser->updateSessionState(*session);
    
    // Verify scroll position was captured
    auto scrollPos = session->getScrollPosition("window");
    EXPECT_EQ(scrollPos.first, 50);
    EXPECT_EQ(scrollPos.second, 100);
}

TEST_F(BrowserSessionTest, UpdateSessionStateWithActiveElements) {
    // Focus an element
    browser->focusElement("#focus-btn");
    std::this_thread::sleep_for(200ms);
    
    // Update session state
    browser->updateSessionState(*session);
    
    // Verify active element was captured
    auto activeElements = session->getActiveElements();
    EXPECT_GT(activeElements.size(), 0);
    EXPECT_NE(activeElements.find("#focus-btn"), activeElements.end());
}

TEST_F(BrowserSessionTest, UpdateSessionStateWithPageHash) {
    // Change page hash using wrapper function
    executeWrappedJS("window.location.hash = '#test-section';");
    std::this_thread::sleep_for(200ms);
    
    // Update session state
    browser->updateSessionState(*session);
    
    // Verify hash was captured
    EXPECT_EQ(session->getPageHash(), "#test-section");
}

TEST_F(BrowserSessionTest, UpdateSessionStateWithCustomAttributes) {
    // Update custom attributes using wrapper function
    executeWrappedJS(
        "document.getElementById('custom-widget').setAttribute('data-state', 'updated'); "
        "document.getElementById('custom-widget').setAttribute('data-custom', 'test-value');"
    );
    std::this_thread::sleep_for(200ms);
    
    // Update session state
    browser->updateSessionState(*session);
    
    // Verify custom attributes were extracted
    Json::Value customAttrs = session->getExtractedState("customAttributes");
    EXPECT_FALSE(customAttrs.isNull());
    EXPECT_TRUE(customAttrs.isObject());
}

// ========== Session Restoration Tests ==========

TEST_F(BrowserSessionTest, RestoreSessionBasic) {
    // Prepare session with basic data
    session->setCurrentUrl(browser->getCurrentUrl());
    session->setUserAgent("HeadlessWeb Test Agent");
    
    // CRITICAL FIX: Use global browser instead of creating new instance
    EXPECT_NO_THROW(browser->restoreSession(*session));
    
    // Enhanced readiness checking after restoration
    bool page_ready = loadPageWithReadinessCheck(browser->getCurrentUrl());
    EXPECT_TRUE(page_ready);
    
    // Verify restoration using enhanced wrapper function
    std::string userAgent = executeWrappedJS("return navigator.userAgent;");
    EXPECT_NE(userAgent.find("HeadlessWeb Test Agent"), std::string::npos);
}

TEST_F(BrowserSessionTest, RestoreSessionWithFormState) {
    // Set up form state in session
    std::vector<FormField> formFields;
    
    FormField textField;
    textField.selector = "#text-input";
    textField.value = "restored text";
    textField.type = "text";
    formFields.push_back(textField);
    
    FormField checkboxField;
    checkboxField.selector = "#checkbox2";
    checkboxField.checked = true;
    checkboxField.type = "checkbox";
    formFields.push_back(checkboxField);
    
    FormField selectField;
    selectField.selector = "#select-single";
    selectField.value = "option3";
    selectField.type = "select";
    formFields.push_back(selectField);
    
    session->setFormFields(formFields);
    session->setCurrentUrl(browser->getCurrentUrl());
    
    // CRITICAL FIX: Use global browser instead of creating new instance
    browser->restoreSession(*session);
    
    // Enhanced readiness checking after restoration
    bool page_ready = loadPageWithReadinessCheck(browser->getCurrentUrl());
    EXPECT_TRUE(page_ready);
    
    // Verify form restoration with enhanced JavaScript wrapper calls
    std::string textValue = browser->getAttribute("#text-input", "value");
    EXPECT_EQ(textValue, "restored text");
    
    std::string checkboxChecked = executeWrappedJS("return document.getElementById('checkbox2').checked;");
    EXPECT_EQ(checkboxChecked, "true");
    
    std::string selectValue = browser->getAttribute("#select-single", "value");
    EXPECT_EQ(selectValue, "option3");
}

TEST_F(BrowserSessionTest, RestoreSessionWithScrollPosition) {
    // Set scroll position in session
    session->setScrollPosition("window", 100, 200);
    session->setCurrentUrl(browser->getCurrentUrl());
    
    // CRITICAL FIX: Use global browser instead of creating new instance
    browser->restoreSession(*session);
    
    // Enhanced readiness checking after restoration
    bool page_ready = loadPageWithReadinessCheck(browser->getCurrentUrl());
    EXPECT_TRUE(page_ready);
    
    // Allow time for scroll position restoration
    std::this_thread::sleep_for(500ms);
    
    // Verify scroll restoration
    auto [x, y] = browser->getScrollPosition();
    EXPECT_EQ(x, 100);
    EXPECT_EQ(y, 200);
}

TEST_F(BrowserSessionTest, RestoreSessionWithActiveElements) {
    // Set active elements in session
    std::set<std::string> activeElements = {"#focus-btn"};
    session->setActiveElements(activeElements);
    session->setCurrentUrl(browser->getCurrentUrl());
    
    // CRITICAL FIX: Use global browser instead of creating new instance
    browser->restoreSession(*session);
    
    // Enhanced readiness checking after restoration
    bool page_ready = loadPageWithReadinessCheck(browser->getCurrentUrl());
    EXPECT_TRUE(page_ready);
    
    // Allow time for active element restoration
    std::this_thread::sleep_for(500ms);
    
    // Verify active element restoration using enhanced wrapper
    std::string focusedElement = executeWrappedJS("return document.activeElement ? document.activeElement.id : '';");
    EXPECT_EQ(focusedElement, "focus-btn");
}

TEST_F(BrowserSessionTest, RestoreSessionWithCustomState) {
    // Set custom state in session
    Json::Value customState;
    customState["appData"] = "test value";
    customState["userSettings"]["theme"] = "dark";
    customState["userSettings"]["lang"] = "en";
    
    session->setExtractedState("appData", customState["appData"]);
    session->setExtractedState("userSettings", customState["userSettings"]);
    session->setCurrentUrl(browser->getCurrentUrl());
    
    // CRITICAL FIX: Use global browser instead of creating new instance
    browser->restoreSession(*session);
    
    // Enhanced readiness checking after restoration
    bool page_ready = loadPageWithReadinessCheck(browser->getCurrentUrl());
    EXPECT_TRUE(page_ready);
    
    // Allow time for custom state restoration
    std::this_thread::sleep_for(500ms);
    
    // Verify custom state restoration using enhanced wrapper
    std::string restoredAppData = executeWrappedJS("return window['_hweb_custom_appData'] || '';");
    EXPECT_EQ(restoredAppData, "test value");
    
    std::string restoredTheme = executeWrappedJS("return window['_hweb_custom_userSettings'] && window['_hweb_custom_userSettings'].theme || '';");
    EXPECT_EQ(restoredTheme, "dark");
}

// ========== Safe Session Restoration Tests ==========

TEST_F(BrowserSessionTest, RestoreSessionSafely) {
    // Valid session
    session->setCurrentUrl(browser->getCurrentUrl());
    
    bool result = browser->restoreSessionSafely(*session);
    EXPECT_TRUE(result);
}

TEST_F(BrowserSessionTest, RestoreSessionSafelyWithInvalidUrl) {
    // Invalid URL
    session->setCurrentUrl("invalid://url");
    
    // Test URL validation first
    bool urlValid = browser->validateUrl("invalid://url");
    EXPECT_FALSE(urlValid) << "URL validation should reject invalid:// URLs";
    
    bool result = browser->restoreSessionSafely(*session);
    EXPECT_FALSE(result) << "Safe restoration should fail for invalid URLs";
}

// ========== Session Validation Tests ==========

TEST_F(BrowserSessionTest, ValidateSession) {
    // Valid session with name
    EXPECT_TRUE(browser->validateSession(*session));
    
    // Invalid session without name
    Session emptySession("");
    EXPECT_FALSE(browser->validateSession(emptySession));
}

// ========== Form State Management Tests ==========

TEST_F(BrowserSessionTest, ExtractFormState) {
    // Modify some form fields
    browser->fillInput("#text-input", "test text");
    browser->fillInput("#email-input", "test@example.com");
    browser->checkElement("#checkbox2");
    browser->selectOption("#select-single", "option2");
    
    std::this_thread::sleep_for(200ms);
    
    // Extract form state
    auto formFields = browser->extractFormState();
    
    EXPECT_GT(formFields.size(), 0);
    
    // Verify specific fields
    bool foundTextInput = false;
    bool foundEmailInput = false;
    bool foundCheckbox = false;
    
    for (const auto& field : formFields) {
        if (field.selector == "#text-input") {
            EXPECT_EQ(field.value, "test text");
            foundTextInput = true;
        } else if (field.selector == "#email-input") {
            EXPECT_EQ(field.value, "test@example.com");
            foundEmailInput = true;
        } else if (field.selector == "#checkbox2") {
            EXPECT_TRUE(field.checked);
            foundCheckbox = true;
        }
    }
    
    EXPECT_TRUE(foundTextInput);
    EXPECT_TRUE(foundEmailInput);
    EXPECT_TRUE(foundCheckbox);
}

TEST_F(BrowserSessionTest, RestoreFormState) {
    // Create form fields to restore
    std::vector<FormField> formFields;
    
    FormField textField;
    textField.selector = "#text-input";
    textField.value = "restored text";
    textField.type = "text";
    formFields.push_back(textField);
    
    FormField checkboxField;
    checkboxField.selector = "#checkbox1";
    checkboxField.checked = false;
    checkboxField.type = "checkbox";
    formFields.push_back(checkboxField);
    
    FormField selectField;
    selectField.selector = "#select-single";
    selectField.value = "option3";
    selectField.type = "select";
    formFields.push_back(selectField);
    
    // Restore form state with enhanced timing
    browser->restoreFormState(formFields);
    
    // Allow time for form restoration to complete
    std::this_thread::sleep_for(500ms);
    
    // Verify DOM is still ready after restoration
    std::string dom_check = executeWrappedJS("return document.getElementById('text-input') !== null;");
    EXPECT_EQ(dom_check, "true");
    
    // Verify restoration
    std::string textValue = browser->getAttribute("#text-input", "value");
    EXPECT_EQ(textValue, "restored text");
    
    std::string selectValue = browser->getAttribute("#select-single", "value");
    EXPECT_EQ(selectValue, "option3");
    
    std::string checkboxChecked = executeWrappedJS("return document.getElementById('checkbox1').checked;");
    EXPECT_EQ(checkboxChecked, "false");
}

// ========== Active Elements Management Tests ==========

TEST_F(BrowserSessionTest, ExtractActiveElements) {
    // Focus an element
    browser->focusElement("#focus-btn");
    std::this_thread::sleep_for(200ms);
    
    // Extract active elements
    auto activeElements = browser->extractActiveElements();
    
    EXPECT_GT(activeElements.size(), 0);
    EXPECT_NE(activeElements.find("#focus-btn"), activeElements.end());
}

TEST_F(BrowserSessionTest, RestoreActiveElements) {
    // Set active elements
    std::set<std::string> elements = {"#focus-btn"};
    
    browser->restoreActiveElements(elements);
    std::this_thread::sleep_for(200ms);
    
    // Verify restoration
    std::string focusedElement = executeWrappedJS(
        "return document.activeElement ? document.activeElement.id : '';");
    EXPECT_EQ(focusedElement, "focus-btn");
}

// ========== Page State Extraction Tests ==========

TEST_F(BrowserSessionTest, ExtractPageHash) {
    // Set page hash using wrapper function
    executeWrappedJS("window.location.hash = '#test-hash';");
    std::this_thread::sleep_for(100ms);
    
    std::string hash = browser->extractPageHash();
    EXPECT_EQ(hash, "#test-hash");
}

TEST_F(BrowserSessionTest, ExtractDocumentReadyState) {
    std::string readyState = browser->extractDocumentReadyState();
    EXPECT_EQ(readyState, "complete");
}

// ========== Scroll Position Management Tests ==========

TEST_F(BrowserSessionTest, ExtractAllScrollPositions) {
    // Set scroll position
    browser->setScrollPosition(100, 200);
    std::this_thread::sleep_for(200ms);
    
    auto positions = browser->extractAllScrollPositions();
    
    EXPECT_GT(positions.size(), 0);
    EXPECT_NE(positions.find("window"), positions.end());
    
    auto windowPos = positions["window"];
    EXPECT_EQ(windowPos.first, 100);
    EXPECT_EQ(windowPos.second, 200);
}

TEST_F(BrowserSessionTest, RestoreScrollPositions) {
    // Create scroll positions map
    std::map<std::string, std::pair<int, int>> positions;
    positions["window"] = std::make_pair(150, 250);
    
    browser->restoreScrollPositions(positions);
    std::this_thread::sleep_for(200ms);
    
    // Verify restoration
    auto [x, y] = browser->getScrollPosition();
    EXPECT_EQ(x, 150);
    EXPECT_EQ(y, 250);
}

// ========== Custom State Management Tests ==========

TEST_F(BrowserSessionTest, ExtractCustomState) {
    // Set custom state via JavaScript using wrapper function
    executeWrappedJS("window.testData = {key: 'value', number: 42};");
    std::this_thread::sleep_for(100ms);
    
    // Create extractors
    std::map<std::string, std::string> extractors;
    extractors["testData"] = "window.testData";
    extractors["timestamp"] = "Date.now()";
    
    Json::Value customState = browser->extractCustomState(extractors);
    
    EXPECT_TRUE(customState.isMember("testData"));
    EXPECT_TRUE(customState.isMember("timestamp"));
    
    EXPECT_EQ(customState["testData"]["key"].asString(), "value");
    EXPECT_EQ(customState["testData"]["number"].asInt(), 42);
}

TEST_F(BrowserSessionTest, RestoreCustomState) {
    // Create custom state
    std::map<std::string, Json::Value> state;
    
    Json::Value testData;
    testData["restored"] = true;
    testData["value"] = "test";
    
    state["testData"] = testData;
    state["simpleValue"] = "simple string";
    
    browser->restoreCustomState(state);
    
    // Allow more time for custom state restoration
    std::this_thread::sleep_for(500ms);
    
    // Verify restoration with error checking
    std::string restoredData = executeWrappedJS(
        "return window['_hweb_custom_testData'] && window['_hweb_custom_testData'].restored ? 'true' : 'false';"
    );
    EXPECT_EQ(restoredData, "true");
    
    std::string simpleValue = executeWrappedJS(
        "return window['_hweb_custom_simpleValue'] || '';"
    );
    EXPECT_EQ(simpleValue, "simple string");
}

// ========== Error Handling Tests ==========

TEST_F(BrowserSessionTest, SessionHandlingWithEmptyPage) {
    // Load empty page using file:// URL
    std::string empty_html = "<html><body></body></html>";
    auto empty_file = temp_dir->createFile("empty_test.html", empty_html);
    std::string empty_url = "file://" + empty_file.string();
    browser->loadUri(empty_url);
    
    // Wait for navigation
    browser->waitForNavigation(5000);
    std::this_thread::sleep_for(500ms);
    
    // Should not crash
    EXPECT_NO_THROW(browser->updateSessionState(*session));
    EXPECT_NO_THROW(browser->restoreSession(*session));
}

TEST_F(BrowserSessionTest, SessionHandlingWithInvalidOperations) {
    // Try to restore invalid form fields
    std::vector<FormField> invalidFields;
    FormField invalid;
    invalid.selector = "#nonexistent";
    invalid.value = "test";
    invalidFields.push_back(invalid);
    
    // Should not crash
    EXPECT_NO_THROW(browser->restoreFormState(invalidFields));
    
    // Try to restore invalid active elements
    std::set<std::string> invalidElements = {"#nonexistent"};
    EXPECT_NO_THROW(browser->restoreActiveElements(invalidElements));
}

// ========== Integration Tests ==========

TEST_F(BrowserSessionTest, FullSessionSaveAndRestore) {
    // Modify page state
    browser->fillInput("#text-input", "full test");
    browser->checkElement("#checkbox2");
    browser->selectOption("#select-single", "option2");
    browser->setScrollPosition(50, 75);
    browser->focusElement("#focus-btn");
    executeWrappedJS("window.location.hash = '#full-test';");
    
    std::this_thread::sleep_for(300ms);
    
    // Extract full session state
    browser->updateSessionState(*session);
    
    // CRITICAL FIX: Use global browser instead of creating new instance
    browser->restoreSession(*session);
    
    // Enhanced readiness checking after restoration
    bool page_ready = loadPageWithReadinessCheck(browser->getCurrentUrl());
    EXPECT_TRUE(page_ready);
    
    // Allow time for complete session restoration
    std::this_thread::sleep_for(500ms);
    
    // Verify complete restoration
    std::string textValue = browser->getAttribute("#text-input", "value");
    EXPECT_EQ(textValue, "full test");
    
    std::string checkboxChecked = executeWrappedJS("return document.getElementById('checkbox2').checked;");
    EXPECT_EQ(checkboxChecked, "true");
    
    std::string selectValue = browser->getAttribute("#select-single", "value");
    EXPECT_EQ(selectValue, "option2");
    
    auto [x, y] = browser->getScrollPosition();
    EXPECT_EQ(x, 50);
    EXPECT_EQ(y, 75);
    
    std::string hash = executeWrappedJS("return window.location.hash;");
    EXPECT_EQ(hash, "#full-test");
}