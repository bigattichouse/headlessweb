#include "Browser.h"
#include <gtk/gtk.h>
#include <webkit/webkit.h>
#include <glib.h>
#include <iostream>

// External debug flag
extern bool g_debug;

// ========== Signal Handlers ==========

void navigation_complete_handler(WebKitWebView* webview, WebKitLoadEvent load_event, gpointer user_data) {
    Browser* browser = static_cast<Browser*>(user_data);
    
    if (load_event == WEBKIT_LOAD_FINISHED) {
        debug_output("Navigation completed via signal");
        
        // Use public interface to notify waiters
        browser->notifyNavigationComplete();
        
        // General completion signal
        if (g_main_loop_is_running(browser->main_loop)) {
            g_main_loop_quit(browser->main_loop);
        }
    }
}

void uri_changed_handler(WebKitWebView* webview, GParamSpec* pspec, gpointer user_data) {
    Browser* browser = static_cast<Browser*>(user_data);
    const gchar* new_uri = webkit_web_view_get_uri(webview);
    
    debug_output("URI changed to: " + std::string(new_uri ? new_uri : ""));
    
    // Use public interface to notify waiters
    browser->notifyUriChanged();
}

void title_changed_handler(WebKitWebView* webview, GParamSpec* pspec, gpointer user_data) {
    Browser* browser = static_cast<Browser*>(user_data);
    const gchar* new_title = webkit_web_view_get_title(webview);
    
    debug_output("Title changed to: " + std::string(new_title ? new_title : ""));
    
    // Use public interface to notify waiters
    browser->notifyTitleChanged();
}

void ready_to_show_handler(WebKitWebView* webview, gpointer user_data) {
    Browser* browser = static_cast<Browser*>(user_data);
    
    debug_output("Page ready to show");
    
    // Use public interface to notify waiters
    browser->notifyReadyToShow();
}

// Callback for load-changed signal
void load_changed_callback(WebKitWebView* web_view, WebKitLoadEvent load_event, gpointer user_data) {
    if (load_event == WEBKIT_LOAD_FINISHED) {
        Browser* browser_instance = static_cast<Browser*>(user_data);
        if (browser_instance) {
            if (g_main_loop_is_running(browser_instance->main_loop)) {
                g_main_loop_quit(browser_instance->main_loop);
            }
        }
    }
}

// ========== Setup and Cleanup Methods ==========

void Browser::setupSignalHandlers() {
    // Navigation events
    g_signal_connect(webView, "load-changed", 
                     G_CALLBACK(navigation_complete_handler), this);
    
    // URI changes (including SPA navigation)
    g_signal_connect(webView, "notify::uri", 
                     G_CALLBACK(uri_changed_handler), this);
    
    // Title changes (often indicates page updates)
    g_signal_connect(webView, "notify::title", 
                     G_CALLBACK(title_changed_handler), this);
    
    // Page ready signal
    g_signal_connect(webView, "ready-to-show", 
                     G_CALLBACK(ready_to_show_handler), this);
}

void Browser::cleanupWaiters() {
    // Clean up active waiters
    for (auto& waiter : active_waiters) {
        if (waiter->timeout_id != 0) {
            g_source_remove(waiter->timeout_id);
        }
    }
    active_waiters.clear();
    
    // Clean up signal waiters
    for (auto& waiter : signal_waiters) {
        if (waiter->timeout_id != 0) {
            g_source_remove(waiter->timeout_id);
        }
    }
    signal_waiters.clear();
}

// ========== Public Notification Methods ==========

void Browser::notifyNavigationComplete() {
    for (auto& waiter : signal_waiters) {
        if (waiter->signal_name == "navigation" && !waiter->completed) {
            waiter->completed = true;
            if (waiter->callback) {
                waiter->callback();
            }
        }
    }
}

void Browser::notifyUriChanged() {
    for (auto& waiter : signal_waiters) {
        if (waiter->signal_name == "uri-change" && !waiter->completed) {
            waiter->completed = true;
            if (waiter->callback) {
                waiter->callback();
            }
        }
    }
}

void Browser::notifyTitleChanged() {
    for (auto& waiter : signal_waiters) {
        if (waiter->signal_name == "title-change" && !waiter->completed) {
            waiter->completed = true;
            if (waiter->callback) {
                waiter->callback();
            }
        }
    }
}

void Browser::notifyReadyToShow() {
    for (auto& waiter : signal_waiters) {
        if (waiter->signal_name == "ready-to-show" && !waiter->completed) {
            waiter->completed = true;
            if (waiter->callback) {
                waiter->callback();
            }
        }
    }
}

// ========== JavaScript Observer Setup Methods ==========

std::string Browser::setupDOMObserver(const std::string& selector, int timeout_ms) {
    return R"(
        (function(selector, timeout) {
            window._hweb_event_result = undefined;
            
            // Check if element already exists
            const existing = document.querySelector(selector);
            if (existing) {
                window._hweb_event_result = true;
                return;
            }
            
            // Set up mutation observer
            const observer = new MutationObserver((mutations) => {
                const element = document.querySelector(selector);
                if (element) {
                    observer.disconnect();
                    window._hweb_event_result = true;
                }
            });
            
            // Observe with comprehensive options
            observer.observe(document.documentElement, {
                childList: true,
                subtree: true,
                attributes: true
            });
            
            // Timeout fallback
            setTimeout(() => {
                observer.disconnect();
                if (window._hweb_event_result === undefined) {
                    window._hweb_event_result = false;
                }
            }, timeout);
            
        })(')" + selector + "', " + std::to_string(timeout_ms) + R"();
    )";
}

std::string Browser::setupVisibilityObserver(const std::string& selector, int timeout_ms) {
    return R"(
        (function(selector, timeout) {
            window._hweb_event_result = undefined;
            
            const element = document.querySelector(selector);
            if (!element) {
                window._hweb_event_result = false;
                return;
            }
            
            // Check if already visible
            const rect = element.getBoundingClientRect();
            if (rect.width > 0 && rect.height > 0) {
                window._hweb_event_result = true;
                return;
            }
            
            // Simple polling for visibility
            let attempts = 0;
            const maxAttempts = timeout / 100;
            
            const checkVisibility = () => {
                attempts++;
                const rect = element.getBoundingClientRect();
                if (rect.width > 0 && rect.height > 0) {
                    window._hweb_event_result = true;
                } else if (attempts >= maxAttempts) {
                    window._hweb_event_result = false;
                } else {
                    setTimeout(checkVisibility, 100);
                }
            };
            
            setTimeout(checkVisibility, 100);
            
        })(')" + selector + "', " + std::to_string(timeout_ms) + R"();
    )";
}

std::string Browser::setupNavigationObserver(int timeout_ms) {
    return R"(
        (function(timeout, initialUrl) {
            window._hweb_event_result = undefined;
            
            // Check for URL changes
            const checkNavigation = () => {
                if (window.location.href !== initialUrl) {
                    window._hweb_event_result = true;
                    return true;
                }
                return false;
            };
            
            // Check immediately
            if (checkNavigation()) return;
            
            // Simple polling approach
            let attempts = 0;
            const maxAttempts = timeout / 500;
            
            const poll = () => {
                attempts++;
                if (checkNavigation()) {
                    // Navigation detected
                } else if (attempts >= maxAttempts) {
                    window._hweb_event_result = false;
                } else {
                    setTimeout(poll, 500);
                }
            };
            
            // Start polling
            setTimeout(poll, 500);
            
        })()" + std::to_string(timeout_ms) + ", '" + getCurrentUrl() + R"(');
    )";
}

std::string Browser::setupConditionObserver(const std::string& condition, int timeout_ms) {
    // Escape quotes in the condition string
    std::string escaped_condition = condition;
    size_t pos = 0;
    while ((pos = escaped_condition.find("'", pos)) != std::string::npos) {
        escaped_condition.replace(pos, 1, "\\'");
        pos += 2;
    }
    
    return R"(
        (function(condition, timeout) {
            window._hweb_event_result = undefined;
            
            const checkCondition = () => {
                try {
                    return eval(condition);
                } catch(e) {
                    return false;
                }
            };
            
            // Check immediately
            if (checkCondition()) {
                window._hweb_event_result = true;
                return;
            }
            
            // Simple polling approach
            let attempts = 0;
            const maxAttempts = timeout / 100;
            
            const poll = () => {
                attempts++;
                try {
                    if (checkCondition()) {
                        window._hweb_event_result = true;
                    } else if (attempts >= maxAttempts) {
                        window._hweb_event_result = false;
                    } else {
                        setTimeout(poll, 100);
                    }
                } catch(e) {
                    if (attempts >= maxAttempts) {
                        window._hweb_event_result = false;
                    } else {
                        setTimeout(poll, 100);
                    }
                }
            };
            
            setTimeout(poll, 100);
            
        })(')" + escaped_condition + "', " + std::to_string(timeout_ms) + R"();
    )";
}

// ========== Event-driven Waiting Implementations ==========

bool Browser::waitForSelectorEvent(const std::string& selector, int timeout_ms) {
    // Set up the JavaScript observer
    std::string observerScript = setupDOMObserver(selector, timeout_ms);
    
    // Clear any previous result
    executeJavascriptSync("window._hweb_event_result = undefined;");
    
    // Start the observer - note this no longer returns a promise
    executeJavascript(observerScript);
    
    // Wait for the observer to complete with more frequent checking
    int elapsed = 0;
    const int check_interval = 100; // Check every 100ms
    
    while (elapsed < timeout_ms) {
        wait(check_interval);
        elapsed += check_interval;
        
        // Check if the observer has set the result
        std::string result = executeJavascriptSync("typeof window._hweb_event_result !== 'undefined' ? String(window._hweb_event_result) : 'undefined'");
        
        if (result == "true") {
            return true;
        } else if (result == "false") {
            return false;
        }
        // If result is "undefined", continue waiting
    }
    
    return false;
}

bool Browser::waitForNavigationEvent(int timeout_ms) {
    std::string observerScript = setupNavigationObserver(timeout_ms);
    
    executeJavascriptSync("window._hweb_event_result = undefined;");
    executeJavascript(observerScript);
    
    int elapsed = 0;
    const int check_interval = 200; // Longer interval for navigation
    
    while (elapsed < timeout_ms) {
        wait(check_interval);
        elapsed += check_interval;
        
        std::string result = executeJavascriptSync("typeof window._hweb_event_result !== 'undefined' ? String(window._hweb_event_result) : 'undefined'");
        
        if (result == "true") {
            return true;
        } else if (result == "false") {
            return false;
        }
        // If result is "undefined", continue waiting
    }
    
    return false;
}

bool Browser::waitForNavigationSignal(int timeout_ms) {
    auto waiter = std::make_unique<SignalWaiter>();
    waiter->signal_name = "navigation";
    waiter->completed = false;
    waiter->timeout_id = 0;
    
    // Set up timeout
    waiter->timeout_id = g_timeout_add(timeout_ms, [](gpointer user_data) -> gboolean {
        SignalWaiter* waiter = static_cast<SignalWaiter*>(user_data);
        if (!waiter->completed) {
            waiter->completed = true;
        }
        return G_SOURCE_REMOVE;
    }, waiter.get());
    
    // Add to active waiters
    signal_waiters.push_back(std::move(waiter));
    
    // Wait for signal or timeout
    g_main_loop_run(main_loop);
    
    // Check result and cleanup
    bool success = false;
    signal_waiters.erase(
        std::remove_if(signal_waiters.begin(), signal_waiters.end(),
            [&success](const std::unique_ptr<SignalWaiter>& w) {
                if (w->signal_name == "navigation") {
                    success = w->completed;
                    if (w->timeout_id != 0) {
                        g_source_remove(w->timeout_id);
                    }
                    return true;
                }
                return false;
            }),
        signal_waiters.end()
    );
    
    return success;
}

bool Browser::waitForBackForwardNavigation(int timeout_ms) {
    // For back/forward navigation, URL change is more reliable than load events
    std::string initial_url = getCurrentUrl();
    debug_output("Waiting for back/forward navigation from: " + initial_url);
    
    const int check_interval = 50; // Faster checking for back/forward
    int elapsed = 0;
    
    while (elapsed < timeout_ms) {
        // Process any pending GTK events
        while (g_main_context_pending(g_main_context_default())) {
            g_main_context_iteration(g_main_context_default(), FALSE);
        }
        
        std::string current_url = getCurrentUrl();
        if (current_url != initial_url && !current_url.empty()) {
            debug_output("Back/forward navigation detected: " + current_url);
            return true;
        }
        
        wait(check_interval);
        elapsed += check_interval;
    }
    
    debug_output("Back/forward navigation timeout");
    return false;
}

bool Browser::waitForVisibilityEvent(const std::string& selector, int timeout_ms) {
    std::string observerScript = setupVisibilityObserver(selector, timeout_ms);
    
    executeJavascriptSync("window._hweb_event_result = undefined;");
    executeJavascript(observerScript);
    
    int elapsed = 0;
    const int check_interval = 100;
    
    while (elapsed < timeout_ms) {
        wait(check_interval);
        elapsed += check_interval;
        
        std::string result = executeJavascriptSync("typeof window._hweb_event_result !== 'undefined' ? String(window._hweb_event_result) : 'undefined'");
        
        if (result == "true") {
            return true;
        } else if (result == "false") {
            return false;
        }
        // If result is "undefined", continue waiting
    }
    
    return false;
}

bool Browser::waitForConditionEvent(const std::string& js_condition, int timeout_ms) {
    std::string observerScript = setupConditionObserver(js_condition, timeout_ms);
    
    executeJavascriptSync("window._hweb_event_result = undefined;");
    executeJavascript(observerScript);
    
    int elapsed = 0;
    const int check_interval = 100;
    
    while (elapsed < timeout_ms) {
        wait(check_interval);
        elapsed += check_interval;
        
        std::string result = executeJavascriptSync("typeof window._hweb_event_result !== 'undefined' ? String(window._hweb_event_result) : 'undefined'");
        
        if (result == "true") {
            return true;
        } else if (result == "false") {
            return false;
        }
        // If result is "undefined", continue waiting
    }
    
    return false;
}

bool Browser::waitForPageReadyEvent(int timeout_ms) {
    // Wait for document ready state first with simpler condition
    bool document_ready = waitForConditionEvent("document.readyState === 'complete'", timeout_ms / 2);
    
    if (!document_ready) {
        // Fallback: wait for interactive state
        document_ready = waitForConditionEvent("document.readyState === 'interactive'", timeout_ms / 4);
    }
    
    if (!document_ready) {
        // Last resort: just wait a bit
        wait(500);
        return false;
    }
    
    // Then do a simple check for basic page elements
    bool basic_ready = waitForConditionEvent("document.body !== null", timeout_ms / 4);
    
    return document_ready && basic_ready;
}

// ========== Public Wrapper Methods ==========

bool Browser::waitForSelector(const std::string& selector, int timeout_ms) {
    return waitForSelectorEvent(selector, timeout_ms);
}

bool Browser::waitForNavigation(int timeout_ms) {
    return waitForNavigationEvent(timeout_ms);
}

bool Browser::waitForJsCondition(const std::string& condition, int timeout_ms) {
    return waitForConditionEvent(condition, timeout_ms);
}

bool Browser::waitForText(const std::string& text, int timeout_ms) {
    std::string escaped_text = text;
    // Escape quotes and other special characters
    size_t pos = 0;
    while ((pos = escaped_text.find("'", pos)) != std::string::npos) {
        escaped_text.replace(pos, 1, "\\'");
        pos += 2;
    }
    
    std::string condition = "document.body && document.body.innerText.includes('" + escaped_text + "')";
    return waitForConditionEvent(condition, timeout_ms);
}

void Browser::waitForPageStabilization(int timeout_ms) {
    if (!waitForPageReadyEvent(timeout_ms)) {
        // Fallback to shorter wait
        wait(500);
    }
}

bool Browser::waitForPageReady(const Session& session) {
    // Implementation needed - placeholder for now
    return waitForPageReadyEvent(5000);
}

bool Browser::waitForElementWithContent(const std::string& selector, int timeout_ms) {
    // Wait for element to exist first
    if (!waitForSelectorEvent(selector, timeout_ms / 2)) {
        return false;
    }
    
    // Then wait for it to have content
    std::string condition = "document.querySelector('" + selector + "') && "
                           "document.querySelector('" + selector + "').innerText.trim().length > 0";
    
    return waitForConditionEvent(condition, timeout_ms / 2);
}
