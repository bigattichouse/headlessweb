#pragma once

#include "../Session/Session.h"
#include "../hweb/Types.h"
#include "../Debug.h"
#include "EventLoopManager.h"
#include "BrowserEventBus.h"
#include <string>
#include <functional>
#include <map>
#include <set>
#include <memory>
#include <vector>
#include <atomic>
#include <mutex>
#include <json/json.h>
#include <glib.h> // Include glib.h for GMainLoop
#include <algorithm> // For std::remove_if

// Forward declarations for GTK and WebKit types
typedef struct _GtkWidget GtkWidget;
typedef struct _WebKitWebView WebKitWebView;
typedef struct _WebKitCookieManager WebKitCookieManager;

class Browser {
    // Friend functions for static callbacks that need access to private members
    friend void navigation_complete_handler(WebKitWebView* webview, int load_event, gpointer user_data);
    friend void uri_changed_handler(WebKitWebView* webview, void* pspec, gpointer user_data);
    friend void title_changed_handler(WebKitWebView* webview, void* pspec, gpointer user_data);
    friend void ready_to_show_handler(WebKitWebView* webview, gpointer user_data);
    friend void js_eval_callback(void* object, void* res, gpointer user_data);
    friend void load_changed_callback(WebKitWebView* web_view, int load_event, gpointer user_data);
    friend void screenshot_callback(void* source_object, void* res, gpointer user_data);

private:
    HWeb::HWebConfig config_; // Store the configuration

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
        std::function<bool()> callback;  // Changed to return bool
        bool completed;
        guint timeout_id;
        std::string condition;  // For conditional signal waiting
        std::chrono::steady_clock::time_point start_time;
    };
    
    // Object lifetime and thread safety
    std::atomic<bool> is_valid;
    mutable std::mutex signal_mutex;
    std::vector<gulong> connected_signal_ids;
    
    std::vector<std::unique_ptr<EventWaiter>> active_waiters;
    std::vector<std::unique_ptr<SignalWaiter>> signal_waiters;
    
    // Navigation tracking for proper waitForNavigation behavior
    std::string previous_url;
    
    // JavaScript event helpers - BrowserEvents.cpp
    std::string setupDOMObserver(const std::string& selector, int timeout_ms);
    std::string setupVisibilityObserver(const std::string& selector, int timeout_ms);
    std::string setupNavigationObserver(int timeout_ms);
    std::string setupConditionObserver(const std::string& condition, int timeout_ms);
    
    // Signal-based waiting infrastructure
    bool waitForSignalCondition(const std::string& signal_name, const std::string& condition, int timeout_ms);
    bool waitForWebKitSignal(const std::string& signal_name, int timeout_ms);
    
    void setupSignalHandlers();
    void cleanupWaiters();

public:
    // Core members
    GtkWidget* window;
    WebKitWebView* webView;
    GMainLoop* main_loop; // For synchronous operations
    
    // Event loop manager for preventing nested loops
    std::unique_ptr<EventLoopManager> event_loop_manager;
    
    // New event-driven infrastructure
    std::shared_ptr<BrowserEvents::BrowserEventBus> event_bus_;
    std::unique_ptr<BrowserEvents::BrowserStateManager> state_manager_;
    std::unique_ptr<BrowserEvents::MutationTracker> mutation_tracker_;
    std::unique_ptr<BrowserEvents::NetworkEventTracker> network_tracker_;
    std::unique_ptr<BrowserEvents::BrowserReadinessTracker> readiness_tracker_;
    std::unique_ptr<BrowserEvents::AsyncDOMOperations> async_dom_;
    std::unique_ptr<BrowserEvents::AsyncNavigationOperations> async_nav_;
    std::unique_ptr<BrowserEvents::AsyncSessionOperations> async_session_;
    
    // Constructor/Destructor - Browser.cpp
    Browser(const HWeb::HWebConfig& config);
    ~Browser();

    // ========== Core Navigation - BrowserCore.cpp ==========
    void loadUri(const std::string& uri);
    std::string getCurrentUrl();
    std::string getPageTitle();
    void goBack();
    void goForward();
    void reload();
    bool validateUrl(const std::string& url) const;
    void setViewport(int width, int height);
    void setUserAgent(const std::string& userAgent);
    bool isFileUrl(const std::string& url) const;
    bool validateFileUrl(const std::string& url) const;

    // ========== Screenshot and Viewport ==========
    std::pair<int, int> getViewport() const;
    void ensureProperViewportForScreenshots();

    // ========== JavaScript Execution - BrowserJavaScript.cpp ==========
    [[deprecated("Use executeJavascriptSync() or executeJavascriptSyncSafe() instead for memory safety")]]
    void executeJavascript(const std::string& script, std::string* result = nullptr);
    bool waitForJavaScriptCompletion(int timeout_ms = 5000);
    std::string executeJavascriptSync(const std::string& script);
    std::string executeJavascriptSyncSafe(const std::string& script);

    // ========== Event-driven Operations - BrowserEvents.cpp ==========
    bool waitForSelectorEvent(const std::string& selector, int timeout_ms);
    bool waitForNavigationEvent(int timeout_ms);
    bool waitForNavigationSignal(int timeout_ms);
    bool waitForBackForwardNavigation(int timeout_ms);
    bool waitForVisibilityEvent(const std::string& selector, int timeout_ms);
    bool waitForConditionEvent(const std::string& js_condition, int timeout_ms);
    bool waitForPageReadyEvent(int timeout_ms);
    
    // Dynamic content waiting (wrappers)
    bool waitForPageReady(const Session& session);
    bool waitForSelector(const std::string& selector, int timeout_ms);
    bool waitForJsCondition(const std::string& condition, int timeout_ms);
    bool waitForText(const std::string& text, int timeout_ms);
    bool waitForElementWithContent(const std::string& selector, int timeout_ms);
    bool waitForNavigation(int timeout_ms);
    
    // Signal connection management
    void disconnectSignalHandlers();
    
    // PUBLIC NOTIFICATION METHODS FOR SIGNAL HANDLERS (thread-safe)
    void notifyNavigationComplete();
    void notifyUriChanged();
    void notifyTitleChanged();
    void notifyReadyToShow();
    void checkSignalConditions();
    
    // Object validity checking
    bool isObjectValid() const;
    
    // ========== Event-Driven Infrastructure Access ==========
    std::shared_ptr<BrowserEvents::BrowserEventBus> getEventBus() const { return event_bus_; }
    BrowserEvents::BrowserStateManager* getStateManager() const { return state_manager_.get(); }
    BrowserEvents::MutationTracker* getMutationTracker() const { return mutation_tracker_.get(); }
    BrowserEvents::NetworkEventTracker* getNetworkTracker() const { return network_tracker_.get(); }
    BrowserEvents::BrowserReadinessTracker* getReadinessTracker() const { return readiness_tracker_.get(); }
    BrowserEvents::AsyncDOMOperations* getAsyncDOM() const { return async_dom_.get(); }
    BrowserEvents::AsyncNavigationOperations* getAsyncNav() const { return async_nav_.get(); }
    BrowserEvents::AsyncSessionOperations* getAsyncSession() const { return async_session_.get(); }
    
    // High-level event-driven waiting methods
    std::future<bool> waitForBrowserReady(int timeout_ms = 10000);
    std::future<bool> waitForDOMReady(int timeout_ms = 10000);
    std::future<bool> waitForElementAsync(const std::string& selector, int timeout_ms = 5000);
    std::future<bool> waitForNavigationAsync(int timeout_ms = 10000);
    std::future<bool> waitForNetworkIdleAsync(int idle_time_ms = 500, int timeout_ms = 10000);
    
    // Enhanced readiness detection methods - replace blocking wait patterns
    std::future<bool> waitForPageFullyReady(int timeout_ms = 15000);
    std::future<bool> waitForPageBasicReady(int timeout_ms = 10000);
    std::future<bool> waitForPageInteractive(int timeout_ms = 5000);
    std::future<bool> waitForJavaScriptReadyAsync(int timeout_ms = 5000);
    std::future<bool> waitForResourcesLoadedAsync(int timeout_ms = 10000);
    
    // Non-blocking readiness checking
    bool isPageFullyReady() const;
    bool isPageBasicReady() const;
    bool isPageInteractive() const;

    // ========== DOM Manipulation - BrowserDOM.cpp ==========
    bool fillInput(const std::string& selector, const std::string& value);
    bool fillInputEnhanced(const std::string& selector, const std::string& value);
    bool interactWithDynamicForm(const std::string& input_selector, const std::string& value, 
                                 const std::string& submit_selector, int wait_timeout = 1000);
    bool clickElement(const std::string& selector);
    bool submitForm(const std::string& form_selector = "form");
    bool searchForm(const std::string& query);
    bool selectOption(const std::string& selector, const std::string& value);
    bool checkElement(const std::string& selector);
    bool uncheckElement(const std::string& selector);
    bool focusElement(const std::string& selector);
    std::string getAttribute(const std::string& selector, const std::string& attribute);
    bool setAttribute(const std::string& selector, const std::string& attribute, const std::string& value);
    
    // ========== Event-Driven DOM Operations - Replace blocking patterns ==========
    std::future<bool> fillInputAsync(const std::string& selector, const std::string& value, int timeout_ms = 5000);
    std::future<bool> clickElementAsync(const std::string& selector, int timeout_ms = 5000);
    std::future<bool> selectOptionAsync(const std::string& selector, const std::string& value, int timeout_ms = 5000);
    std::future<bool> submitFormAsync(const std::string& selector, int timeout_ms = 5000);
    std::future<bool> checkElementAsync(const std::string& selector, int timeout_ms = 5000);
    std::future<bool> uncheckElementAsync(const std::string& selector, int timeout_ms = 5000);
    std::future<bool> focusElementAsync(const std::string& selector, int timeout_ms = 5000);
    
    // ========== Event-Driven Navigation Operations - Replace blocking patterns ==========
    std::future<bool> waitForPageLoadCompleteAsync(const std::string& url = "", int timeout_ms = 10000);
    std::future<bool> waitForViewportReadyAsync(int timeout_ms = 5000);
    std::future<bool> waitForRenderingCompleteAsync(int timeout_ms = 5000);
    std::future<bool> waitForSPANavigationAsync(const std::string& route = "", int timeout_ms = 10000);
    std::future<bool> waitForFrameworkReadyAsync(const std::string& framework = "", int timeout_ms = 15000);
    
    // ========== Event-Driven Session Operations - Replace blocking patterns ==========
    std::future<bool> waitForUserAgentSetAsync(int timeout_ms = 2000);
    std::future<bool> waitForViewportSetAsync(int timeout_ms = 2000);
    std::future<bool> waitForCookiesRestoredAsync(int timeout_ms = 5000);
    std::future<bool> waitForStorageRestoredAsync(const std::string& storage_type, int timeout_ms = 5000);
    std::future<bool> waitForFormStateRestoredAsync(int timeout_ms = 10000);
    std::future<bool> waitForActiveElementsRestoredAsync(int timeout_ms = 2000);
    std::future<bool> waitForCustomAttributesRestoredAsync(int timeout_ms = 5000);
    std::future<bool> waitForCustomStateRestoredAsync(int timeout_ms = 5000);
    std::future<bool> waitForScrollPositionsRestoredAsync(int timeout_ms = 3000);
    std::future<bool> waitForSessionRestorationCompleteAsync(int timeout_ms = 30000);
    std::future<bool> restoreSessionAsync(const Session& session, int timeout_ms = 30000);
    
    // Element queries
    bool elementExists(const std::string& selector);
    // Returns: 0 = doesn't exist, 1 = exists, -1 = invalid selector
    int elementExistsWithValidation(const std::string& selector);
    int countElements(const std::string& selector);
    std::string getElementHtml(const std::string& selector);
    std::string getInnerText(const std::string& selector);
    std::string getFirstNonEmptyText(const std::string& selector);

    // ========== Storage Management - BrowserStorage.cpp ==========
    void getCookiesAsync(std::function<void(std::vector<Cookie>)> callback);
    void setCookie(const Cookie& cookie);
    void setCookieSafe(const Cookie& cookie);
    void clearCookies();
    
    std::map<std::string, std::string> getLocalStorage();
    void setLocalStorage(const std::map<std::string, std::string>& storage);
    std::map<std::string, std::string> getSessionStorage();
    void setSessionStorage(const std::map<std::string, std::string>& storage);
    
    void clearLocalStorage();
    void clearSessionStorage();
    void clearAllStorage();

    // ========== Session Management - BrowserSession.cpp ==========
    void restoreSession(const Session& session);
    void updateSessionState(Session& session);
    bool restoreSessionSafely(const Session& session);
    void waitForPageStabilization(int timeout_ms = 2000);
    bool validateSession(const Session& session) const;
    
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
    
    // Custom state
    Json::Value extractCustomState(const std::map<std::string, std::string>& extractors);
    void restoreCustomState(const std::map<std::string, Json::Value>& state);
    
    // Custom attributes management
    std::string extractCustomAttributesScript();
    void restoreCustomAttributesFromState(const Json::Value& attributesState);

    // ========== Screenshot Operations - BrowserScreenshot.cpp ==========
    void takeScreenshot(const std::string& filename);
    void takeFullPageScreenshot(const std::string& filename);

    // ========== Utility Functions - BrowserUtilities.cpp ==========
    void wait(int milliseconds); // Blocking wait
    bool isPageLoaded() const;
    std::string getPageLoadState() const;
    std::string getPageSource();
    bool executeActionSequence(const std::vector<Session::RecordedAction>& actions);
    void setScrollPosition(int x, int y);
    std::pair<int, int> getScrollPosition();
    void initializeDataManager(const std::string& sessionName);


    // ========== Advanced Waiting Methods ==========
    bool waitForTextAdvanced(const std::string& text, int timeout_ms, bool case_sensitive = false, bool exact_match = false);
    bool waitForNetworkIdle(int idle_time_ms, int timeout_ms);
    bool waitForNetworkRequest(const std::string& url_pattern, int timeout_ms);
    bool waitForElementVisible(const std::string& selector, int timeout_ms);
    bool waitForElementCount(const std::string& selector, const std::string& operator_str, int expected_count, int timeout_ms);
    bool waitForAttribute(const std::string& selector, const std::string& attribute, const std::string& expected_value, int timeout_ms);
    bool waitForUrlChange(const std::string& pattern, int timeout_ms);
    bool waitForTitleChange(const std::string& pattern, int timeout_ms);
    bool waitForSPANavigation(const std::string& route, int timeout_ms);
    bool waitForFrameworkReady(const std::string& framework, int timeout_ms);
    bool waitForDOMChange(const std::string& selector, int timeout_ms);
    bool waitForContentChange(const std::string& selector, const std::string& property, int timeout_ms);

protected:
    WebKitCookieManager* cookieManager;
    std::string sessionDataPath;
    // js_result_buffer removed - using local buffers for thread safety
    
    // Event-driven infrastructure already declared above in private section
    // Removed duplicate declarations to fix compilation errors
};
