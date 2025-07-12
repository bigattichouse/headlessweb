#pragma once

#include "Session.h"
#include "Debug.h"
#include <string>
#include <functional>
#include <map>
#include <set>
#include <memory>
#include <vector>
#include <json/json.h>
#include <glib.h> // Include glib.h for GMainLoop
#include <algorithm> // For std::remove_if

// Forward declarations for GTK and WebKit types
typedef struct _GtkWidget GtkWidget;
typedef struct _WebKitWebView WebKitWebView;
typedef struct _WebKitCookieManager WebKitCookieManager;

class Browser {
private:
    // Event management structures
    struct EventWaiter {
        std::string event_type;
        std::string condition;
        std::function<void(bool)> callback;
        guint timeout_id;
        bool completed;
    };
    
    struct SignalWaiter {
        gulong signal_id;
        std::string signal_name;
        std::function<void()> callback;
        bool completed;
        guint timeout_id;
    };
    
    std::vector<std::unique_ptr<EventWaiter>> active_waiters;
    std::vector<std::unique_ptr<SignalWaiter>> signal_waiters;
    
    // JavaScript event helpers
    std::string setupDOMObserver(const std::string& selector, int timeout_ms);
    std::string setupVisibilityObserver(const std::string& selector, int timeout_ms);
    std::string setupNavigationObserver(int timeout_ms);
    std::string setupConditionObserver(const std::string& condition, int timeout_ms);
    
    void setupSignalHandlers();
    void cleanupWaiters();

public:
    GtkWidget* window;
    WebKitWebView* webView;
    Browser();
    ~Browser();

    // Navigation
    void loadUri(const std::string& uri);
    std::string getCurrentUrl();
    std::string getPageTitle();
    void goBack();
    void goForward();
    void reload();
    
    // URL validation
    bool validateUrl(const std::string& url) const;

    // JavaScript execution
    void executeJavascript(const std::string& script, std::string* result = nullptr);
    bool waitForJavaScriptCompletion(int timeout_ms = 5000);

    // PUBLIC NOTIFICATION METHODS FOR SIGNAL HANDLERS
    void notifyNavigationComplete();
    void notifyUriChanged();
    void notifyTitleChanged();
    void notifyReadyToShow();

    // Session state management
    void restoreSession(const Session& session);
    void updateSessionState(Session& session);
    
    // Enhanced session operations
    bool restoreSessionSafely(const Session& session);
    void waitForPageStabilization(int timeout_ms = 2000);
    
    // Validation and health checks
    bool isPageLoaded() const;
    bool validateSession(const Session& session) const;
    std::string getPageLoadState() const;
    
    // Cookie management
    void getCookiesAsync(std::function<void(std::vector<Cookie>)> callback);
    void setCookie(const Cookie& cookie);
    void setCookieSafe(const Cookie& cookie);
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
    
    // Event-driven waiting methods (NEW)
    bool waitForSelectorEvent(const std::string& selector, int timeout_ms);
    bool waitForNavigationEvent(int timeout_ms);
    bool waitForNavigationSignal(int timeout_ms);
    bool waitForVisibilityEvent(const std::string& selector, int timeout_ms);
    bool waitForConditionEvent(const std::string& js_condition, int timeout_ms);
    bool waitForPageReadyEvent(int timeout_ms);
    
    // Dynamic content waiting (UPDATED to use events)
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
    std::string getInnerText(const std::string& selector);
    std::string getFirstNonEmptyText(const std::string& selector);
    
    // Form operations (ENHANCED with event waiting)
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
    void takeFullPageScreenshot(const std::string& filename);
    std::string getPageSource();
    
    // Element queries
    bool elementExists(const std::string& selector);
    int countElements(const std::string& selector);
    std::string getElementHtml(const std::string& selector);

    // Enhanced JavaScript execution with error handling
    std::string executeJavascriptSyncSafe(const std::string& script);
    
    // File URL validation and handling
    bool isFileUrl(const std::string& url) const;
    bool validateFileUrl(const std::string& url) const;

    void wait(int milliseconds); // Blocking wait

protected:
    WebKitCookieManager* cookieManager;
    std::string sessionDataPath;
    std::string js_result_buffer;  // Buffer for JavaScript results
public:
    GMainLoop* main_loop; // For synchronous operations
    
    void initializeDataManager(const std::string& sessionName);
    std::string executeJavascriptSync(const std::string& script);
};
