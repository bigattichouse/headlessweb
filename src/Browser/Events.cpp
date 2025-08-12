#include "Browser.h"
#include <gtk/gtk.h>
#include <webkit/webkit.h>
#include <glib.h>
#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>
#include <cstring>  // For strnlen
#include <ctime>

// External debug flag
extern bool g_debug;

// ========== Signal Handlers ==========

void navigation_complete_handler(WebKitWebView* webview, WebKitLoadEvent load_event, gpointer user_data) {
    // CRITICAL SAFETY: Validate all pointers before any operations
    if (!webview || !user_data) {
        return;
    }
    
    Browser* browser = static_cast<Browser*>(user_data);
    
    // CRITICAL FIX: Check if browser object is still valid
    if (!browser || !browser->isObjectValid()) {
        return;
    }
    
    // SAFETY: Use safer string construction to prevent stack smashing
    const gchar* uri = webkit_web_view_get_uri(webview);
    std::string current_url;
    if (uri && strnlen(uri, 2048) < 2048) {  // Prevent buffer overflow
        current_url = std::string(uri);
    }
    
    auto event_bus = browser->getEventBus();
    auto state_manager = browser->getStateManager();
    auto readiness_tracker = browser->getReadinessTracker();
    
    switch (load_event) {
        case WEBKIT_LOAD_STARTED:
            debug_output("Navigation started");
            if (event_bus) {
                event_bus->emit(BrowserEvents::NavigationEvent(BrowserEvents::EventType::NAVIGATION_STARTED, current_url));
                // Emit page load started event
                event_bus->emit(BrowserEvents::PageLoadEvent(BrowserEvents::EventType::PAGE_LOAD_STARTED, current_url, 0.0, "started", false));
            }
            if (state_manager) {
                state_manager->transitionToState(BrowserEvents::BrowserState::LOADING);
            }
            // Trigger navigation monitoring JavaScript
            if (auto async_nav = browser->getAsyncNav()) {
                std::string monitor_script = async_nav->generatePageLoadMonitorScript();
                browser->executeJavascriptSync(monitor_script);
            }
            break;
        case WEBKIT_LOAD_REDIRECTED:
            debug_output("Navigation redirected");
            break;
        case WEBKIT_LOAD_COMMITTED:
            debug_output("Navigation committed (DOM available)");
            if (event_bus) {
                event_bus->emit(BrowserEvents::EventType::DOM_CONTENT_LOADED, current_url);
                // Emit page load progress event
                event_bus->emit(BrowserEvents::PageLoadEvent(BrowserEvents::EventType::PAGE_LOAD_PROGRESS, current_url, 0.5, "progress", false));
            }
            if (state_manager) {
                state_manager->transitionToState(BrowserEvents::BrowserState::DOM_LOADING);
            }
            if (readiness_tracker) {
                readiness_tracker->updateDOMReady();
            }
            // Initialize SPA navigation and framework detection scripts
            if (auto async_nav = browser->getAsyncNav()) {
                browser->executeJavascriptSync(async_nav->generateSPANavigationDetectionScript());
                browser->executeJavascriptSync(async_nav->generateFrameworkDetectionScript(""));
                browser->executeJavascriptSync(async_nav->generateRenderingCompleteScript());
            }
            // Check signal conditions on DOM commit
            browser->checkSignalConditions();
            break;
        case WEBKIT_LOAD_FINISHED:
            debug_output("Navigation finished");
            if (event_bus) {
                event_bus->emit(BrowserEvents::NavigationEvent(BrowserEvents::EventType::NAVIGATION_COMPLETED, current_url, "", true));
                // Emit page load complete event
                event_bus->emit(BrowserEvents::PageLoadEvent(BrowserEvents::EventType::PAGE_LOAD_COMPLETE, current_url, 1.0, "complete", false));
                // Emit viewport ready event
                event_bus->emit(BrowserEvents::EventType::VIEWPORT_READY, current_url);
            }
            if (state_manager) {
                state_manager->transitionToState(BrowserEvents::BrowserState::FULLY_READY);
            }
            if (readiness_tracker) {
                readiness_tracker->updateResourcesLoaded();
                
                // Trigger comprehensive readiness check with error handling
                try {
                    std::string readiness_script = readiness_tracker->generateReadinessCheckScript();
                    std::string result = browser->executeJavascriptSync(readiness_script);
                    
                    // Update readiness state based on JavaScript results
                    if (!result.empty() && result != "null" && result != "undefined") {
                        if (result.find("\"javascript_ready\":true") != std::string::npos) {
                            readiness_tracker->updateJavaScriptReady();
                        }
                        if (result.find("\"fonts_loaded\":true") != std::string::npos) {
                            readiness_tracker->updateFontsLoaded();
                        }
                        if (result.find("\"images_loaded\":true") != std::string::npos) {
                            readiness_tracker->updateImagesLoaded();
                        }
                        if (result.find("\"styles_applied\":true") != std::string::npos) {
                            readiness_tracker->updateStylesApplied();
                        }
                    } else {
                        // Fallback: assume basic readiness after load finished
                        readiness_tracker->updateJavaScriptReady();
                        readiness_tracker->updateStylesApplied();
                    }
                } catch (const std::exception& e) {
                    // Fallback: assume basic readiness if JavaScript check fails
                    debug_output("Readiness JavaScript check failed, using fallback");
                    readiness_tracker->updateJavaScriptReady();
                    readiness_tracker->updateStylesApplied();
                }
            }
            // Use public interface to notify waiters
            browser->notifyNavigationComplete();
            // Check signal conditions on load complete
            browser->checkSignalConditions();
            break;
    }
}

void uri_changed_handler(WebKitWebView* webview, GParamSpec* pspec, gpointer user_data) {
    // CRITICAL SAFETY: Validate all pointers before any operations  
    if (!webview || !user_data) {
        return;
    }
    
    Browser* browser = static_cast<Browser*>(user_data);
    
    // CRITICAL FIX: Check if browser object is still valid
    if (!browser || !browser->isObjectValid()) {
        return;
    }
    
    // SAFETY: Use safer string construction to prevent stack smashing
    const gchar* new_uri = webkit_web_view_get_uri(webview);
    std::string current_url;
    if (new_uri && strnlen(new_uri, 2048) < 2048) {  // Prevent buffer overflow
        current_url = std::string(new_uri);
    }
    
    debug_output("URI changed to: " + current_url);
    
    // Emit event through new event bus
    auto event_bus = browser->getEventBus();
    if (event_bus) {
        event_bus->emit(BrowserEvents::EventType::URL_CHANGED, current_url);
    }
    
    // Use public interface to notify waiters
    browser->notifyUriChanged();
    // Check signal conditions on URI change
    browser->checkSignalConditions();
}

void title_changed_handler(WebKitWebView* webview, GParamSpec* pspec, gpointer user_data) {
    // CRITICAL SAFETY: Validate all pointers before any operations
    if (!webview || !user_data) {
        return;
    }
    
    Browser* browser = static_cast<Browser*>(user_data);
    
    // CRITICAL FIX: Check if browser object is still valid
    if (!browser || !browser->isObjectValid()) {
        return;
    }
    
    // SAFETY: Use safer string construction to prevent stack smashing
    const gchar* new_title = webkit_web_view_get_title(webview);
    std::string title_str;
    if (new_title && strnlen(new_title, 1024) < 1024) {  // Prevent buffer overflow
        title_str = std::string(new_title);
    }
    
    debug_output("Title changed to: " + title_str);
    
    // Use public interface to notify waiters
    browser->notifyTitleChanged();
}

void ready_to_show_handler(WebKitWebView* webview, gpointer user_data) {
    // CRITICAL SAFETY: Validate all pointers before any operations
    if (!webview || !user_data) {
        return;
    }
    
    Browser* browser = static_cast<Browser*>(user_data);
    
    // CRITICAL FIX: Check if browser object is still valid
    if (!browser || !browser->isObjectValid()) {
        return;
    }
    
    debug_output("Page ready to show");
    
    // Use public interface to notify waiters
    browser->notifyReadyToShow();
    // Check signal conditions when page is ready
    browser->checkSignalConditions();
}

// Callback for load-changed signal
void load_changed_callback(WebKitWebView* web_view, WebKitLoadEvent load_event, gpointer user_data) {
    
}


// ========== Setup and Cleanup Methods ==========

void Browser::setupSignalHandlers() {
    std::lock_guard<std::mutex> lock(signal_mutex);
    
    // Clear any existing signal IDs
    connected_signal_ids.clear();
    
    // Navigation events
    gulong nav_id = g_signal_connect(webView, "load-changed", 
                                    G_CALLBACK(navigation_complete_handler), this);
    connected_signal_ids.push_back(nav_id);
    
    // URI changes (including SPA navigation)
    gulong uri_id = g_signal_connect(webView, "notify::uri", 
                                    G_CALLBACK(uri_changed_handler), this);
    connected_signal_ids.push_back(uri_id);
    
    // Title changes (often indicates page updates)
    gulong title_id = g_signal_connect(webView, "notify::title", 
                                      G_CALLBACK(title_changed_handler), this);
    connected_signal_ids.push_back(title_id);
    
    // Note: document-loaded is not a valid WebKit signal, 
    // ready-to-show and load-changed with WEBKIT_LOAD_FINISHED are sufficient
    
    // Page ready signal
    gulong ready_id = g_signal_connect(webView, "ready-to-show", 
                                      G_CALLBACK(ready_to_show_handler), this);
    connected_signal_ids.push_back(ready_id);
    
    debug_output("Connected " + std::to_string(connected_signal_ids.size()) + " signal handlers");
}

void Browser::disconnectSignalHandlers() {
    std::lock_guard<std::mutex> lock(signal_mutex);
    
    if (!webView) {
        return;
    }
    
    // Disconnect all stored signal handlers
    for (gulong signal_id : connected_signal_ids) {
        if (g_signal_handler_is_connected(webView, signal_id)) {
            g_signal_handler_disconnect(webView, signal_id);
        }
    }
    
    connected_signal_ids.clear();
}

void Browser::cleanupWaiters() {
    std::lock_guard<std::mutex> lock(signal_mutex);
    
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

bool Browser::isObjectValid() const {
    return is_valid.load();
}

// ========== Public Notification Methods ==========

void Browser::notifyNavigationComplete() {
    // Check if object is still valid before accessing members
    if (!is_valid.load()) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(signal_mutex);
    
    for (auto& waiter : signal_waiters) {
        if (waiter && waiter->signal_name == "navigation" && !waiter->completed) {
            waiter->completed = true;
            if (waiter->callback) {
                try {
                    waiter->callback();
                } catch (const std::exception& e) {
                    // Log error but don't crash
                    debug_output("Error in navigation callback: " + std::string(e.what()));
                }
            }
        }
    }
}

void Browser::notifyUriChanged() {
    if (!is_valid.load()) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(signal_mutex);
    
    for (auto& waiter : signal_waiters) {
        if (waiter && waiter->signal_name == "uri-change" && !waiter->completed) {
            waiter->completed = true;
            if (waiter->callback) {
                try {
                    waiter->callback();
                } catch (const std::exception& e) {
                    debug_output("Error in URI change callback: " + std::string(e.what()));
                }
            }
        }
    }
}

void Browser::notifyTitleChanged() {
    if (!is_valid.load()) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(signal_mutex);
    
    for (auto& waiter : signal_waiters) {
        if (waiter && waiter->signal_name == "title-change" && !waiter->completed) {
            waiter->completed = true;
            if (waiter->callback) {
                try {
                    waiter->callback();
                } catch (const std::exception& e) {
                    debug_output("Error in title change callback: " + std::string(e.what()));
                }
            }
        }
    }
}

void Browser::notifyReadyToShow() {
    if (!is_valid.load()) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(signal_mutex);
    
    for (auto& waiter : signal_waiters) {
        if (waiter && waiter->signal_name == "ready-to-show" && !waiter->completed) {
            waiter->completed = true;
            if (waiter->callback) {
                try {
                    waiter->callback();
                } catch (const std::exception& e) {
                    debug_output("Error in ready to show callback: " + std::string(e.what()));
                }
            }
        }
    }
}

// ========== JavaScript Observer Setup Methods ==========

std::string Browser::setupDOMObserver(const std::string& selector, int timeout_ms) {
    // Escape double quotes and backslashes in selector for JavaScript double-quoted string
    std::string escaped_selector = selector;
    size_t pos = 0;
    // Escape backslashes first to avoid double-escaping
    while ((pos = escaped_selector.find("\\", pos)) != std::string::npos) {
        escaped_selector.replace(pos, 1, "\\\\");
        pos += 2;
    }
    pos = 0;
    // Then escape double quotes
    while ((pos = escaped_selector.find("\"", pos)) != std::string::npos) {
        escaped_selector.replace(pos, 1, "\\\"");
        pos += 2;
    }
    
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
            
        })('" + escaped_selector + "', " + std::to_string(timeout_ms) + ");";
    )";
}

std::string Browser::setupVisibilityObserver(const std::string& selector, int timeout_ms) {
    // Escape double quotes and backslashes in selector for JavaScript double-quoted string
    std::string escaped_selector = selector;
    size_t pos = 0;
    // Escape backslashes first to avoid double-escaping
    while ((pos = escaped_selector.find("\\", pos)) != std::string::npos) {
        escaped_selector.replace(pos, 1, "\\\\");
        pos += 2;
    }
    pos = 0;
    // Then escape double quotes
    while ((pos = escaped_selector.find("\"", pos)) != std::string::npos) {
        escaped_selector.replace(pos, 1, "\\\"");
        pos += 2;
    }
    
    return R"(
        (function(selector, timeout) {
            window._hweb_event_result = undefined;
            
            const element = document.querySelector(selector);
            if (!element) {
                window._hweb_event_result = false;
                return;
            }
            
            // Enhanced visibility checking function
            const isElementVisible = (el) => {
                // Check if element exists
                if (!el) return false;
                
                // Check bounding box dimensions (relaxed for headless environment)
                const rect = el.getBoundingClientRect();
                // In headless mode, elements might not have proper dimensions even when visible
                // Only fail if both width AND height are exactly 0
                if (rect.width === 0 && rect.height === 0) {
                    return false;
                }
                
                // Check computed styles for visibility
                const style = window.getComputedStyle(el);
                if (style.display === 'none') return false;
                if (style.visibility === 'hidden') return false;
                if (style.opacity === '0' || style.opacity === 0) return false;
                
                // Check if element is positioned off-screen
                if (rect.left < -1000 || rect.top < -1000) return false;
                
                // Check parent chain for visibility
                let parent = el.parentElement;
                while (parent && parent !== document.body) {
                    const parentStyle = window.getComputedStyle(parent);
                    if (parentStyle.display === 'none') return false;
                    if (parentStyle.visibility === 'hidden') return false;
                    parent = parent.parentElement;
                }
                
                return true;
            };
            
            // Check if already visible
            if (isElementVisible(element)) {
                window._hweb_event_result = true;
                return;
            }
            
            // Enhanced polling for visibility with MutationObserver fallback
            let attempts = 0;
            const maxAttempts = timeout / 100;
            
            // Set up MutationObserver for style changes
            const observer = new MutationObserver((mutations) => {
                if (isElementVisible(element)) {
                    observer.disconnect();
                    window._hweb_event_result = true;
                }
            });
            
            // Observe the element and its ancestors for attribute and style changes
            observer.observe(element, {
                attributes: true,
                attributeFilter: ['style', 'class'],
                subtree: false
            });
            
            // Also observe the parent for changes that might affect visibility
            if (element.parentElement) {
                observer.observe(element.parentElement, {
                    attributes: true,
                    attributeFilter: ['style', 'class'],
                    childList: true,
                    subtree: true
                });
            }
            
            const checkVisibility = () => {
                attempts++;
                if (isElementVisible(element)) {
                    observer.disconnect();
                    window._hweb_event_result = true;
                } else if (attempts >= maxAttempts) {
                    observer.disconnect();
                    window._hweb_event_result = false;
                } else {
                    setTimeout(checkVisibility, 100);
                }
            };
            
            // Start polling after a small delay
            setTimeout(checkVisibility, 100);
            
            // Cleanup timeout
            setTimeout(() => {
                observer.disconnect();
                if (window._hweb_event_result === undefined) {
                    window._hweb_event_result = false;
                }
            }, timeout);
            
        })('" + escaped_selector + "', " + std::to_string(timeout_ms) + ");";
    )";
}

std::string Browser::setupNavigationObserver(int timeout_ms) {
    return R"(
        (function(timeout, initialUrl) {
            window._hweb_event_result = undefined;
            
            // Debug output for troubleshooting
            console.log('Navigation Observer - Initial URL: "' + initialUrl + '"');
            console.log('Navigation Observer - Current URL: "' + window.location.href + '"');
            
            // Check for URL changes
            const checkNavigation = () => {
                if (window.location.href !== initialUrl) {
                    console.log('Navigation detected: ' + initialUrl + ' -> ' + window.location.href);
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
                    console.log('Navigation timeout after ' + attempts + ' attempts');
                    window._hweb_event_result = false;
                } else {
                    setTimeout(poll, 500);
                }
            };
            
            // Start polling
            setTimeout(poll, 500);
            
        })(" + std::to_string(timeout_ms) + ", '" + previous_url + "');";
    )";
}

std::string Browser::setupConditionObserver(const std::string& condition, int timeout_ms) {
    // Enhanced escaping for JavaScript strings - escape backslashes first, then quotes
    std::string escaped_condition = condition;
    size_t pos = 0;
    
    // Escape backslashes first
    while ((pos = escaped_condition.find("\\", pos)) != std::string::npos) {
        escaped_condition.replace(pos, 1, "\\\\");
        pos += 2;
    }
    
    // Then escape single quotes
    pos = 0;
    while ((pos = escaped_condition.find("'", pos)) != std::string::npos) {
        escaped_condition.replace(pos, 1, "\\'");
        pos += 2;
    }
    
    // Enhanced condition observer with comprehensive debugging and error handling
    return R"(
        (function(condition, timeout) {
            window._hweb_event_result = undefined;
            window._hweb_debug_info = {
                condition: condition,
                timeout: timeout,
                startTime: Date.now(),
                attempts: 0,
                lastError: null,
                lastResult: null
            };
            
            // Enhanced condition checking with better error reporting
            const checkCondition = () => {
                try {
                    window._hweb_debug_info.attempts++;
                    
                    // Debug logging
                    if (window.console && window.console.log && window._hweb_debug_info.attempts <= 3) {
                        console.log('Condition check attempt', window._hweb_debug_info.attempts, ':', condition);
                    }
                    
                    // Evaluate the condition
                    const result = eval(condition);
                    window._hweb_debug_info.lastResult = result;
                    
                    if (window.console && window.console.log && window._hweb_debug_info.attempts <= 3) {
                        console.log('Condition result:', result, 'Type:', typeof result);
                    }
                    
                    return result;
                } catch(e) {
                    window._hweb_debug_info.lastError = e.message || e.toString();
                    if (window.console && window.console.log) {
                        console.log('Condition evaluation error:', e.message, 'Condition:', condition);
                    }
                    return false;
                }
            };
            
            // Check immediately with enhanced logging
            if (window.console && window.console.log) {
                console.log('Setting up condition observer - Condition:', condition, 'Timeout:', timeout);
            }
            
            const initialResult = checkCondition();
            if (initialResult) {
                window._hweb_event_result = true;
                if (window.console && window.console.log) {
                    console.log('Condition immediately satisfied!');
                }
                return;
            }
            
            // Enhanced polling with better timeout management
            const startTime = Date.now();
            const maxAttempts = Math.max(timeout / 100, 10); // At least 10 attempts
            
            const poll = () => {
                try {
                    const elapsed = Date.now() - startTime;
                    
                    // Check timeout first
                    if (elapsed >= timeout) {
                        window._hweb_event_result = false;
                        if (window.console && window.console.log) {
                            console.log('Condition timeout after', elapsed, 'ms, attempts:', window._hweb_debug_info.attempts);
                        }
                        return;
                    }
                    
                    // Check condition
                    const conditionResult = checkCondition();
                    if (conditionResult) {
                        window._hweb_event_result = true;
                        if (window.console && window.console.log) {
                            console.log('Condition satisfied after', elapsed, 'ms, attempts:', window._hweb_debug_info.attempts);
                        }
                        return;
                    }
                    
                    // Continue polling if we haven't exceeded attempts or timeout
                    if (window._hweb_debug_info.attempts < maxAttempts) {
                        setTimeout(poll, 100);
                    } else {
                        window._hweb_event_result = false;
                        if (window.console && window.console.log) {
                            console.log('Condition failed after max attempts:', maxAttempts);
                        }
                    }
                    
                } catch(pollError) {
                    window._hweb_debug_info.lastError = pollError.message || pollError.toString();
                    if (window.console && window.console.log) {
                        console.log('Polling error:', pollError.message);
                    }
                    
                    // Try to continue or fail
                    const elapsed = Date.now() - startTime;
                    if (elapsed >= timeout || window._hweb_debug_info.attempts >= maxAttempts) {
                        window._hweb_event_result = false;
                    } else {
                        setTimeout(poll, 200); // Slower retry on error
                    }
                }
            };
            
            // Start polling after a small delay
            setTimeout(poll, 100);
            
        })('" + escaped_condition + "', " + std::to_string(timeout_ms) + ");";
    )";
}

// ========== Event-driven Waiting Implementations ==========

bool Browser::waitForSelectorEvent(const std::string& selector, int timeout_ms) {
    // Signal-based approach: Set up DOM mutation observer and wait for signal
    
    // Escape double quotes and backslashes in selector for JavaScript double-quoted string
    std::string escaped_selector = selector;
    size_t pos = 0;
    // Escape backslashes first to avoid double-escaping
    while ((pos = escaped_selector.find("\\", pos)) != std::string::npos) {
        escaped_selector.replace(pos, 1, "\\\\");
        pos += 2;
    }
    pos = 0;
    // Then escape double quotes
    while ((pos = escaped_selector.find("\"", pos)) != std::string::npos) {
        escaped_selector.replace(pos, 1, "\\\"");
        pos += 2;
    }
    
    // Handle zero or negative timeout - perform single immediate check
    if (timeout_ms <= 0) {
        std::string result = executeJavascriptSync(
            "(function() { "
            "  try { "
            "    const element = document.querySelector(\"" + escaped_selector + "\"); "
            "    return element !== null ? 'true' : 'false'; "
            "  } catch(e) { return 'false'; } "
            "})()");
        return result == "true";
    }
    
    // Check if element already exists
    std::string immediate_result = executeJavascriptSync(
        "(function() { "
        "  try { "
        "    const element = document.querySelector(\"" + escaped_selector + "\"); "
        "    return element !== null ? 'true' : 'false'; "
        "  } catch(e) { return 'false'; } "
        "})()");
    
    if (immediate_result == "true") {
        return true;
    }
    
    // Set up signal-based DOM mutation observer
    std::string condition = "document.querySelector(\"" + escaped_selector + "\") !== null";
    return waitForSignalCondition("load-changed", condition, timeout_ms);
}

bool Browser::waitForNavigationEvent(int timeout_ms) {
    // Signal-based approach: Wait for navigation completion signal
    return waitForWebKitSignal("load-changed", timeout_ms);
}

bool Browser::waitForNavigationSignal(int timeout_ms) {
    // CRITICAL SEGFAULT FIX: Add object validity check
    if (!is_valid.load()) {
        return false;
    }
    
    // Handle zero or negative timeout - perform single immediate check
    if (timeout_ms <= 0) {
        if (webView) {
            std::string ready_state = executeJavascriptSync("document.readyState");
            return ready_state == "complete";
        }
        return false;
    }
    
    // Signal-based approach: Wait for document ready state
    std::string condition = "document.readyState === 'complete'";
    return waitForSignalCondition("load-changed", condition, timeout_ms);
}

bool Browser::waitForBackForwardNavigation(int timeout_ms) {
    // CRITICAL SEGFAULT FIX: Add object validity check
    if (!is_valid.load()) {
        return false;
    }
    
    // For back/forward navigation, URL change is more reliable than load events
    std::string initial_url = getCurrentUrl();
    debug_output("Waiting for back/forward navigation from: " + initial_url);
    
    // Handle zero or negative timeout - perform single immediate check
    if (timeout_ms <= 0) {
        std::string current_url = getCurrentUrl();
        return (current_url != initial_url && !current_url.empty());
    }
    
    const int check_interval = 50; // Faster checking for back/forward
    int elapsed = 0;
    
    while (elapsed < timeout_ms) {
        // CRITICAL SEGFAULT FIX: Check object validity before each operation
        if (!is_valid.load()) {
            return false;
        }
        
        // CRITICAL SEGFAULT FIX: Removed unsafe GTK event processing that could access freed memory
        // The original code processed GTK events manually which could trigger callbacks on destroyed objects
        
        std::string current_url = getCurrentUrl();
        if (current_url != initial_url && !current_url.empty()) {
            debug_output("Back/forward navigation detected: " + current_url);
            return true;
        }
        
        // EVENT-DRIVEN APPROACH: Process pending events before minimal sleep
        while (g_main_context_pending(g_main_context_default())) {
            g_main_context_iteration(g_main_context_default(), FALSE);
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(std::min(check_interval, 25)));
        elapsed += check_interval;
    }
    
    debug_output("Back/forward navigation timeout");
    return false;
}

bool Browser::waitForVisibilityEvent(const std::string& selector, int timeout_ms) {
    std::string observerScript = setupVisibilityObserver(selector, timeout_ms);
    
    executeJavascriptSync("window._hweb_event_result = undefined;");
    executeJavascriptSync(observerScript);
    
    int elapsed = 0;
    const int check_interval = 100;
    
    while (elapsed < timeout_ms) {
        // Use shorter wait intervals for better responsiveness
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
    // Signal-based approach: Use the signal waiting infrastructure
    return waitForSignalCondition("load-changed", js_condition, timeout_ms);
}

bool Browser::waitForPageReadyEvent(int timeout_ms) {
    // CRITICAL REPLACEMENT: Use event-driven readiness detection instead of blocking wait(3000)
    
    if (!readiness_tracker_) {
        // Fallback to simplified checking if readiness tracker unavailable
        bool document_ready = waitForConditionEvent("document.readyState === 'complete'", timeout_ms / 2);
        if (!document_ready) {
            document_ready = waitForConditionEvent("document.readyState === 'interactive'", timeout_ms / 4);
        }
        return document_ready;
    }
    
    // Use new event-driven approach - no blocking waits
    try {
        // First check if already ready
        if (readiness_tracker_->isBasicReady()) {
            return true;
        }
        
        // Setup comprehensive readiness detection
        readiness_tracker_->setupJavaScriptReadinessDetection();
        
        // Execute readiness check script
        std::string readiness_script = readiness_tracker_->generateReadinessCheckScript();
        std::string result = executeJavascriptSync(readiness_script);
        
        // Parse result and update readiness state
        if (result.find("\"dom_ready\":true") != std::string::npos) {
            readiness_tracker_->updateDOMReady();
        }
        
        if (result.find("\"javascript_ready\":true") != std::string::npos) {
            readiness_tracker_->updateJavaScriptReady();
        }
        
        if (result.find("\"resources_loaded\":true") != std::string::npos) {
            readiness_tracker_->updateResourcesLoaded();
        }
        
        if (result.find("\"fonts_loaded\":true") != std::string::npos) {
            readiness_tracker_->updateFontsLoaded();
        }
        
        if (result.find("\"images_loaded\":true") != std::string::npos) {
            readiness_tracker_->updateImagesLoaded();
        }
        
        if (result.find("\"styles_applied\":true") != std::string::npos) {
            readiness_tracker_->updateStylesApplied();
        }
        
        // Wait for basic readiness asynchronously
        auto readiness_future = readiness_tracker_->waitForBasicReadiness(timeout_ms);
        
        // Use std::future::wait_for for non-blocking timeout
        auto status = readiness_future.wait_for(std::chrono::milliseconds(timeout_ms));
        
        if (status == std::future_status::ready) {
            return readiness_future.get();
        } else {
            // Timeout - return current basic readiness state
            return readiness_tracker_->isBasicReady();
        }
        
    } catch (const std::exception& e) {
        // Fallback to basic DOM ready check
        return waitForConditionEvent("document.readyState === 'complete'", timeout_ms / 2);
    }
}

// ========== Public Wrapper Methods ==========

bool Browser::waitForSelector(const std::string& selector, int timeout_ms) {
    return waitForSelectorEvent(selector, timeout_ms);
}

bool Browser::waitForNavigation(int timeout_ms) {
    return waitForNavigationSignal(timeout_ms);
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
    // Use event-driven approach for page stabilization
    if (async_nav_) {
        auto future = async_nav_->waitForRenderingComplete(timeout_ms);
        if (future.wait_for(std::chrono::milliseconds(timeout_ms)) != std::future_status::ready || !future.get()) {
            // Fallback to readiness tracker or brief wait
            if (readiness_tracker_ && readiness_tracker_->isFullyReady()) {
                return; // Already ready
            }
            wait(200); // Reduced fallback wait
        }
    } else if (!waitForPageReadyEvent(timeout_ms)) {
        wait(200); // Reduced fallback wait
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
    
    // Escape double quotes and backslashes in selector for JavaScript double-quoted string
    std::string escaped_selector = selector;
    size_t pos = 0;
    // Escape backslashes first to avoid double-escaping
    while ((pos = escaped_selector.find("\\", pos)) != std::string::npos) {
        escaped_selector.replace(pos, 1, "\\\\");
        pos += 2;
    }
    pos = 0;
    // Then escape double quotes
    while ((pos = escaped_selector.find("\"", pos)) != std::string::npos) {
        escaped_selector.replace(pos, 1, "\\\"");
        pos += 2;
    }
    
    // Then wait for it to have content
    std::string condition = "document.querySelector(\"" + escaped_selector + "\") && "
                           "document.querySelector(\"" + escaped_selector + "\").innerText.trim().length > 0";
    
    return waitForConditionEvent(condition, timeout_ms / 2);
}

// ========== Signal-Based Waiting Infrastructure ==========

bool Browser::waitForSignalCondition(const std::string& signal_name, const std::string& condition, int timeout_ms) {
    // Check condition immediately first
    std::string result = executeJavascriptSync(
        "(function() { "
        "  try { return (" + condition + ") ? 'true' : 'false'; } "
        "  catch(e) { return 'false'; } "
        "})()");
    
    if (result == "true") {
        return true;
    }
    
    // Set up signal waiter with proper synchronization
    std::atomic<bool> condition_met{false};
    std::atomic<bool> timed_out{false};
    std::atomic<bool> timeout_auto_removed{false};
    
    // Set up timeout with tracking for automatic removal
    struct TimeoutData {
        std::atomic<bool>* timed_out_flag;
        std::atomic<bool>* auto_removed_flag;
    };
    
    TimeoutData timeout_data{&timed_out, &timeout_auto_removed};
    
    guint timeout_id = g_timeout_add(timeout_ms, [](gpointer user_data) -> gboolean {
        TimeoutData* data = static_cast<TimeoutData*>(user_data);
        data->timed_out_flag->store(true);
        data->auto_removed_flag->store(true);  // Mark as auto-removed
        return G_SOURCE_REMOVE;
    }, &timeout_data);
    
    // EVENT-DRIVEN: Use more aggressive timing to prevent test hangs
    const int check_interval = std::min(timeout_ms / 20, 100); // More frequent event processing, less frequent JS
    const int js_check_interval = std::max(check_interval * 4, 200); // JS checks less frequently
    int elapsed = 0;
    int js_elapsed = 0;
    
    while (elapsed < timeout_ms && !timed_out.load()) {
        // EVENT-DRIVEN: Process events first to handle signals before expensive JavaScript execution
        while (g_main_context_pending(g_main_context_default())) {
            g_main_context_iteration(g_main_context_default(), FALSE);
        }
        
        // Check condition with reduced frequency to prevent recursive blocking and hangs
        if (js_elapsed >= js_check_interval) {
            try {
                std::string current_result = executeJavascriptSync(
                    "(function() { "
                    "  try { return (" + condition + ") ? 'true' : 'false'; } "
                    "  catch(e) { return 'false'; } "
                    "})()");
                
                if (current_result == "true") {
                    if (!timeout_auto_removed.load()) {
                        g_source_remove(timeout_id);
                    }
                    return true;
                }
                js_elapsed = 0; // Reset JS check counter
            } catch (const std::exception& e) {
                debug_output("JavaScript condition check failed: " + std::string(e.what()));
                // Don't fail completely, just skip this check
            }
        }
        
        // EVENT-DRIVEN: Process events + minimal sleep to avoid blocking
        while (g_main_context_pending(g_main_context_default())) {
            g_main_context_iteration(g_main_context_default(), FALSE);
        }
        
        const int sleep_time = std::min(check_interval, 25);
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time));
        elapsed += sleep_time;
        js_elapsed += sleep_time;
    }
    
    if (!timeout_auto_removed.load()) {
        g_source_remove(timeout_id);
    }
    return false;
}

bool Browser::waitForWebKitSignal(const std::string& signal_name, int timeout_ms) {
    // Simple signal-based waiting - just wait for any load event to occur
    std::atomic<bool> signal_received{false};
    std::atomic<bool> timed_out{false};
    
    // Set up timeout
    guint timeout_id = g_timeout_add(timeout_ms, [](gpointer user_data) -> gboolean {
        std::atomic<bool>* timeout_flag = static_cast<std::atomic<bool>*>(user_data);
        timeout_flag->store(true);
        return G_SOURCE_REMOVE;
    }, &timed_out);
    
    // Monitor for load-changed signals by processing main context
    const int check_interval = 50;
    int elapsed = 0;
    
    while (elapsed < timeout_ms && !timed_out.load()) {
        // Process pending events to handle WebKit signals
        while (g_main_context_pending(nullptr)) {
            g_main_context_iteration(nullptr, FALSE);
            // If any event was processed, consider signal received
            signal_received.store(true);
        }
        
        if (signal_received.load()) {
            g_source_remove(timeout_id);
            return true;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(check_interval));
        elapsed += check_interval;
    }
    
    if (timeout_id != 0) {
        g_source_remove(timeout_id);
    }
    return false;
}

void Browser::checkSignalConditions() {
    std::lock_guard<std::mutex> lock(signal_mutex);
    
    for (auto& waiter : signal_waiters) {
        if (!waiter->completed && !waiter->condition.empty()) {
            if (waiter->callback()) {
                waiter->completed = true;
                g_main_loop_quit(main_loop);
                break;
            }
        }
    }
}


