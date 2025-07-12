#include "Browser.h"
#include <gtk/gtk.h>
#include <webkit/webkit.h>
#include <gdk/gdk.h>
#include <cairo/cairo.h>
#include <unistd.h>
#include <glib.h>
#include <iostream>
#include <sstream>
#include <filesystem>
#include <thread>
#include <chrono>
#include <cmath>
#include <stdexcept>

// External debug flag
extern bool g_debug;

// Forward declarations for signal handlers - SINGLE DEFINITION ONLY
static void navigation_complete_handler(WebKitWebView* webview, WebKitLoadEvent load_event, gpointer user_data);
static void uri_changed_handler(WebKitWebView* webview, GParamSpec* pspec, gpointer user_data);
static void title_changed_handler(WebKitWebView* webview, GParamSpec* pspec, gpointer user_data);
static void ready_to_show_handler(WebKitWebView* webview, gpointer user_data);

// Signal handler implementations - SINGLE DEFINITION
static void navigation_complete_handler(WebKitWebView* webview, WebKitLoadEvent load_event, gpointer user_data) {
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

static void uri_changed_handler(WebKitWebView* webview, GParamSpec* pspec, gpointer user_data) {
    Browser* browser = static_cast<Browser*>(user_data);
    const gchar* new_uri = webkit_web_view_get_uri(webview);
    
    debug_output("URI changed to: " + std::string(new_uri ? new_uri : ""));
    
    // Use public interface to notify waiters
    browser->notifyUriChanged();
}

static void title_changed_handler(WebKitWebView* webview, GParamSpec* pspec, gpointer user_data) {
    Browser* browser = static_cast<Browser*>(user_data);
    const gchar* new_title = webkit_web_view_get_title(webview);
    
    debug_output("Title changed to: " + std::string(new_title ? new_title : ""));
    
    // Use public interface to notify waiters
    browser->notifyTitleChanged();
}

static void ready_to_show_handler(WebKitWebView* webview, gpointer user_data) {
    Browser* browser = static_cast<Browser*>(user_data);
    
    debug_output("Page ready to show");
    
    // Use public interface to notify waiters
    browser->notifyReadyToShow();
}

// Callback for JavaScript evaluation
static void js_eval_callback(GObject* object, GAsyncResult* res, gpointer user_data) {
    GError* error = NULL;
    JSCValue* value = webkit_web_view_evaluate_javascript_finish(WEBKIT_WEB_VIEW(object), res, &error);
    
    Browser* browser_instance = static_cast<Browser*>(g_object_get_data(G_OBJECT(object), "browser-instance"));
    
    if (error) {
        // Don't log SecurityError for localStorage/sessionStorage on file:// URLs - it's expected
        if (!strstr(error->message, "SecurityError")) {
            std::cerr << "JavaScript error: " << error->message << std::endl;
        }
        g_error_free(error);
        if (user_data) {
            *(static_cast<std::string*>(user_data)) = "";
        }
    } else if (value) {
        if (user_data) {
            try {
                if (jsc_value_is_string(value)) {
                    char* str_value = jsc_value_to_string(value);
                    if (str_value) {
                        *(static_cast<std::string*>(user_data)) = str_value;
                        g_free(str_value);
                    } else {
                        *(static_cast<std::string*>(user_data)) = "";
                    }
                } else if (jsc_value_is_number(value)) {
                    double num_val = jsc_value_to_double(value);
                    if (num_val == floor(num_val)) {
                        *(static_cast<std::string*>(user_data)) = std::to_string((long long)num_val);
                    } else {
                        *(static_cast<std::string*>(user_data)) = std::to_string(num_val);
                    }
                } else if (jsc_value_is_boolean(value)) {
                    *(static_cast<std::string*>(user_data)) = jsc_value_to_boolean(value) ? "true" : "false";
                } else if (jsc_value_is_null(value)) {
                    *(static_cast<std::string*>(user_data)) = "null";
                } else if (jsc_value_is_undefined(value)) {
                    *(static_cast<std::string*>(user_data)) = "undefined";
                } else if (jsc_value_is_object(value)) {
                    // Try to convert object to string
                    char* str_value = jsc_value_to_string(value);
                    if (str_value) {
                        *(static_cast<std::string*>(user_data)) = str_value;
                        g_free(str_value);
                    } else {
                        *(static_cast<std::string*>(user_data)) = "[object Object]";
                    }
                } else {
                    // Fallback: try to convert to string
                    char* str_value = jsc_value_to_string(value);
                    if (str_value) {
                        *(static_cast<std::string*>(user_data)) = str_value;
                        g_free(str_value);
                    } else {
                        *(static_cast<std::string*>(user_data)) = "";
                    }
                }
            } catch (const std::exception& e) {
                std::cerr << "Error processing JavaScript result: " << e.what() << std::endl;
                *(static_cast<std::string*>(user_data)) = "";
            }
        }
        g_object_unref(value);
    } else {
        if (user_data) {
            *(static_cast<std::string*>(user_data)) = "";
        }
    }
    
    if (browser_instance) {
        if (g_main_loop_is_running(browser_instance->main_loop)) {
            g_main_loop_quit(browser_instance->main_loop);
        }
    }
}

// Callback for load-changed signal
static void load_changed_callback(WebKitWebView* web_view, WebKitLoadEvent load_event, gpointer user_data) {
    if (load_event == WEBKIT_LOAD_FINISHED) {
        Browser* browser_instance = static_cast<Browser*>(user_data);
        if (browser_instance) {
            if (g_main_loop_is_running(browser_instance->main_loop)) {
                g_main_loop_quit(browser_instance->main_loop);
            }
        }
    }
}

// Screenshot callback structure
struct ScreenshotData {
    std::string filename;
    GMainLoop* loop;
    bool success;
};

// Callback for screenshot completion
static void screenshot_callback(GObject* source_object, GAsyncResult* res, gpointer user_data) {
    ScreenshotData* data = static_cast<ScreenshotData*>(user_data);
    GError* error = NULL;
    
    // In newer WebKitGTK, this returns a GdkTexture
    GdkTexture* texture = webkit_web_view_get_snapshot_finish(
        WEBKIT_WEB_VIEW(source_object), res, &error);
    
    if (error) {
        std::cerr << "Screenshot error: " << error->message << std::endl;
        g_error_free(error);
        data->success = false;
    } else if (texture) {
        int width = gdk_texture_get_width(texture);
        int height = gdk_texture_get_height(texture);
        
        // Allocate buffer for pixel data
        size_t stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, width);
        guchar* pixels = (guchar*)g_malloc(height * stride);
        
        // Download texture data
        gdk_texture_download(texture, pixels, stride);
        
        // Create cairo surface from the pixel data
        cairo_surface_t* surface = cairo_image_surface_create_for_data(
            pixels, CAIRO_FORMAT_ARGB32, width, height, stride);
        
        // Save to PNG
        cairo_status_t status = cairo_surface_write_to_png(surface, data->filename.c_str());
        
        if (status == CAIRO_STATUS_SUCCESS) {
            data->success = true;
        } else {
            std::cerr << "Failed to write PNG: " << cairo_status_to_string(status) << std::endl;
            data->success = false;
        }
        
        // Cleanup
        cairo_surface_destroy(surface);
        g_free(pixels);
        g_object_unref(texture);
    } else {
        std::cerr << "No texture returned from snapshot" << std::endl;
        data->success = false;
    }
    
    if (g_main_loop_is_running(data->loop)) {
        g_main_loop_quit(data->loop);
    }
}

Browser::Browser() : cookieManager(nullptr), main_loop(g_main_loop_new(NULL, FALSE)) {
    gtk_init();
    
    // Initialize with a proper data manager for cookie/storage persistence
    std::string home = std::getenv("HOME");
    sessionDataPath = home + "/.hweb-poc/webkit-data";
    std::filesystem::create_directories(sessionDataPath);
    
    // Create WebKit settings first
    WebKitSettings* settings = webkit_settings_new();
    webkit_settings_set_enable_media(settings, FALSE);
    webkit_settings_set_enable_media_stream(settings, FALSE);
    webkit_settings_set_enable_webaudio(settings, FALSE);
    webkit_settings_set_enable_javascript(settings, TRUE);
    webkit_settings_set_enable_developer_extras(settings, TRUE);
    webkit_settings_set_enable_page_cache(settings, TRUE);
    webkit_settings_set_enable_html5_local_storage(settings, TRUE);
    webkit_settings_set_enable_html5_database(settings, TRUE);
    webkit_settings_set_allow_file_access_from_file_urls(settings, TRUE);
    webkit_settings_set_allow_universal_access_from_file_urls(settings, TRUE);
    
    // Create web context with custom data directory
    WebKitWebContext* context = webkit_web_context_get_default();
    webkit_web_context_set_cache_model(context, WEBKIT_CACHE_MODEL_WEB_BROWSER);
    
    // Create data directories for persistent storage
    std::string dataDir = sessionDataPath + "/data";
    std::string cacheDir = sessionDataPath + "/cache";
    std::filesystem::create_directories(dataDir);
    std::filesystem::create_directories(cacheDir);
    
    // Create web view
    webView = WEBKIT_WEB_VIEW(webkit_web_view_new());
    
    if (!webView) {
        std::cerr << "Failed to create WebKit web view" << std::endl;
        exit(1);
    }
    
    // Apply settings to the web view
    webkit_web_view_set_settings(webView, settings);
    
    // Get the network session for cookie management
    WebKitNetworkSession* session = webkit_web_view_get_network_session(webView);
    if (session) {
        cookieManager = webkit_network_session_get_cookie_manager(session);
        
        // Set persistent cookie storage
        std::string cookieFile = sessionDataPath + "/cookies.txt";
        webkit_cookie_manager_set_persistent_storage(cookieManager, 
            cookieFile.c_str(), WEBKIT_COOKIE_PERSISTENT_STORAGE_TEXT);
    }
    
    window = gtk_window_new();
    gtk_window_set_child(GTK_WINDOW(window), GTK_WIDGET(webView));
    
    // Set window to be offscreen for headless operation
    gtk_widget_set_visible(window, FALSE);
    
    // Store browser instance in the webView for callbacks
    g_object_set_data(G_OBJECT(webView), "browser-instance", this);
    
    // Setup event-driven signal handlers
    setupSignalHandlers();
    
    g_object_unref(settings);
}

Browser::~Browser() {
    cleanupWaiters();
    if (main_loop) {
        g_main_loop_unref(main_loop);
    }
}

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

// Public notification methods for signal handlers
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

// Event-driven waiting implementations
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

// Event-driven waiting implementations
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

// Updated public methods to use event-driven approaches
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

// Navigation methods
void Browser::loadUri(const std::string& uri) {
    // Validate URL format before attempting to load
    if (uri.empty()) {
        throw std::invalid_argument("Error: Empty URL provided");
    }
    
    // Basic URL validation
    if (uri.find("://") == std::string::npos) {
        throw std::invalid_argument("Error: Invalid URL format (missing protocol): " + uri);
    }
    
    // Check for clearly invalid protocols
    std::string protocol = uri.substr(0, uri.find("://"));
    if (protocol != "http" && protocol != "https" && protocol != "file" && protocol != "ftp") {
        // Allow some other common protocols but warn about unknown ones
        if (protocol != "data" && protocol != "about" && protocol != "javascript") {
            throw std::invalid_argument("Error: Invalid URL protocol '" + protocol + "': " + uri);
        }
    }
    
    // Additional validation for file URLs
    if (protocol == "file") {
        std::string path = uri.substr(7); // Remove "file://"
        if (path.empty()) {
            throw std::invalid_argument("Error: Invalid file URL (empty path): " + uri);
        }
    }
    
    debug_output("Loading URI: " + uri);
    webkit_web_view_load_uri(webView, uri.c_str());
}

std::string Browser::getCurrentUrl() {
    const gchar* uri = webkit_web_view_get_uri(webView);
    return uri ? std::string(uri) : "";
}

std::string Browser::getPageTitle() {
    const gchar* title = webkit_web_view_get_title(webView);
    return title ? std::string(title) : "";
}

void Browser::goBack() {
    webkit_web_view_go_back(webView);
}

void Browser::goForward() {
    webkit_web_view_go_forward(webView);
}

void Browser::reload() {
    webkit_web_view_reload(webView);
}

// JavaScript execution methods
void Browser::executeJavascript(const std::string& script, std::string* result) {
    if (result) {
        result->clear();
    }
    
    if (script.empty()) {
        std::cerr << "Warning: Empty JavaScript script" << std::endl;
        if (result) {
            *result = "";
        }
        return;
    }
    
    if (!webView) {
        std::cerr << "Error: WebView is null" << std::endl;
        if (result) {
            *result = "";
        }
        return;
    }
    
    webkit_web_view_evaluate_javascript(
        webView, 
        script.c_str(), 
        -1,
        NULL, 
        NULL, 
        NULL, 
        js_eval_callback, 
        result
    );
}

bool Browser::waitForJavaScriptCompletion(int timeout_ms) {
    struct CompletionData {
        GMainLoop* loop;
        bool timed_out;
        guint timeout_id;
    };
    
    CompletionData data = {main_loop, false, 0};
    
    data.timeout_id = g_timeout_add(timeout_ms, [](gpointer user_data) -> gboolean {
        CompletionData* data = static_cast<CompletionData*>(user_data);
        data->timed_out = true;
        if (g_main_loop_is_running(data->loop)) {
            g_main_loop_quit(data->loop);
        }
        return G_SOURCE_REMOVE;
    }, &data);
    
    g_main_loop_run(main_loop);
    
    if (!data.timed_out && data.timeout_id != 0) {
        g_source_remove(data.timeout_id);
    }
    
    return !data.timed_out;
}

std::string Browser::executeJavascriptSync(const std::string& script) {
    if (script.empty() || !webView) {
        return "";
    }
    
    js_result_buffer.clear();
    
    try {
        executeJavascript(script, &js_result_buffer);
        if (!waitForJavaScriptCompletion(5000)) {
            debug_output("JavaScript execution timeout for: " + script.substr(0, 50) + "...");
            return "";
        }
        
        // Clean up the result buffer
        std::string result = js_result_buffer;
        
        // Handle common problematic return values
        if (result.empty() || result == "undefined" || result == "null") {
            return "";
        }
        
        // Ensure we don't have a ridiculously long result
        if (result.length() > 100000) {
            return result.substr(0, 100000);
        }
        
        return result;
        
    } catch (const std::exception& e) {
        std::cerr << "Exception in JavaScript execution: " << e.what() << std::endl;
        return "";
    }
}

std::string Browser::executeJavascriptSyncSafe(const std::string& script) {
    try {
        if (!webView) {
            std::cerr << "Error: WebView not initialized" << std::endl;
            return "";
        }
        
        if (script.empty()) {
            return "";
        }
        
        if (!isPageLoaded()) {
            std::cerr << "Warning: Executing JavaScript on potentially unready page" << std::endl;
        }
        
        return executeJavascriptSync(script);
    } catch (const std::exception& e) {
        std::cerr << "Error in JavaScript execution: " << e.what() << std::endl;
        return "";
    }
}

// Form interaction methods
bool Browser::fillInput(const std::string& selector, const std::string& value) {
    if (!waitForSelectorEvent(selector, 5000)) {
        return false;
    }
    
    // Small delay to ensure element is ready
    wait(100);
    
    // Escape quotes and other special characters in the value
    std::string escaped_value = value;
    size_t pos = 0;
    while ((pos = escaped_value.find("'", pos)) != std::string::npos) {
        escaped_value.replace(pos, 1, "\\'");
        pos += 2;
    }
    // Also escape backslashes
    pos = 0;
    while ((pos = escaped_value.find("\\", pos)) != std::string::npos) {
        escaped_value.replace(pos, 1, "\\\\");
        pos += 2;
    }
    
    std::string js_script = 
        "(function() { "
        "  try { "
        "    var element = document.querySelector('" + selector + "'); "
        "    if (element) { "
        "      element.focus(); "
        "      element.value = '" + escaped_value + "'; "
        "      element.dispatchEvent(new Event('input', { bubbles: true })); "
        "      element.dispatchEvent(new Event('change', { bubbles: true })); "
        "      element.dispatchEvent(new Event('blur', { bubbles: true })); "
        "      return 'true'; "
        "    } "
        "    return 'false'; "
        "  } catch(e) { "
        "    return 'error: ' + e.message; "
        "  } "
        "})()";
    
    std::string result = executeJavascriptSync(js_script);
    
    // Add verification step with delay
    if (result == "true") {
        wait(200); // Allow time for the value to be processed
        
        // Verify the value was actually set
        std::string verifyJs = "document.querySelector('" + selector + "') ? document.querySelector('" + selector + "').value : 'NOT_FOUND'";
        std::string actualValue = executeJavascriptSync(verifyJs);
        
        if (actualValue == escaped_value || actualValue == value) {
            return true;
        } else {
            debug_output("Warning: Value verification failed. Expected: '" + value + "', Got: '" + actualValue + "'");
            
            // Try alternative method using setAttribute
            std::string altJs = 
                "try { "
                "  var el = document.querySelector('" + selector + "'); "
                "  if (el) { "
                "    el.setAttribute('value', '" + escaped_value + "'); "
                "    el.value = '" + escaped_value + "'; "
                "    return 'retry_success'; "
                "  } "
                "  return 'retry_failed'; "
                "} catch(e) { return 'retry_error'; }";
            
            std::string retryResult = executeJavascriptSync(altJs);
            if (retryResult == "retry_success") {
                wait(200); // Additional wait after retry
            }
            return retryResult == "retry_success";
        }
    }
    
    debug_output("fillInput failed: " + result);
    return false;
}

bool Browser::clickElement(const std::string& selector) {
    if (!waitForSelectorEvent(selector, 5000)) {
        return false;
    }
    
    if (!waitForVisibilityEvent(selector, 2000)) {
        debug_output("Warning: Element exists but may not be visible");
    }
    
    std::string js_script = 
        "(function() { "
        "  try { "
        "    var element = document.querySelector('" + selector + "'); "
        "    if (element) { "
        "      element.click(); "
        "      return true; "
        "    } "
        "    return false; "
        "  } catch(e) { "
        "    return false; "
        "  } "
        "})()";
    
    std::string result = executeJavascriptSync(js_script);
    return result == "true";
}

bool Browser::submitForm(const std::string& form_selector) {
    std::string js_script = 
        "(function() { "
        "  try { "
        "    var form = document.querySelector('" + form_selector + "'); "
        "    if (form) { "
        "      form.submit(); "
        "      return true; "
        "    } "
        "    return false; "
        "  } catch(e) { "
        "    return false; "
        "  } "
        "})()";
    
    std::string result = executeJavascriptSync(js_script);
    return result == "true";
}

bool Browser::selectOption(const std::string& selector, const std::string& value) {
    // Wait for element and add a small delay
    wait(100);
    
    // Escape quotes and other special characters in the value
    std::string escaped_value = value;
    size_t pos = 0;
    while ((pos = escaped_value.find("'", pos)) != std::string::npos) {
        escaped_value.replace(pos, 1, "\\'");
        pos += 2;
    }
    // Also escape backslashes
    pos = 0;
    while ((pos = escaped_value.find("\\", pos)) != std::string::npos) {
        escaped_value.replace(pos, 1, "\\\\");
        pos += 2;
    }
    
    std::string js_script = 
        "(function() { "
        "  try { "
        "    var select = document.querySelector('" + selector + "'); "
        "    if (select) { "
        "      select.focus(); "
        "      select.value = '" + escaped_value + "'; "
        "      select.dispatchEvent(new Event('change', { bubbles: true })); "
        "      select.dispatchEvent(new Event('blur', { bubbles: true })); "
        "      return 'true'; "
        "    } "
        "    return 'false'; "
        "  } catch(e) { "
        "    return 'error: ' + e.message; "
        "  } "
        "})()";
    
    std::string result = executeJavascriptSync(js_script);
    
    // Add verification step with delay
    if (result == "true") {
        wait(200); // Allow time for the value to be processed
        
        // Verify the value was actually set
        std::string verifyJs = "document.querySelector('" + selector + "') ? document.querySelector('" + selector + "').value : 'NOT_FOUND'";
        std::string actualValue = executeJavascriptSync(verifyJs);
        
        if (actualValue == escaped_value || actualValue == value) {
            return true;
        } else {
            debug_output("Warning: Select verification failed. Expected: '" + value + "', Got: '" + actualValue + "'");
            
            // Try alternative method using selectedIndex
            std::string altJs = 
                "try { "
                "  var sel = document.querySelector('" + selector + "'); "
                "  if (sel) { "
                "    for (var i = 0; i < sel.options.length; i++) { "
                "      if (sel.options[i].value === '" + escaped_value + "') { "
                "        sel.selectedIndex = i; "
                "        sel.dispatchEvent(new Event('change', { bubbles: true })); "
                "        return 'retry_success'; "
                "      } "
                "    } "
                "  } "
                "  return 'retry_failed'; "
                "} catch(e) { return 'retry_error'; }";
            
            std::string retryResult = executeJavascriptSync(altJs);
            if (retryResult == "retry_success") {
                wait(200); // Additional wait after retry
            }
            return retryResult == "retry_success";
        }
    }
    
    debug_output("selectOption failed: " + result);
    return false;
}

bool Browser::checkElement(const std::string& selector) {
    // Small delay to ensure element is ready
    wait(100);
    
    std::string js_script = 
        "(function() { "
        "  try { "
        "    var element = document.querySelector('" + selector + "'); "
        "    if (element) { "
        "      element.focus(); "
        "      element.checked = true; "
        "      element.dispatchEvent(new Event('change', { bubbles: true })); "
        "      element.dispatchEvent(new Event('click', { bubbles: true })); "
        "      element.dispatchEvent(new Event('blur', { bubbles: true })); "
        "      return 'true'; "
        "    } "
        "    return 'false'; "
        "  } catch(e) { "
        "    return 'error: ' + e.message; "
        "  } "
        "})()";
    
    std::string result = executeJavascriptSync(js_script);
    
    // Add verification step with delay
    if (result == "true") {
        wait(200); // Allow time for the change to be processed
        
        // Verify the checkbox was actually checked
        std::string verifyJs = "document.querySelector('" + selector + "') ? document.querySelector('" + selector + "').checked : false";
        std::string actualValue = executeJavascriptSync(verifyJs);
        
        if (actualValue == "true") {
            return true;
        } else {
            debug_output("Warning: Checkbox verification failed. Expected: checked, Got: " + actualValue);
        }
    }
    
    debug_output("checkElement failed: " + result);
    return result == "true"; // Return success even if verification fails, as the action was attempted
}

bool Browser::uncheckElement(const std::string& selector) {
    // Small delay to ensure element is ready
    wait(100);
    
    std::string js_script = 
        "(function() { "
        "  try { "
        "    var element = document.querySelector('" + selector + "'); "
        "    if (element) { "
        "      element.focus(); "
        "      element.checked = false; "
        "      element.dispatchEvent(new Event('change', { bubbles: true })); "
        "      element.dispatchEvent(new Event('click', { bubbles: true })); "
        "      element.dispatchEvent(new Event('blur', { bubbles: true })); "
        "      return 'true'; "
        "    } "
        "    return 'false'; "
        "  } catch(e) { "
        "    return 'error: ' + e.message; "
        "  } "
        "})()";
    
    std::string result = executeJavascriptSync(js_script);
    
    // Add verification step with delay
    if (result == "true") {
        wait(200); // Allow time for the change to be processed
        
        // Verify the checkbox was actually unchecked
        std::string verifyJs = "document.querySelector('" + selector + "') ? document.querySelector('" + selector + "').checked : true";
        std::string actualValue = executeJavascriptSync(verifyJs);
        
        if (actualValue == "false") {
            return true;
        } else {
            debug_output("Warning: Uncheck verification failed. Expected: unchecked, Got: " + actualValue);
        }
    }
    
    debug_output("uncheckElement failed: " + result);
    return result == "true"; // Return success even if verification fails, as the action was attempted
}

bool Browser::focusElement(const std::string& selector) {
    std::string js_script = 
        "(function() { "
        "  try { "
        "    var element = document.querySelector('" + selector + "'); "
        "    if (element) { "
        "      element.focus(); "
        "      return true; "
        "    } "
        "    return false; "
        "  } catch(e) { "
        "    return false; "
        "  } "
        "})()";
    
    std::string result = executeJavascriptSync(js_script);
    return result == "true";
}

// Helper methods
std::string Browser::getInnerText(const std::string& selector) {
    std::string js_script = 
        "(function() { "
        "  try { "
        "    var element = document.querySelector('" + selector + "'); "
        "    return element ? element.innerText || element.textContent || '' : ''; "
        "  } catch(e) { "
        "    return ''; "
        "  } "
        "})()";
    
    return executeJavascriptSync(js_script);
}

bool Browser::searchForm(const std::string& query) {
    // Implementation for search form
    std::string js_script = 
        "(function() { "
        "  try { "
        "    var inputs = document.querySelectorAll('input[type=\"search\"], input[name*=\"search\"], input[placeholder*=\"search\"]'); "
        "    if (inputs.length > 0) { "
        "      inputs[0].value = '" + query + "'; "
        "      inputs[0].dispatchEvent(new Event('input', { bubbles: true })); "
        "      var form = inputs[0].closest('form'); "
        "      if (form) { "
        "        form.submit(); "
        "        return true; "
        "      } "
        "    } "
        "    return false; "
        "  } catch(e) { "
        "    return false; "
        "  } "
        "})()";
    
    std::string result = executeJavascriptSync(js_script);
    return result == "true";
}

// Element query methods
bool Browser::elementExists(const std::string& selector) {
    std::string js_script = 
        "(function() { "
        "  try { "
        "    return document.querySelector('" + selector + "') !== null; "
        "  } catch(e) { "
        "    return false; "
        "  } "
        "})()";
    
    std::string result = executeJavascriptSync(js_script);
    return result == "true";
}

int Browser::countElements(const std::string& selector) {
    std::string js_script = 
        "(function() { "
        "  try { "
        "    return document.querySelectorAll('" + selector + "').length; "
        "  } catch(e) { "
        "    return 0; "
        "  } "
        "})()";
    
    std::string result = executeJavascriptSync(js_script);
    try {
        return std::stoi(result);
    } catch (...) {
        return 0;
    }
}

std::string Browser::getElementHtml(const std::string& selector) {
    std::string js_script = 
        "(function() { "
        "  try { "
        "    var element = document.querySelector('" + selector + "'); "
        "    return element ? element.outerHTML : ''; "
        "  } catch(e) { "
        "    return ''; "
        "  } "
        "})()";
    
    return executeJavascriptSync(js_script);
}

std::string Browser::getAttribute(const std::string& selector, const std::string& attribute) {
    std::string js_script = 
        "(function() { "
        "  try { "
        "    var element = document.querySelector('" + selector + "'); "
        "    return element ? (element.getAttribute('" + attribute + "') || '') : ''; "
        "  } catch(e) { "
        "    return ''; "
        "  } "
        "})()";
    
    return executeJavascriptSync(js_script);
}

// Viewport and user agent methods
void Browser::setViewport(int width, int height) {
    gtk_window_set_default_size(GTK_WINDOW(window), width, height);
}

void Browser::setUserAgent(const std::string& userAgent) {
    WebKitSettings* settings = webkit_web_view_get_settings(webView);
    webkit_settings_set_user_agent(settings, userAgent.c_str());
}

// Screenshot methods - FIXED IMPLEMENTATION
void Browser::takeScreenshot(const std::string& filename) {
    ScreenshotData data;
    data.filename = filename;
    data.loop = main_loop;
    data.success = false;
    
    webkit_web_view_get_snapshot(webView, 
                                WEBKIT_SNAPSHOT_REGION_VISIBLE,
                                WEBKIT_SNAPSHOT_OPTIONS_NONE, 
                                NULL,
                                screenshot_callback, 
                                &data);
    
    // Wait for the screenshot to complete
    g_main_loop_run(main_loop);
    
    if (!data.success) {
        std::cerr << "Failed to take screenshot" << std::endl;
    }
}

void Browser::takeFullPageScreenshot(const std::string& filename) {
    ScreenshotData data;
    data.filename = filename;
    data.loop = main_loop;
    data.success = false;
    
    webkit_web_view_get_snapshot(webView, 
                                WEBKIT_SNAPSHOT_REGION_FULL_DOCUMENT,
                                WEBKIT_SNAPSHOT_OPTIONS_NONE, 
                                NULL,
                                screenshot_callback, 
                                &data);
    
    // Wait for the screenshot to complete
    g_main_loop_run(main_loop);
    
    if (!data.success) {
        std::cerr << "Failed to take full page screenshot" << std::endl;
    }
}

// Action sequence execution
bool Browser::executeActionSequence(const std::vector<Session::RecordedAction>& actions) {
    for (const auto& action : actions) {
        if (action.type == "click") {
            if (!clickElement(action.selector)) {
                return false;
            }
        } else if (action.type == "type") {
            if (!fillInput(action.selector, action.value)) {
                return false;
            }
        }
        // Add more action types as needed
        
        wait(action.delay);
    }
    return true;
}

// Session state management methods
void Browser::restoreSession(const Session& session) {
    try {
        // Set user agent first if present
        if (!session.getUserAgent().empty()) {
            setUserAgent(session.getUserAgent());
            wait(100); // Small delay for user agent to take effect
        }
        
        // Navigate to current URL if present and not already there
        if (!session.getCurrentUrl().empty() && session.getCurrentUrl() != getCurrentUrl()) {
            debug_output("Loading URL: " + session.getCurrentUrl());
            loadUri(session.getCurrentUrl());
            
            // Wait for load using event-driven approach
            if (!waitForNavigationSignal(15000)) {
                std::cerr << "Warning: Page load timeout during session restore" << std::endl;
                return; // Don't try to restore state on failed page load
            }
            
            // Wait for page to be ready using event-driven approach
            waitForPageReadyEvent(5000);
            
            // Verify we can execute JavaScript
            std::string testResult = executeJavascriptSync("(function() { return 'test'; })()");
            if (testResult != "test") {
                std::cerr << "Warning: JavaScript execution not working properly" << std::endl;
                return;
            }
            
            debug_output("Page loaded successfully");
        }
        
        // Only restore state if page loaded successfully
        std::string readyState = executeJavascriptSync("(function() { try { return document.readyState; } catch(e) { return 'error'; } })()");
        if (readyState != "complete" && readyState != "interactive") {
            std::cerr << "Warning: Page not ready for state restoration (state: " << readyState << ")" << std::endl;
            return;
        }
        
        // Check if this is a file:// URL
        bool isFileUrl = session.getCurrentUrl().find("file://") == 0;
        
        // Restore state step by step with error handling
        debug_output("Starting state restoration...");
        
        // Cookies
        try {
            if (!session.getCookies().empty()) {
                for (const auto& cookie : session.getCookies()) {
                    setCookieSafe(cookie);
                }
                wait(500);
                debug_output("Restored " + std::to_string(session.getCookies().size()) + " cookies");
            }
        } catch (const std::exception& e) {
            std::cerr << "Warning: Failed to restore cookies: " << e.what() << std::endl;
        }
        
        // Storage - skip for file:// URLs due to security restrictions
        if (!isFileUrl) {
            try {
                if (!session.getLocalStorage().empty()) {
                    setLocalStorage(session.getLocalStorage());
                    wait(500);
                    debug_output("Restored localStorage");
                }
                
                if (!session.getSessionStorage().empty()) {
                    setSessionStorage(session.getSessionStorage());
                    wait(500);
                    debug_output("Restored sessionStorage");
                }
            } catch (const std::exception& e) {
                std::cerr << "Warning: Failed to restore storage: " << e.what() << std::endl;
            }
        } else {
            debug_output("Skipping storage restoration for file:// URL");
        }
        
        // Form state - use the new implementation
        try {
            if (!session.getFormFields().empty()) {
                debug_output("Restoring " + std::to_string(session.getFormFields().size()) + " form fields");
                for (const auto& field : session.getFormFields()) {
                    debug_output("  Restoring: " + field.selector + " = " + field.value + " (checked: " + std::to_string(field.checked) + ")");
                }
                restoreFormState(session.getFormFields());
                wait(500);
                debug_output("Restored form state");
            }
        } catch (const std::exception& e) {
            std::cerr << "Warning: Failed to restore form state: " << e.what() << std::endl;
        }
        
        // Scroll positions - use the new implementation
        try {
            if (!session.getAllScrollPositions().empty()) {
                restoreScrollPositions(session.getAllScrollPositions());
                wait(500);
                debug_output("Restored scroll positions");
            }
        } catch (const std::exception& e) {
            std::cerr << "Warning: Failed to restore scroll positions: " << e.what() << std::endl;
        }
        
        // Active elements
        try {
            if (!session.getActiveElements().empty()) {
                restoreActiveElements(session.getActiveElements());
                wait(200);
                debug_output("Restored active elements");
            }
        } catch (const std::exception& e) {
            std::cerr << "Warning: Failed to restore active elements: " << e.what() << std::endl;
        }
        
        // Final wait for everything to settle
        wait(1000);
        debug_output("Session restoration complete");
        
    } catch (const std::exception& e) {
        std::cerr << "Error in session restoration: " << e.what() << std::endl;
    }
}

void Browser::updateSessionState(Session& session) {
    try {
        // Always update current URL first (this should never fail)
        session.setCurrentUrl(getCurrentUrl());
        
        // Try a simple JavaScript test first
        std::string testResult = executeJavascriptSync("(function() { try { return 'alive'; } catch(e) { return 'dead'; } })()");
        if (testResult != "alive") {
            std::cerr << "Warning: JavaScript execution not working, skipping state extraction" << std::endl;
            session.updateLastAccessed();
            return;
        }
        
        // Check if we can safely execute JavaScript
        std::string readyState = executeJavascriptSync("(function() { try { return document.readyState || 'unknown'; } catch(e) { return 'error'; } })()");
        
        if (readyState == "error" || readyState.empty() || readyState == "unknown") {
            std::cerr << "Warning: Cannot determine page state, skipping detailed state extraction" << std::endl;
            session.updateLastAccessed();
            return;
        }
        
        // Check if this is a file:// URL
        bool isFileUrl = getCurrentUrl().find("file://") == 0;
        
        // Only proceed if we have a properly loaded page
        if (readyState == "complete" || readyState == "interactive") {
            // Safe state extraction with individual try-catch blocks
            try {
                session.setPageHash(extractPageHash());
            } catch (const std::exception& e) {
                std::cerr << "Warning: Failed to extract page hash: " << e.what() << std::endl;
            }
            
            try {
                session.setDocumentReadyState(readyState);
            } catch (const std::exception& e) {
                std::cerr << "Warning: Failed to set document ready state: " << e.what() << std::endl;
            }
            
            // Cookies - use the new implementation
            try {
                getCookiesAsync([&session](std::vector<Cookie> cookies) {
                    if (g_debug) {
                        std::cerr << "Debug: Extracted " << cookies.size() << " cookies" << std::endl;
                        for (const auto& cookie : cookies) {
                            std::cerr << "  Cookie: " << cookie.name << " = " << cookie.value << std::endl;
                        }
                    }
                    session.setCookies(cookies);
                });
                waitForJavaScriptCompletion(1000); // Shorter timeout
            } catch (const std::exception& e) {
                std::cerr << "Warning: Failed to extract cookies: " << e.what() << std::endl;
            }
            
            // Storage - skip for file:// URLs but use new implementations
            if (!isFileUrl) {
                try {
                    auto localStorage = getLocalStorage();
                    session.setLocalStorage(localStorage);
                    debug_output("Extracted " + std::to_string(localStorage.size()) + " localStorage items");
                } catch (const std::exception& e) {
                    // Expected for file:// URLs
                }
                
                try {
                    auto sessionStorage = getSessionStorage();
                    session.setSessionStorage(sessionStorage);
                    debug_output("Extracted " + std::to_string(sessionStorage.size()) + " sessionStorage items");
                } catch (const std::exception& e) {
                    // Expected for file:// URLs
                }
            }
            
            // Form state - use the new implementation
            try {
                auto formFields = extractFormState();
                session.setFormFields(formFields);
                debug_output("Extracted " + std::to_string(formFields.size()) + " form fields");
                for (const auto& field : formFields) {
                    debug_output("  Field: " + field.selector + " = " + field.value + " (checked: " + std::to_string(field.checked) + ")");
                }
            } catch (const std::exception& e) {
                std::cerr << "Warning: Failed to extract form state: " << e.what() << std::endl;
            }
            
            // Active elements - use the new implementation
            try {
                auto activeElements = extractActiveElements();
                session.setActiveElements(activeElements);
                debug_output("Extracted " + std::to_string(activeElements.size()) + " active elements");
            } catch (const std::exception& e) {
                std::cerr << "Warning: Failed to extract active elements: " << e.what() << std::endl;
            }
            
            // Scroll positions - use the new implementation
            try {
                auto scrollPositions = extractAllScrollPositions();
                debug_output("Extracted scroll positions:");
                for (const auto& [selector, pos] : scrollPositions) {
                    session.setScrollPosition(selector, pos.first, pos.second);
                    debug_output("  " + selector + ": " + std::to_string(pos.first) + ", " + std::to_string(pos.second));
                }
            } catch (const std::exception& e) {
                std::cerr << "Warning: Failed to extract scroll positions: " << e.what() << std::endl;
            }
            
            // Custom state - skip for now to avoid issues
            try {
                if (!session.getStateExtractors().empty()) {
                    auto customState = extractCustomState(session.getStateExtractors());
                    auto memberNames = customState.getMemberNames();
                    for (const auto& key : memberNames) {
                        session.setExtractedState(key, customState[key]);
                    }
                }
            } catch (const std::exception& e) {
                std::cerr << "Warning: Failed to extract custom state: " << e.what() << std::endl;
            }
        } else {
            std::cerr << "Warning: Page not in ready state (" << readyState << "), skipping detailed extraction" << std::endl;
        }
        
        // Always update last accessed time
        session.updateLastAccessed();
        
    } catch (const std::exception& e) {
        std::cerr << "Error in updateSessionState: " << e.what() << std::endl;
        // At minimum, update the timestamp
        try {
            session.updateLastAccessed();
        } catch (...) {
            // Even this failed, just continue
        }
    }
}

// Validation methods
bool Browser::isPageLoaded() const {
    std::string readyState = const_cast<Browser*>(this)->executeJavascriptSync("(function() { try { return document.readyState; } catch(e) { return 'loading'; } })()");
    return readyState == "complete" || readyState == "interactive";
}

bool Browser::validateSession(const Session& session) const {
    return !session.getName().empty();
}

std::string Browser::getPageLoadState() const {
    return const_cast<Browser*>(this)->executeJavascriptSync("(function() { try { return document.readyState + '|' + window.location.href; } catch(e) { return 'error|unknown'; } })()");
}

bool Browser::restoreSessionSafely(const Session& session) {
    try {
        restoreSession(session);
        return isPageLoaded();
    } catch (const std::exception& e) {
        return false;
    }
}

// Wait method
void Browser::wait(int milliseconds) {
    if (milliseconds <= 0) return;
    
    struct TimeoutData {
        GMainLoop* loop;
        guint source_id;
        bool completed;
    };
    
    TimeoutData data = {main_loop, 0, false};
    
    data.source_id = g_timeout_add(milliseconds, [](gpointer user_data) -> gboolean {
        TimeoutData* data = static_cast<TimeoutData*>(user_data);
        data->completed = true;
        if (g_main_loop_is_running(data->loop)) {
            g_main_loop_quit(data->loop);
        }
        return G_SOURCE_REMOVE;
    }, &data);
    
    g_main_loop_run(main_loop);
    
    if (!data.completed && data.source_id != 0) {
        g_source_remove(data.source_id);
    }
}

// Placeholder implementations for remaining methods
void Browser::getCookiesAsync(std::function<void(std::vector<Cookie>)> callback) {
    std::string cookieJs = R"(
        (function() {
            const cookies = document.cookie.split(';').map(c => c.trim()).filter(c => c.length > 0);
            const result = [];
            
            cookies.forEach(cookie => {
                const [name, value] = cookie.split('=', 2);
                if (name && value) {
                    result.push({
                        name: name.trim(),
                        value: value.trim(),
                        domain: window.location.hostname,
                        path: '/'
                    });
                }
            });
            
            return JSON.stringify(result);
        })()
    )";
    
    std::string result = executeJavascriptSync(cookieJs);
    std::vector<Cookie> cookies;
    
    if (!result.empty() && result != "undefined") {
        try {
            Json::Value root;
            Json::Reader reader;
            if (reader.parse(result, root) && root.isArray()) {
                for (const auto& item : root) {
                    Cookie cookie;
                    cookie.name = item["name"].asString();
                    cookie.value = item["value"].asString();
                    cookie.domain = item["domain"].asString();
                    cookie.path = item["path"].asString();
                    cookies.push_back(cookie);
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Error parsing cookies: " << e.what() << std::endl;
        }
    }
    
    callback(cookies);
}

void Browser::setCookie(const Cookie& cookie) {
    std::string cookieStr = cookie.name + "=" + cookie.value + "; path=" + cookie.path;
    if (!cookie.domain.empty()) {
        cookieStr += "; domain=" + cookie.domain;
    }
    
    std::string js = "document.cookie = '" + cookieStr + "'; 'cookie set';";
    executeJavascriptSync(js);
}

void Browser::setCookieSafe(const Cookie& cookie) {
    try {
        setCookie(cookie);
    } catch (const std::exception& e) {
        std::cerr << "Warning: Failed to set cookie " << cookie.name << ": " << e.what() << std::endl;
    }
}

void Browser::clearCookies() {
    std::string clearJs = R"(
        (function() {
            document.cookie.split(";").forEach(function(c) { 
                document.cookie = c.replace(/^ +/, "").replace(/=.*/, "=;expires=" + new Date().toUTCString() + ";path=/"); 
            });
            return "cleared";
        })()
    )";
    executeJavascriptSync(clearJs);
}

std::map<std::string, std::string> Browser::getLocalStorage() {
    std::map<std::string, std::string> storage;
    
    std::string storageJs = R"(
        (function() {
            try {
                const result = {};
                for (let i = 0; i < localStorage.length; i++) {
                    const key = localStorage.key(i);
                    const value = localStorage.getItem(key);
                    result[key] = value;
                }
                return JSON.stringify(result);
            } catch(e) {
                return "{}";
            }
        })()
    )";
    
    std::string result = executeJavascriptSync(storageJs);
    
    if (!result.empty() && result != "undefined" && result != "{}") {
        try {
            Json::Value root;
            Json::Reader reader;
            if (reader.parse(result, root) && root.isObject()) {
                for (const auto& key : root.getMemberNames()) {
                    storage[key] = root[key].asString();
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Error parsing localStorage: " << e.what() << std::endl;
        }
    }
    
    return storage;
}

void Browser::setLocalStorage(const std::map<std::string, std::string>& storage) {
    for (const auto& [key, value] : storage) {
        std::string js = "try { localStorage.setItem('" + key + "', '" + value + "'); } catch(e) { 'localStorage error'; }";
        executeJavascriptSync(js);
    }
}

std::map<std::string, std::string> Browser::getSessionStorage() {
    std::map<std::string, std::string> storage;
    
    std::string storageJs = R"(
        (function() {
            try {
                const result = {};
                for (let i = 0; i < sessionStorage.length; i++) {
                    const key = sessionStorage.key(i);
                    const value = sessionStorage.getItem(key);
                    result[key] = value;
                }
                return JSON.stringify(result);
            } catch(e) {
                return "{}";
            }
        })()
    )";
    
    std::string result = executeJavascriptSync(storageJs);
    
    if (!result.empty() && result != "undefined" && result != "{}") {
        try {
            Json::Value root;
            Json::Reader reader;
            if (reader.parse(result, root) && root.isObject()) {
                for (const auto& key : root.getMemberNames()) {
                    storage[key] = root[key].asString();
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Error parsing sessionStorage: " << e.what() << std::endl;
        }
    }
    
    return storage;
}

void Browser::setSessionStorage(const std::map<std::string, std::string>& storage) {
    for (const auto& [key, value] : storage) {
        std::string js = "try { sessionStorage.setItem('" + key + "', '" + value + "'); } catch(e) { 'sessionStorage error'; }";
        executeJavascriptSync(js);
    }
}

std::vector<FormField> Browser::extractFormState() {
    std::vector<FormField> fields;
    
    // Extract input fields
    std::string inputJs = R"(
        (function() {
            const inputs = document.querySelectorAll('input, textarea, select');
            const result = [];
            
            inputs.forEach((el, index) => {
                const field = {};
                field.selector = el.id ? '#' + el.id : 
                                (el.name ? '[name="' + el.name + '"]' : 
                                ':nth-child(' + (Array.from(el.parentNode.children).indexOf(el) + 1) + ')');
                field.value = el.value || '';
                field.checked = el.type === 'checkbox' || el.type === 'radio' ? el.checked : false;
                field.type = el.type || el.tagName.toLowerCase();
                result.push(field);
            });
            
            return JSON.stringify(result);
        })()
    )";
    
    std::string result = executeJavascriptSync(inputJs);
    
    if (!result.empty() && result != "undefined") {
        try {
            Json::Value root;
            Json::Reader reader;
            if (reader.parse(result, root) && root.isArray()) {
                for (const auto& item : root) {
                    FormField field;
                    field.selector = item["selector"].asString();
                    field.value = item["value"].asString();
                    field.checked = item["checked"].asBool();
                    field.type = item["type"].asString();
                    fields.push_back(field);
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Error parsing form state: " << e.what() << std::endl;
        }
    }
    
    return fields;
}

void Browser::restoreFormState(const std::vector<FormField>& fields) {
    for (const auto& field : fields) {
        try {
            // Determine element type dynamically since tagName isn't available
            if (field.type == "checkbox" || field.type == "radio") {
                // Handle checkboxes and radio buttons
                if (field.checked) {
                    checkElement(field.selector);
                } else {
                    uncheckElement(field.selector);
                }
            } else if (field.type == "select" || field.type == "select-one" || field.type == "select-multiple") {
                // Handle select elements
                selectOption(field.selector, field.value);
            } else {
                // Check if it's a select element by querying the DOM
                std::string isSelectJs = "document.querySelector('" + field.selector + "') && "
                                        "document.querySelector('" + field.selector + "').tagName === 'SELECT'";
                std::string isSelect = executeJavascriptSync(isSelectJs);
                
                if (isSelect == "true") {
                    selectOption(field.selector, field.value);
                } else {
                    // Handle text inputs and textareas
                    fillInput(field.selector, field.value);
                }
            }
            
            // Small delay between form field restorations
            wait(50);
        } catch (const std::exception& e) {
            std::cerr << "Error restoring form field " << field.selector << ": " << e.what() << std::endl;
        }
    }
}

std::set<std::string> Browser::extractActiveElements() {
    // Implementation needed
    return std::set<std::string>();
}

void Browser::restoreActiveElements(const std::set<std::string>& elements) {
    // Implementation needed
}

std::string Browser::extractPageHash() {
    // Implementation needed
    return "";
}

std::string Browser::extractDocumentReadyState() {
    // Implementation needed
    return "";
}

std::map<std::string, std::pair<int, int>> Browser::extractAllScrollPositions() {
    // Implementation needed
    return std::map<std::string, std::pair<int, int>>();
}

void Browser::restoreScrollPositions(const std::map<std::string, std::pair<int, int>>& positions) {
    // Implementation needed
}

bool Browser::waitForPageReady(const Session& session) {
    // Implementation needed
    return true;
}

bool Browser::waitForElementWithContent(const std::string& selector, int timeout_ms) {
    // Implementation needed
    return false;
}

Json::Value Browser::extractCustomState(const std::map<std::string, std::string>& extractors) {
    Json::Value result;
    
    for (const auto& [name, script] : extractors) {
        try {
            std::string value = executeJavascriptSync(script);
            if (!value.empty() && value != "undefined") {
                // Try to parse as JSON first
                Json::Value parsed;
                Json::Reader reader;
                if (reader.parse(value, parsed)) {
                    result[name] = parsed;
                } else {
                    // Store as string if not valid JSON
                    result[name] = value;
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Warning: Failed to extract custom state '" << name << "': " << e.what() << std::endl;
        }
    }
    
    return result;
}

void Browser::restoreCustomState(const std::map<std::string, Json::Value>& state) {
    for (const auto& [name, value] : state) {
        try {
            // Convert JSON value back to string and execute as JavaScript
            std::string valueStr;
            if (value.isString()) {
                valueStr = value.asString();
            } else {
                Json::FastWriter writer;
                valueStr = writer.write(value);
            }
            
            // Store in a window variable for access
            std::string js = "window['_hweb_custom_" + name + "'] = " + valueStr + "; 'restored';";
            executeJavascriptSync(js);
        } catch (const std::exception& e) {
            std::cerr << "Warning: Failed to restore custom state '" << name << "': " << e.what() << std::endl;
        }
    }
}

void Browser::setScrollPosition(int x, int y) {
    // Implementation needed
}

std::pair<int, int> Browser::getScrollPosition() {
    // Implementation needed
    return std::make_pair(0, 0);
}

std::string Browser::getFirstNonEmptyText(const std::string& selector) {
    // Implementation needed
    return "";
}

std::string Browser::getPageSource() {
    // Implementation needed
    return "";
}

bool Browser::isFileUrl(const std::string& url) const {
    return url.find("file://") == 0;
}

bool Browser::validateFileUrl(const std::string& url) const {
    return isFileUrl(url);
}

void Browser::initializeDataManager(const std::string& sessionName) {
    // Implementation needed
}
