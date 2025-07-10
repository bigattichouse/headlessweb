#pragma once

#include "Session.h"
#include <string>
#include <functional>

// Forward declarations for GTK and WebKit types
typedef struct _GtkWidget GtkWidget;
typedef struct _WebKitWebView WebKitWebView;
typedef struct _WebKitCookieManager WebKitCookieManager;
typedef struct _WebKitWebsiteDataManager WebKitWebsiteDataManager;

class Browser {
public:
    GtkWidget* window;
    WebKitWebView* webView;
    bool operation_completed;

    Browser();
    ~Browser();

    // Navigation
    void loadUri(const std::string& uri);
    std::string getCurrentUrl();
    std::string getPageTitle();
    void goBack();
    void goForward();
    void reload();

    // JavaScript execution
    void executeJavascript(const std::string& script, std::string* result = nullptr);
    bool waitForJavaScriptCompletion(int timeout_ms = 5000);

    // Session state management
    void restoreSession(const Session& session);
    void updateSessionState(Session& session);
    
    // Cookie management
    void getCookiesAsync(std::function<void(std::vector<Cookie>)> callback);
    void setCookie(const Cookie& cookie);
    void clearCookies();
    
    // Storage management
    std::map<std::string, std::string> getLocalStorage();
    void setLocalStorage(const std::map<std::string, std::string>& storage);
    std::map<std::string, std::string> getSessionStorage();
    void setSessionStorage(const std::map<std::string, std::string>& storage);
    
    // Form state management
    std::vector<FormField> extractFormState();
    void restoreFormState(const std::vector<FormField>& fields);
    std::set<std::string> extractActiveElements();
    void restoreActiveElements(const std::set<std::string>& elements);
    
    // Page state extraction
    std::string extractPageHash();
    std::string extractDocumentReadyState();
    std::map<std::string, std::pair<int, int>> extractAllScrollPositions();
    void restoreScrollPositions(const std::map<std::string, std::pair<int, int>>& positions);
    
    // Dynamic content waiting
    bool waitForPageReady(const Session& session);
    bool waitForSelector(const std::string& selector, int timeout_ms);
    bool waitForJsCondition(const std::string& condition, int timeout_ms);
    bool waitForText(const std::string& text, int timeout_ms);
    bool waitForElementWithContent(const std::string& selector, int timeout_ms);
    bool waitForNavigation(int timeout_ms);
    
    // Custom state extraction
    Json::Value extractCustomState(const std::map<std::string, std::string>& extractors);
    void restoreCustomState(const std::map<std::string, Json::Value>& state);
    
    // Viewport and scroll
    void setViewport(int width, int height);
    void setScrollPosition(int x, int y);
    std::pair<int, int> getScrollPosition();
    
    // User agent
    void setUserAgent(const std::string& userAgent);

    // Helper methods
    bool isOperationCompleted() const;
    std::string getInnerText(const std::string& selector);
    std::string getFirstNonEmptyText(const std::string& selector);
    
    // Form operations
    bool fillInput(const std::string& selector, const std::string& value);
    bool clickElement(const std::string& selector);
    bool submitForm(const std::string& form_selector = "form");
    bool searchForm(const std::string& query);
    bool selectOption(const std::string& selector, const std::string& value);
    bool checkElement(const std::string& selector);
    bool uncheckElement(const std::string& selector);
    bool focusElement(const std::string& selector);
    std::string getAttribute(const std::string& selector, const std::string& attribute);
    
    // Advanced operations
    bool executeActionSequence(const std::vector<Session::RecordedAction>& actions);
    void takeScreenshot(const std::string& filename);
    std::string getPageSource();
    
    // Element queries
    bool elementExists(const std::string& selector);
    int countElements(const std::string& selector);
    std::string getElementHtml(const std::string& selector);

private:
    WebKitCookieManager* cookieManager;
    WebKitWebsiteDataManager* dataManager;
    std::string sessionDataPath;
    
    void initializeDataManager(const std::string& sessionName);
    std::string executeJavascriptSync(const std::string& script);
    void wait(int milliseconds);
};
