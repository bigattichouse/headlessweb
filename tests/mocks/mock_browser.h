#pragma once

#include <gmock/gmock.h>
#include <string>

// Create a test-friendly Browser class that inherits from the real Browser
// but allows for mocking specific methods during tests

// We need to include the real Browser header but in a test-safe way
#ifndef TESTING_MODE
#include "../../src/Browser/Browser.h"
#else

// In testing mode, create a minimal Browser interface that can be mocked
class Browser {
public:
    // GTK/WebKit members that we'll mock away
    void* window = nullptr;
    void* webView = nullptr;
    void* main_loop = nullptr;
    void* cookieManager = nullptr;
    std::string sessionDataPath;
    std::string js_result_buffer;

    Browser() = default;
    virtual ~Browser() = default;
    
    // Virtual methods that can be mocked
    virtual bool elementExists(const std::string& selector) { return false; }
    virtual std::string getInnerText(const std::string& selector) { return ""; }
    virtual int countElements(const std::string& selector) { return 0; }
    virtual std::string executeJavascriptSync(const std::string& script) { return ""; }
    virtual std::string getAttribute(const std::string& selector, const std::string& attribute) { return ""; }
    virtual bool clickElement(const std::string& selector) { return false; }
    virtual bool fillInput(const std::string& selector, const std::string& text) { return false; }
    virtual void loadUri(const std::string& url) {}
    virtual std::string getCurrentUrl() { return ""; }
    virtual bool waitForSelector(const std::string& selector, int timeout_ms) { return false; }
    virtual std::string getPageTitle() { return ""; }
    virtual void executeJavascript(const std::string& script, std::string* result = nullptr) {}
};

#endif

// Mock browser implementation that inherits from Browser
class MockBrowser : public Browser {
public:
    MockBrowser() = default;
    virtual ~MockBrowser() = default;
    
    // Mock methods that override Browser methods
    MOCK_METHOD(bool, elementExists, (const std::string& selector), (override));
    MOCK_METHOD(std::string, getInnerText, (const std::string& selector), (override));
    MOCK_METHOD(int, countElements, (const std::string& selector), (override));
    MOCK_METHOD(std::string, executeJavascriptSync, (const std::string& script), (override));
    MOCK_METHOD(std::string, getAttribute, (const std::string& selector, const std::string& attribute), (override));
    MOCK_METHOD(bool, clickElement, (const std::string& selector), (override));
    MOCK_METHOD(bool, fillInput, (const std::string& selector, const std::string& text), (override));
    MOCK_METHOD(void, loadUri, (const std::string& url), (override));
    MOCK_METHOD(std::string, getCurrentUrl, (), (override));
    MOCK_METHOD(bool, waitForSelector, (const std::string& selector, int timeout_ms), (override));
    MOCK_METHOD(std::string, getPageTitle, (), (override));
    MOCK_METHOD(void, executeJavascript, (const std::string& script, std::string* result), (override));
};